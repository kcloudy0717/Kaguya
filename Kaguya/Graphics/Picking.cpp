#include "pch.h"
#include "Picking.h"

#include <ResourceUploadBatch.h>
#include "RendererRegistry.h"

using namespace DirectX;

namespace
{
	// Symbols
	constexpr LPCWSTR g_RayGeneration = L"RayGeneration";
	constexpr LPCWSTR g_Miss = L"Miss";
	constexpr LPCWSTR g_ClosestHit = L"ClosestHit";

	// HitGroup Exports
	constexpr LPCWSTR g_HitGroupExport = L"Default";

	ShaderIdentifier g_RayGenerationSID;
	ShaderIdentifier g_MissSID;
	ShaderIdentifier g_DefaultSID;
}

void Picking::Create()
{
	auto& RenderDevice = RenderDevice::Instance();

	m_GlobalRS = RenderDevice.CreateRootSignature([](RootSignatureBuilder& Builder)
	{
		Builder.AddConstantBufferView<0, 0>();	// g_SystemConstants		b0 | space0
		Builder.AddShaderResourceView<0, 0>();	// Scene					t0 | space0
		Builder.AddUnorderedAccessView<0, 0>();	// PickingResult,			u0 | space0
	});

	m_RTPSO = RenderDevice.CreateRaytracingPipelineState([&](RaytracingPipelineStateBuilder& Builder)
	{
		Builder.AddLibrary(Libraries::Picking,
			{
				g_RayGeneration,
				g_Miss,
				g_ClosestHit
			});

		Builder.AddHitGroup(g_HitGroupExport, nullptr, g_ClosestHit, nullptr);

		Builder.SetGlobalRootSignature(m_GlobalRS);

		Builder.SetRaytracingShaderConfig(sizeof(int), SizeOfBuiltInTriangleIntersectionAttributes);
		Builder.SetRaytracingPipelineConfig(1);
	});

	g_RayGenerationSID = m_RTPSO.GetShaderIdentifier(L"RayGeneration");
	g_MissSID = m_RTPSO.GetShaderIdentifier(L"Miss");
	g_DefaultSID = m_RTPSO.GetShaderIdentifier(L"Default");

	ResourceUploadBatch uploader(RenderDevice.GetDevice());

	uploader.Begin(D3D12_COMMAND_LIST_TYPE_COPY);

	D3D12MA::ALLOCATION_DESC allocationDesc = {};
	allocationDesc.Flags = D3D12MA::ALLOCATION_FLAG_COMMITTED;
	allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

	// Ray Generation Shader Table
	{
		m_RayGenerationShaderTable.AddShaderRecord(g_RayGenerationSID);

		UINT64 sbtSize = m_RayGenerationShaderTable.GetSizeInBytes();

		SharedGraphicsResource rayGenSBTUpload = RenderDevice.GraphicsMemory()->Allocate(sbtSize);
		m_RayGenerationSBT = RenderDevice.CreateBuffer(&allocationDesc, sbtSize);

		m_RayGenerationShaderTable.AssociateResource(m_RayGenerationSBT->pResource.Get());
		m_RayGenerationShaderTable.Generate(static_cast<BYTE*>(rayGenSBTUpload.Memory()));

		uploader.Upload(m_RayGenerationShaderTable.Resource(), rayGenSBTUpload);
	}

	// Miss Shader Table
	{
		ShaderTable<void>::Record Record = {};
		Record.ShaderIdentifier = g_MissSID;

		m_MissShaderTable.AddShaderRecord(Record);

		UINT64 sbtSize = m_MissShaderTable.GetSizeInBytes();

		SharedGraphicsResource missSBTUpload = RenderDevice.GraphicsMemory()->Allocate(sbtSize);
		m_MissSBT = RenderDevice.CreateBuffer(&allocationDesc, sbtSize);

		m_MissShaderTable.AssociateResource(m_MissSBT->pResource.Get());
		m_MissShaderTable.Generate(static_cast<BYTE*>(missSBTUpload.Memory()));

		uploader.Upload(m_MissSBT->pResource.Get(), missSBTUpload);
	}

	auto finish = uploader.End(RenderDevice.GetCopyQueue());
	finish.wait();

	m_HitGroupSBT = RenderDevice.CreateBuffer(&allocationDesc, m_HitGroupShaderTable.GetStrideInBytes() * Scene::MAX_INSTANCE_SUPPORTED);

	allocationDesc = {};
	allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

	m_Result = RenderDevice.CreateBuffer(&allocationDesc, sizeof(int),
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, 0, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	allocationDesc.HeapType = D3D12_HEAP_TYPE_READBACK;

	m_Readback = RenderDevice.CreateBuffer(&allocationDesc, sizeof(int),
		D3D12_RESOURCE_FLAG_NONE, 0, D3D12_RESOURCE_STATE_COPY_DEST);

	m_HitGroupShaderTable.reserve(Scene::MAX_INSTANCE_SUPPORTED);
	m_HitGroupShaderTable.AssociateResource(m_HitGroupSBT->pResource.Get());

	m_Entities.reserve(Scene::MAX_INSTANCE_SUPPORTED);
}

void Picking::UpdateShaderTable(const RaytracingAccelerationStructure& RaytracingAccelerationStructure, CommandList& CommandList)
{
	auto& RenderDevice = RenderDevice::Instance();

	m_HitGroupShaderTable.clear();
	m_Entities.clear();
	for (auto [i, meshRenderer] : enumerate(RaytracingAccelerationStructure.m_MeshRenderers))
	{
		m_HitGroupShaderTable.AddShaderRecord(g_DefaultSID);

		Entity entity(meshRenderer->Handle, meshRenderer->pScene);
		m_Entities.push_back(entity);
	}

	UINT64 sbtSize = m_HitGroupShaderTable.GetSizeInBytes();

	GraphicsResource hitGroupUpload = RenderDevice.GraphicsMemory()->Allocate(sbtSize);

	m_HitGroupShaderTable.Generate(static_cast<BYTE*>(hitGroupUpload.Memory()));

	CommandList.TransitionBarrier(m_HitGroupSBT->pResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST);
	CommandList->CopyBufferRegion(m_HitGroupSBT->pResource.Get(), 0, hitGroupUpload.Resource(), hitGroupUpload.ResourceOffset(), hitGroupUpload.Size());
	CommandList.TransitionBarrier(m_HitGroupSBT->pResource.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
}

void Picking::ShootPickingRay(D3D12_GPU_VIRTUAL_ADDRESS SystemConstants,
	const RaytracingAccelerationStructure& Scene, CommandList& CommandList)
{
	CommandList->SetPipelineState1(m_RTPSO);
	CommandList->SetComputeRootSignature(m_GlobalRS);
	CommandList->SetComputeRootConstantBufferView(0, SystemConstants);
	CommandList->SetComputeRootShaderResourceView(1, Scene);
	CommandList->SetComputeRootUnorderedAccessView(2, m_Result->pResource->GetGPUVirtualAddress());

	D3D12_DISPATCH_RAYS_DESC desc =
	{
		.RayGenerationShaderRecord = m_RayGenerationShaderTable,
		.MissShaderTable = m_MissShaderTable,
		.HitGroupTable = m_HitGroupShaderTable,
		.Width = 1,
		.Height = 1,
		.Depth = 1
	};

	CommandList.DispatchRays(&desc);

	CommandList.TransitionBarrier(m_Result->pResource.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE);
	CommandList->CopyBufferRegion(m_Readback->pResource.Get(), 0, m_Result->pResource.Get(), 0, sizeof(int));
	CommandList.TransitionBarrier(m_Result->pResource.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
}

std::optional<Entity> Picking::GetSelectedEntity()
{
	INT instanceID = -1;
	INT* pHostMemory = nullptr;
	if (SUCCEEDED(m_Readback->pResource->Map(0, nullptr, reinterpret_cast<void**>(&pHostMemory))))
	{
		instanceID = pHostMemory[0];
		m_Readback->pResource->Unmap(0, nullptr);
	}

	if (instanceID != -1 && instanceID < m_Entities.size())
	{
		return m_Entities[instanceID];
	}

	return {};
}