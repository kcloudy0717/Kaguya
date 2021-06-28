#include "pch.h"
#include "Picking.h"

#include "RendererRegistry.h"

namespace
{
// Symbols
constexpr LPCWSTR g_RayGeneration = L"RayGeneration";
constexpr LPCWSTR g_Miss		  = L"Miss";
constexpr LPCWSTR g_ClosestHit	  = L"ClosestHit";

// HitGroup Exports
constexpr LPCWSTR g_HitGroupExport = L"Default";

ShaderIdentifier g_RayGenerationSID;
ShaderIdentifier g_MissSID;
ShaderIdentifier g_DefaultSID;
} // namespace

void Picking::Create()
{
	auto& RenderDevice = RenderDevice::Instance();

	m_GlobalRS = RenderDevice.CreateRootSignature(
		[](RootSignatureBuilder& Builder)
		{
			Builder.AddConstantBufferView<0, 0>();	// g_SystemConstants		b0 | space0
			Builder.AddShaderResourceView<0, 0>();	// Scene					t0 | space0
			Builder.AddUnorderedAccessView<0, 0>(); // PickingResult,			u0 | space0
		});

	m_RTPSO = RenderDevice.CreateRaytracingPipelineState(
		[&](RaytracingPipelineStateBuilder& Builder)
		{
			Builder.AddLibrary(Libraries::Picking, { g_RayGeneration, g_Miss, g_ClosestHit });

			Builder.AddHitGroup(g_HitGroupExport, {}, g_ClosestHit, {});

			Builder.SetGlobalRootSignature(m_GlobalRS);

			Builder.SetRaytracingShaderConfig(sizeof(int), SizeOfBuiltInTriangleIntersectionAttributes);
			Builder.SetRaytracingPipelineConfig(1);
		});

	g_RayGenerationSID = m_RTPSO.GetShaderIdentifier(L"RayGeneration");
	g_MissSID		   = m_RTPSO.GetShaderIdentifier(L"Miss");
	g_DefaultSID	   = m_RTPSO.GetShaderIdentifier(L"Default");

	ResourceUploader uploader(RenderDevice.GetDevice());

	uploader.Begin();

	// Ray Generation Shader Table
	{
		m_RayGenerationShaderTable.AddShaderRecord(g_RayGenerationSID);

		UINT64 sbtSize = m_RayGenerationShaderTable.GetSizeInBytes();

		m_RayGenerationSBT = Buffer(
			RenderDevice.GetDevice(),
			sbtSize,
			m_RayGenerationShaderTable.StrideInBytes,
			D3D12_HEAP_TYPE_DEFAULT,
			D3D12_RESOURCE_FLAG_NONE);
		std::unique_ptr<BYTE[]> data = std::make_unique<BYTE[]>(sbtSize);

		m_RayGenerationShaderTable.Generate(data.get());

		D3D12_SUBRESOURCE_DATA subresource = {};
		subresource.pData				   = data.get();
		subresource.RowPitch			   = sbtSize;
		subresource.SlicePitch			   = 0;

		uploader.Upload(subresource, m_RayGenerationSBT.GetResource());

		m_RayGenerationShaderTable.AssociateResource(m_RayGenerationSBT.GetResource());
	}

	// Miss Shader Table
	{
		m_MissShaderTable.AddShaderRecord(g_MissSID);

		UINT64 sbtSize = m_MissShaderTable.GetSizeInBytes();

		m_MissSBT = Buffer(
			RenderDevice.GetDevice(),
			sbtSize,
			m_MissShaderTable.StrideInBytes,
			D3D12_HEAP_TYPE_DEFAULT,
			D3D12_RESOURCE_FLAG_NONE);
		std::unique_ptr<BYTE[]> data = std::make_unique<BYTE[]>(sbtSize);

		m_MissShaderTable.Generate(data.get());

		D3D12_SUBRESOURCE_DATA subresource = {};
		subresource.pData				   = data.get();
		subresource.RowPitch			   = sbtSize;
		subresource.SlicePitch			   = 0;

		uploader.Upload(subresource, m_MissSBT.GetResource());

		m_MissShaderTable.AssociateResource(m_MissSBT.GetResource());
	}

	uploader.End(true);

	m_HitGroupSBT = Buffer(
		RenderDevice.GetDevice(),
		m_HitGroupShaderTable.StrideInBytes * Scene::MAX_INSTANCE_SUPPORTED,
		m_HitGroupShaderTable.StrideInBytes,
		D3D12_HEAP_TYPE_DEFAULT,
		D3D12_RESOURCE_FLAG_NONE);

	m_HitGroupShaderTable.reserve(Scene::MAX_INSTANCE_SUPPORTED);
	m_HitGroupShaderTable.AssociateResource(m_HitGroupSBT.GetResource());

	m_Result =
		Buffer(RenderDevice.GetDevice(), sizeof(int), sizeof(int), D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_FLAG_NONE);

	m_Readback =
		Buffer(RenderDevice.GetDevice(), sizeof(int), sizeof(int), D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_FLAG_NONE);

	m_Entities.reserve(Scene::MAX_INSTANCE_SUPPORTED);
}

void Picking::UpdateShaderTable(
	const RaytracingAccelerationStructure& RaytracingAccelerationStructure,
	CommandContext&						   Context)
{
	m_HitGroupShaderTable.clear();
	m_Entities.clear();
	for (auto [i, meshRenderer] : enumerate(RaytracingAccelerationStructure.MeshRenderers))
	{
		m_HitGroupShaderTable.AddShaderRecord(g_DefaultSID);

		Entity entity(meshRenderer->Handle, meshRenderer->pScene);
		m_Entities.push_back(entity);
	}

	UINT64 sbtSize = m_HitGroupShaderTable.GetSizeInBytes();

	Allocation Allocation = Context.CpuConstantAllocator.Allocate(sbtSize);

	m_HitGroupShaderTable.Generate(static_cast<BYTE*>(Allocation.CPUVirtualAddress));

	Context->CopyBufferRegion(m_HitGroupSBT.GetResource(), 0, Allocation.pResource, Allocation.Offset, Allocation.Size);
}

void Picking::ShootPickingRay(
	D3D12_GPU_VIRTUAL_ADDRESS			   SystemConstants,
	const RaytracingAccelerationStructure& Scene,
	CommandContext&						   Context)
{
	Context.CommandListHandle.GetGraphicsCommandList4()->SetPipelineState1(m_RTPSO);
	Context->SetComputeRootSignature(m_GlobalRS);
	Context->SetComputeRootConstantBufferView(0, SystemConstants);
	Context->SetComputeRootShaderResourceView(1, Scene);
	Context->SetComputeRootUnorderedAccessView(2, m_Result.GetGPUVirtualAddress());

	D3D12_DISPATCH_RAYS_DESC Desc = { .RayGenerationShaderRecord = m_RayGenerationShaderTable,
									  .MissShaderTable			 = m_MissShaderTable,
									  .HitGroupTable			 = m_HitGroupShaderTable,
									  .Width					 = 1,
									  .Height					 = 1,
									  .Depth					 = 1 };

	Context.DispatchRays(&Desc);

	Context->CopyBufferRegion(m_Readback.GetResource(), 0, m_Result.GetResource(), 0, sizeof(int));
}

std::optional<Entity> Picking::GetSelectedEntity()
{
	INT	 instanceID	 = -1;
	INT* pHostMemory = nullptr;
	if (SUCCEEDED(m_Readback.GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&pHostMemory))))
	{
		instanceID = pHostMemory[0];
		m_Readback.GetResource()->Unmap(0, nullptr);
	}

	if (instanceID != -1 && instanceID < m_Entities.size())
	{
		return m_Entities[instanceID];
	}

	return {};
}
