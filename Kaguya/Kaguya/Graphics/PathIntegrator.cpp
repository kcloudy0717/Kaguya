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

void PathIntegrator_DXR_1_0::Initialize()
{
	UAV = UnorderedAccessView(RenderCore::pAdapter->GetDevice());
	SRV = ShaderResourceView(RenderCore::pAdapter->GetDevice());

	GlobalRS = RenderCore::pAdapter->CreateRootSignature(
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

	LocalHitGroupRS = RenderCore::pAdapter->CreateRootSignature(
		[](RootSignatureBuilder& Builder)
		{
			Builder.Add32BitConstants<0, 1>(1); // RootConstants	b0 | space1

			Builder.AddShaderResourceView<0, 1>(); // VertexBuffer		t0 | space1
			Builder.AddShaderResourceView<1, 1>(); // IndexBuffer		t1 | space1

			Builder.SetAsLocalRootSignature();
		},
		false);

	RTPSO = RenderCore::pAdapter->CreateRaytracingPipelineState(
		[&](RaytracingPipelineStateBuilder& Builder)
		{
			Builder.AddLibrary(Libraries::PathTrace, { g_RayGeneration, g_Miss, g_ShadowMiss, g_ClosestHit });

			Builder.AddHitGroup(g_HitGroupExport, {}, g_ClosestHit, {});

			Builder.AddRootSignatureAssociation(LocalHitGroupRS, { g_HitGroupExport });

			Builder.SetGlobalRootSignature(GlobalRS);

			constexpr UINT PayloadSize = 12	  // p
										 + 4  // materialID
										 + 8  // uv
										 + 8  // Ng
										 + 8; // Ns
			Builder.SetRaytracingShaderConfig(PayloadSize, D3D12_BUILTIN_TRIANGLE_INTERSECTION_ATTRIBUTES);

			// +1 for Primary
			Builder.SetRaytracingPipelineConfig(1);
		});

	g_RayGenerationSID = RTPSO.GetShaderIdentifier(L"RayGeneration");
	g_MissSID		   = RTPSO.GetShaderIdentifier(L"Miss");
	g_ShadowMissSID	   = RTPSO.GetShaderIdentifier(L"ShadowMiss");
	g_DefaultSID	   = RTPSO.GetShaderIdentifier(L"Default");

	RayGenerationShaderTable = ShaderBindingTable.AddRayGenerationShaderTable<void>(1);
	RayGenerationShaderTable->AddShaderRecord(g_RayGenerationSID);

	MissShaderTable = ShaderBindingTable.AddMissShaderTable<void>(2);
	MissShaderTable->AddShaderRecord(g_MissSID);
	MissShaderTable->AddShaderRecord(g_ShadowMissSID);

	HitGroupShaderTable = ShaderBindingTable.AddHitGroupShaderTable<RootArgument>(World::InstanceLimit);

	ShaderBindingTable.Generate(RenderCore::pAdapter->GetDevice());
}

void PathIntegrator_DXR_1_0::SetResolution(UINT Width, UINT Height)
{
	if ((this->Width == Width && this->Height == Height))
	{
		return;
	}

	this->Width	 = Width;
	this->Height = Height;

	auto ResourceDesc	   = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R32G32B32A32_FLOAT, Width, Height);
	ResourceDesc.MipLevels = 1;
	ResourceDesc.Flags	   = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	RenderTarget = Texture(RenderCore::pAdapter->GetDevice(), ResourceDesc, {});
	RenderTarget.GetResource()->SetName(L"PathIntegrator Output");

	RenderTarget.CreateUnorderedAccessView(UAV);
	RenderTarget.CreateShaderResourceView(SRV);

	Reset();
}

void PathIntegrator_DXR_1_0::Reset()
{
	Settings::NumAccumulatedSamples = 0;
}

void PathIntegrator_DXR_1_0::UpdateShaderTable(
	const RaytracingAccelerationStructure& RaytracingAccelerationStructure,
	CommandContext&						   Context)
{
	HitGroupShaderTable->Reset();
	for (auto [i, MeshRenderer] : enumerate(RaytracingAccelerationStructure.MeshRenderers))
	{
		ID3D12Resource* VertexBuffer = MeshRenderer->pMeshFilter->Mesh->VertexResource.GetResource();
		ID3D12Resource* IndexBuffer	 = MeshRenderer->pMeshFilter->Mesh->IndexResource.GetResource();

		RaytracingShaderTable<RootArgument>::Record Record = {};
		Record.ShaderIdentifier							   = g_DefaultSID;
		Record.RootArguments							   = { .MaterialIndex = static_cast<UINT>(i),
								   .Padding		  = 0xDEADBEEF,
								   .VertexBuffer  = VertexBuffer->GetGPUVirtualAddress(),
								   .IndexBuffer	  = IndexBuffer->GetGPUVirtualAddress() };

		HitGroupShaderTable->AddShaderRecord(Record);
	}

	ShaderBindingTable.Write();
	ShaderBindingTable.CopyToGPU(Context);
}

void PathIntegrator_DXR_1_0::Render(
	const PathIntegratorState&			   State,
	D3D12_GPU_VIRTUAL_ADDRESS			   SystemConstants,
	const RaytracingAccelerationStructure& RaytracingAccelerationStructure,
	D3D12_GPU_VIRTUAL_ADDRESS			   Materials,
	D3D12_GPU_VIRTUAL_ADDRESS			   Lights,
	CommandContext&						   Context)
{
	D3D12ScopedEvent(Context, "Path Trace");

	struct RenderPassData
	{
		unsigned int MaxDepth;
		unsigned int NumAccumulatedSamples;

		unsigned int RenderTarget;

		float SkyIntensity;
	} g_RenderPassData = {};

	g_RenderPassData.MaxDepth			   = State.MaxDepth;
	g_RenderPassData.NumAccumulatedSamples = Settings::NumAccumulatedSamples++;
	g_RenderPassData.RenderTarget		   = UAV.GetIndex();
	g_RenderPassData.SkyIntensity		   = State.SkyIntensity;

	Allocation Allocation = Context.CpuConstantAllocator.Allocate(g_RenderPassData);

	Context.CommandListHandle.GetGraphicsCommandList4()->SetPipelineState1(RTPSO);
	Context->SetComputeRootSignature(GlobalRS);
	Context->SetComputeRootConstantBufferView(0, SystemConstants);
	Context->SetComputeRootConstantBufferView(1, Allocation.GPUVirtualAddress);
	Context->SetComputeRootShaderResourceView(2, RaytracingAccelerationStructure);
	Context->SetComputeRootShaderResourceView(3, Materials);
	Context->SetComputeRootShaderResourceView(4, Lights);

	RenderCore::pAdapter->BindComputeDescriptorTable(GlobalRS, Context);

	D3D12_DISPATCH_RAYS_DESC Desc = ShaderBindingTable.GetDispatchRaysDesc(0, 0);
	Desc.Width					  = Width;
	Desc.Height					  = Height;
	Desc.Depth					  = 1;

	Context.DispatchRays(&Desc);
	Context.UAVBarrier(nullptr);
}
