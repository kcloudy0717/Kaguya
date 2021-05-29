#include "pch.h"
#include "PathIntegrator.h"

#include <ResourceUploadBatch.h>
#include "RendererRegistry.h"

using namespace DirectX;

namespace
{
	// Symbols
	constexpr LPCWSTR g_RayGeneration = L"RayGeneration";
	constexpr LPCWSTR g_Miss = L"Miss";
	constexpr LPCWSTR g_ShadowMiss = L"ShadowMiss";
	constexpr LPCWSTR g_ClosestHit = L"ClosestHit";

	// HitGroup Exports
	constexpr LPCWSTR g_HitGroupExport = L"Default";

	ShaderIdentifier g_RayGenerationSID;
	ShaderIdentifier g_MissSID;
	ShaderIdentifier g_ShadowMissSID;
	ShaderIdentifier g_DefaultSID;
}

void PathIntegrator::Settings::RenderGui()
{
	if (ImGui::TreeNode("Path Integrator"))
	{
		if (ImGui::Button("Restore Defaults"))
		{
			Settings::RestoreDefaults();
		}

		bool dirty = false;
		dirty |= ImGui::SliderScalar("Max Depth", ImGuiDataType_U32, &MaxDepth, &MinimumDepth, &MaximumDepth);
		ImGui::Text("Num Samples Accumulated: %u", NumAccumulatedSamples);

		if (dirty)
		{
			NumAccumulatedSamples = 0;
		}

		ImGui::TreePop();
	}
}

PathIntegrator::PathIntegrator(
	_In_ RenderDevice& RenderDevice)
{
	Settings::RestoreDefaults();

	m_UAV = RenderDevice.GetResourceViewHeaps().AllocateResourceView();
	m_SRV = RenderDevice.GetResourceViewHeaps().AllocateResourceView();

	m_GlobalRS = RenderDevice.CreateRootSignature(
		[](RootSignatureBuilder& Builder)
		{
			Builder.AddConstantBufferView<0, 0>(); // g_SystemConstants		b0 | space0
			Builder.AddConstantBufferView<1, 0>(); // g_RenderPassData		b1 | space0

			Builder.AddShaderResourceView<0, 0>(); // g_Scene				t0 | space0
			Builder.AddShaderResourceView<1, 0>(); // g_Materials			t1 | space0
			Builder.AddShaderResourceView<2, 0>(); // g_Lights				t2 | space0

			Builder.AddStaticSampler<0, 0>(D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_WRAP, 16);	// g_SamplerPointWrap			s0 | space0;
			Builder.AddStaticSampler<1, 0>(D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 16);	// g_SamplerPointClamp			s1 | space0;
			Builder.AddStaticSampler<2, 0>(D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, 16);	// g_SamplerLinearWrap			s2 | space0;
			Builder.AddStaticSampler<3, 0>(D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 16);	// g_SamplerLinearClamp			s3 | space0;
			Builder.AddStaticSampler<4, 0>(D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_WRAP, 16);			// g_SamplerAnisotropicWrap		s4 | space0;
			Builder.AddStaticSampler<5, 0>(D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 16);			// g_SamplerAnisotropicClamp	s5 | space0;
		});

	m_LocalHitGroupRS = RenderDevice::Instance().CreateRootSignature(
		[](RootSignatureBuilder& Builder)
		{
			Builder.Add32BitConstants<0, 1>(1);		// RootConstants	b0 | space1

			Builder.AddShaderResourceView<0, 1>();	// VertexBuffer		t0 | space1
			Builder.AddShaderResourceView<1, 1>();	// IndexBuffer		t1 | space1

			Builder.SetAsLocalRootSignature();
		}, false);

	m_RTPSO = RenderDevice.CreateRaytracingPipelineState(
		[&](RaytracingPipelineStateBuilder& Builder)
		{
			Builder.AddLibrary(Libraries::PathTrace,
				{
					g_RayGeneration,
					g_Miss,
					g_ShadowMiss,
					g_ClosestHit
				});

			Builder.AddHitGroup(g_HitGroupExport, nullptr, g_ClosestHit, nullptr);

			Builder.AddRootSignatureAssociation(m_LocalHitGroupRS,
				{
					g_HitGroupExport
				});

			Builder.SetGlobalRootSignature(m_GlobalRS);

			Builder.SetRaytracingShaderConfig(12 * sizeof(float) + 2 * sizeof(unsigned int), SizeOfBuiltInTriangleIntersectionAttributes);

			// +1 for Primary, +1 for Shadow
			Builder.SetRaytracingPipelineConfig(2);
		});

	g_RayGenerationSID = m_RTPSO.GetShaderIdentifier(L"RayGeneration");
	g_MissSID = m_RTPSO.GetShaderIdentifier(L"Miss");
	g_ShadowMissSID = m_RTPSO.GetShaderIdentifier(L"ShadowMiss");
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
		m_MissShaderTable.AddShaderRecord(g_MissSID);
		m_MissShaderTable.AddShaderRecord(g_ShadowMissSID);

		UINT64 sbtSize = m_MissShaderTable.GetSizeInBytes();

		SharedGraphicsResource missSBTUpload = RenderDevice.GraphicsMemory()->Allocate(sbtSize);
		m_MissSBT = RenderDevice.CreateBuffer(&allocationDesc, sbtSize);

		m_MissShaderTable.AssociateResource(m_MissSBT->pResource.Get());
		m_MissShaderTable.Generate(static_cast<BYTE*>(missSBTUpload.Memory()));

		uploader.Upload(m_MissSBT->pResource.Get(), missSBTUpload);
	}

	auto finish = uploader.End(RenderDevice.GetCopyQueue());
	finish.wait();

	m_HitGroupSBT = RenderDevice.CreateBuffer(&allocationDesc, m_HitGroupShaderTable.StrideInBytes * Scene::MAX_INSTANCE_SUPPORTED);

	m_HitGroupShaderTable.reserve(Scene::MAX_INSTANCE_SUPPORTED);
	m_HitGroupShaderTable.AssociateResource(m_HitGroupSBT->pResource.Get());
}

void PathIntegrator::SetResolution(
	_In_ UINT Width,
	_In_ UINT Height)
{
	auto& RenderDevice = RenderDevice::Instance();

	if ((m_Width == Width && m_Height == Height))
	{
		return;
	}

	m_Width = Width;
	m_Height = Height;

	D3D12MA::ALLOCATION_DESC allocationDesc = {};
	allocationDesc.Flags = D3D12MA::ALLOCATION_FLAG_COMMITTED;
	allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

	auto resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R32G32B32A32_FLOAT, Width, Height);
	resourceDesc.MipLevels = 1;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	m_RenderTarget = RenderDevice.CreateResource(&allocationDesc,
		&resourceDesc, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, nullptr);

	RenderDevice.CreateUnorderedAccessView(m_RenderTarget->pResource.Get(), m_UAV);
	RenderDevice.CreateShaderResourceView(m_RenderTarget->pResource.Get(), m_SRV);

	Reset();
}

void PathIntegrator::Reset()
{
	Settings::NumAccumulatedSamples = 0;
}

void PathIntegrator::UpdateShaderTable(
	_In_ const RaytracingAccelerationStructure& RaytracingAccelerationStructure,
	_In_ CommandList& CommandList)
{
	auto& RenderDevice = RenderDevice::Instance();

	m_HitGroupShaderTable.clear();
	for (auto [i, meshRenderer] : enumerate(RaytracingAccelerationStructure.m_MeshRenderers))
	{
		auto meshFilter = meshRenderer->pMeshFilter;

		const auto& vertexBuffer = meshFilter->Mesh->VertexResource->pResource;
		const auto& indexBuffer = meshFilter->Mesh->IndexResource->pResource;

		RaytracingShaderTable<RootArgument>::Record shaderRecord = {};
		shaderRecord.ShaderIdentifier = g_DefaultSID;
		shaderRecord.RootArguments =
		{
			.MaterialIndex = (UINT)i,
			.Padding = 0,
			.VertexBuffer = vertexBuffer->GetGPUVirtualAddress(),
			.IndexBuffer = indexBuffer->GetGPUVirtualAddress()
		};

		m_HitGroupShaderTable.AddShaderRecord(shaderRecord);
	}

	UINT64 sbtSize = m_HitGroupShaderTable.GetSizeInBytes();

	GraphicsResource hitGroupUpload = RenderDevice.GraphicsMemory()->Allocate(sbtSize);

	m_HitGroupShaderTable.Generate(static_cast<BYTE*>(hitGroupUpload.Memory()));

	CommandList.TransitionBarrier(m_HitGroupSBT->pResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST);
	CommandList->CopyBufferRegion(m_HitGroupSBT->pResource.Get(), 0, hitGroupUpload.Resource(), hitGroupUpload.ResourceOffset(), hitGroupUpload.Size());
	CommandList.TransitionBarrier(m_HitGroupSBT->pResource.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
}

void PathIntegrator::Render(
	_In_ D3D12_GPU_VIRTUAL_ADDRESS SystemConstants,
	_In_ const RaytracingAccelerationStructure& RaytracingAccelerationStructure,
	_In_ D3D12_GPU_VIRTUAL_ADDRESS Materials,
	_In_ D3D12_GPU_VIRTUAL_ADDRESS Lights,
	_In_ CommandList& CommandList)
{
	auto& RenderDevice = RenderDevice::Instance();

	struct RenderPassData
	{
		uint MaxDepth;
		uint NumAccumulatedSamples;

		uint RenderTarget;
	} g_RenderPassData = {};

	g_RenderPassData.MaxDepth = Settings::MaxDepth;
	g_RenderPassData.NumAccumulatedSamples = Settings::NumAccumulatedSamples++;

	g_RenderPassData.RenderTarget = m_UAV.Index;

	GraphicsResource constantBuffer = RenderDevice.GraphicsMemory()->AllocateConstant(g_RenderPassData);

	CommandList->SetPipelineState1(m_RTPSO);
	CommandList->SetComputeRootSignature(m_GlobalRS);
	CommandList->SetComputeRootConstantBufferView(0, SystemConstants);
	CommandList->SetComputeRootConstantBufferView(1, constantBuffer.GpuAddress());
	CommandList->SetComputeRootShaderResourceView(2, RaytracingAccelerationStructure);
	CommandList->SetComputeRootShaderResourceView(3, Materials);
	CommandList->SetComputeRootShaderResourceView(4, Lights);

	RenderDevice.BindComputeDescriptorTable(m_GlobalRS, CommandList);

	D3D12_DISPATCH_RAYS_DESC desc =
	{
		.RayGenerationShaderRecord = m_RayGenerationShaderTable,
		.MissShaderTable = m_MissShaderTable,
		.HitGroupTable = m_HitGroupShaderTable,
		.Width = m_Width,
		.Height = m_Height,
		.Depth = 1
	};

	CommandList.DispatchRays(&desc);
	CommandList.UAVBarrier(m_RenderTarget->pResource.Get());
}
