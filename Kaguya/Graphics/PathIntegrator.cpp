#include "pch.h"
#include "PathIntegrator.h"

#include "RendererRegistry.h"

namespace
{
// Symbols
constexpr LPCWSTR g_RayGeneration = L"RayGeneration";
constexpr LPCWSTR g_Miss		  = L"Miss";
constexpr LPCWSTR g_ShadowMiss	  = L"ShadowMiss";
constexpr LPCWSTR g_ClosestHit	  = L"ClosestHit";

// HitGroup Exports
constexpr LPCWSTR g_HitGroupExport = L"Default";

ShaderIdentifier g_RayGenerationSID;
ShaderIdentifier g_MissSID;
ShaderIdentifier g_ShadowMissSID;
ShaderIdentifier g_DefaultSID;
} // namespace

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

PathIntegrator::PathIntegrator(_In_ RenderDevice& RenderDevice)
{
	Settings::RestoreDefaults();

	m_GlobalRS = RenderDevice.CreateRootSignature(
		[](RootSignatureBuilder& Builder)
		{
			Builder.AddConstantBufferView<0, 0>(); // g_SystemConstants		b0 | space0
			Builder.AddConstantBufferView<1, 0>(); // g_RenderPassData		b1 | space0

			Builder.AddShaderResourceView<0, 0>(); // g_Scene				t0 | space0
			Builder.AddShaderResourceView<1, 0>(); // g_Materials			t1 | space0
			Builder.AddShaderResourceView<2, 0>(); // g_Lights				t2 | space0

			Builder.AllowResourceDescriptorHeapIndexing();
			Builder.AllowSampleDescriptorHeapIndexing();

			Builder.AddStaticSampler<0, 0>(
				D3D12_FILTER_MIN_MAG_MIP_POINT,
				D3D12_TEXTURE_ADDRESS_MODE_WRAP,
				16); // g_SamplerPointWrap			s0 | space0;
			Builder.AddStaticSampler<1, 0>(
				D3D12_FILTER_MIN_MAG_MIP_POINT,
				D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
				16); // g_SamplerPointClamp			s1 | space0;
			Builder.AddStaticSampler<2, 0>(
				D3D12_FILTER_MIN_MAG_MIP_LINEAR,
				D3D12_TEXTURE_ADDRESS_MODE_WRAP,
				16); // g_SamplerLinearWrap			s2 | space0;
			Builder.AddStaticSampler<3, 0>(
				D3D12_FILTER_MIN_MAG_MIP_LINEAR,
				D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
				16); // g_SamplerLinearClamp			s3 | space0;
			Builder.AddStaticSampler<4, 0>(
				D3D12_FILTER_ANISOTROPIC,
				D3D12_TEXTURE_ADDRESS_MODE_WRAP,
				16); // g_SamplerAnisotropicWrap		s4 | space0;
			Builder.AddStaticSampler<5, 0>(
				D3D12_FILTER_ANISOTROPIC,
				D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
				16); // g_SamplerAnisotropicClamp	s5 | space0;
		});

	m_LocalHitGroupRS = RenderDevice::Instance().CreateRootSignature(
		[](RootSignatureBuilder& Builder)
		{
			Builder.Add32BitConstants<0, 1>(1); // RootConstants	b0 | space1

			Builder.AddShaderResourceView<0, 1>(); // VertexBuffer		t0 | space1
			Builder.AddShaderResourceView<1, 1>(); // IndexBuffer		t1 | space1

			Builder.SetAsLocalRootSignature();
		},
		false);

	m_RTPSO = RenderDevice.CreateRaytracingPipelineState(
		[&](RaytracingPipelineStateBuilder& Builder)
		{
			Builder.AddLibrary(Libraries::PathTrace, { g_RayGeneration, g_Miss, g_ShadowMiss, g_ClosestHit });

			Builder.AddHitGroup(g_HitGroupExport, {}, g_ClosestHit, {});

			Builder.AddRootSignatureAssociation(m_LocalHitGroupRS, { g_HitGroupExport });

			Builder.SetGlobalRootSignature(m_GlobalRS);

			Builder.SetRaytracingShaderConfig(
				12 * sizeof(float) + 2 * sizeof(unsigned int),
				SizeOfBuiltInTriangleIntersectionAttributes);

			// +1 for Primary, +1 for Shadow
			Builder.SetRaytracingPipelineConfig(2);
		});

	g_RayGenerationSID = m_RTPSO.GetShaderIdentifier(L"RayGeneration");
	g_MissSID		   = m_RTPSO.GetShaderIdentifier(L"Miss");
	g_ShadowMissSID	   = m_RTPSO.GetShaderIdentifier(L"ShadowMiss");
	g_DefaultSID	   = m_RTPSO.GetShaderIdentifier(L"Default");

	CommandContext& CopyContext2 = RenderDevice.GetDevice()->GetCopyContext2();

	ResourceUploader uploader(RenderDevice.GetDevice());
	uploader.Begin();

	// TODO: Merge shader table resources into one resource, right now there are
	// memory wasted due to alignment
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
		m_MissShaderTable.AddShaderRecord(g_ShadowMissSID);

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
}

void PathIntegrator::SetResolution(_In_ UINT Width, _In_ UINT Height)
{
	auto& RenderDevice = RenderDevice::Instance();

	if ((m_Width == Width && m_Height == Height))
	{
		return;
	}

	m_Width	 = Width;
	m_Height = Height;

	auto ResourceDesc	   = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R32G32B32A32_FLOAT, Width, Height);
	ResourceDesc.MipLevels = 1;
	ResourceDesc.Flags	   = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	m_RenderTarget = Texture(RenderDevice.GetDevice(), ResourceDesc, {});

	Reset();
}

void PathIntegrator::Reset()
{
	Settings::NumAccumulatedSamples = 0;
}

void PathIntegrator::UpdateShaderTable(
	const RaytracingAccelerationStructure& RaytracingAccelerationStructure,
	CommandContext&						   Context)
{
	m_HitGroupShaderTable.clear();
	for (auto [i, meshRenderer] : enumerate(RaytracingAccelerationStructure.MeshRenderers))
	{
		auto meshFilter = meshRenderer->pMeshFilter;

		ID3D12Resource* vertexBuffer = meshFilter->Mesh->VertexResource.GetResource();
		ID3D12Resource* indexBuffer	 = meshFilter->Mesh->IndexResource.GetResource();

		RaytracingShaderTable<RootArgument>::Record shaderRecord = {};
		shaderRecord.ShaderIdentifier							 = g_DefaultSID;

		shaderRecord.RootArguments = { .MaterialIndex = (UINT)i,
									   .Padding		  = 0,
									   .VertexBuffer  = vertexBuffer->GetGPUVirtualAddress(),
									   .IndexBuffer	  = indexBuffer->GetGPUVirtualAddress() };

		m_HitGroupShaderTable.AddShaderRecord(shaderRecord);
	}

	UINT64 sbtSize = m_HitGroupShaderTable.GetSizeInBytes();

	Allocation Allocation = Context.CpuConstantAllocator.Allocate(sbtSize);

	m_HitGroupShaderTable.Generate(static_cast<BYTE*>(Allocation.CPUVirtualAddress));

	Context->CopyBufferRegion(m_HitGroupSBT.GetResource(), 0, Allocation.pResource, Allocation.Offset, Allocation.Size);
}

void PathIntegrator::Render(
	D3D12_GPU_VIRTUAL_ADDRESS			   SystemConstants,
	const RaytracingAccelerationStructure& RaytracingAccelerationStructure,
	D3D12_GPU_VIRTUAL_ADDRESS			   Materials,
	D3D12_GPU_VIRTUAL_ADDRESS			   Lights,
	CommandContext&						   Context)
{
	auto& RenderDevice = RenderDevice::Instance();

	struct RenderPassData
	{
		unsigned int MaxDepth;
		unsigned int NumAccumulatedSamples;

		unsigned int RenderTarget;
	} g_RenderPassData = {};

	g_RenderPassData.MaxDepth			   = Settings::MaxDepth;
	g_RenderPassData.NumAccumulatedSamples = Settings::NumAccumulatedSamples++;

	g_RenderPassData.RenderTarget = m_RenderTarget.UAV.GetIndex();

	Allocation Allocation = Context.CpuConstantAllocator.Allocate(sizeof(RenderPassData));
	std::memcpy(Allocation.CPUVirtualAddress, &g_RenderPassData, sizeof(RenderPassData));

	Context.CommandListHandle.GetGraphicsCommandList4()->SetPipelineState1(m_RTPSO);
	Context->SetComputeRootSignature(m_GlobalRS);
	Context->SetComputeRootConstantBufferView(0, SystemConstants);
	Context->SetComputeRootConstantBufferView(1, Allocation.GPUVirtualAddress);
	Context->SetComputeRootShaderResourceView(2, RaytracingAccelerationStructure);
	Context->SetComputeRootShaderResourceView(3, Materials);
	Context->SetComputeRootShaderResourceView(4, Lights);

	RenderDevice.BindComputeDescriptorTable(m_GlobalRS, Context);

	D3D12_DISPATCH_RAYS_DESC desc = { .RayGenerationShaderRecord = m_RayGenerationShaderTable,
									  .MissShaderTable			 = m_MissShaderTable,
									  .HitGroupTable			 = m_HitGroupShaderTable,
									  .Width					 = m_Width,
									  .Height					 = m_Height,
									  .Depth					 = 1 };

	Context.DispatchRays(&desc);
	Context.UAVBarrier(nullptr);
}
