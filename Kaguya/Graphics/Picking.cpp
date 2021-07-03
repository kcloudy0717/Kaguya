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

	GlobalRS = RenderDevice.CreateRootSignature(
		[](RootSignatureBuilder& Builder)
		{
			Builder.AddConstantBufferView<0, 0>();	// g_SystemConstants		b0 | space0
			Builder.AddShaderResourceView<0, 0>();	// Scene					t0 | space0
			Builder.AddUnorderedAccessView<0, 0>(); // PickingResult,			u0 | space0
		});

	RTPSO = RenderDevice.CreateRaytracingPipelineState(
		[&](RaytracingPipelineStateBuilder& Builder)
		{
			Builder.AddLibrary(Libraries::Picking, { g_RayGeneration, g_Miss, g_ClosestHit });

			Builder.AddHitGroup(g_HitGroupExport, {}, g_ClosestHit, {});

			Builder.SetGlobalRootSignature(GlobalRS);

			Builder.SetRaytracingShaderConfig(sizeof(int), SizeOfBuiltInTriangleIntersectionAttributes);
			Builder.SetRaytracingPipelineConfig(1);
		});

	g_RayGenerationSID = RTPSO.GetShaderIdentifier(L"RayGeneration");
	g_MissSID		   = RTPSO.GetShaderIdentifier(L"Miss");
	g_DefaultSID	   = RTPSO.GetShaderIdentifier(L"Default");

	RayGenerationShaderTable = ShaderBindingTable.AddRayGenerationShaderTable<void>(1);
	RayGenerationShaderTable->AddShaderRecord(g_RayGenerationSID);

	MissShaderTable = ShaderBindingTable.AddMissShaderTable<void>(1);
	MissShaderTable->AddShaderRecord(g_MissSID);

	HitGroupShaderTable = ShaderBindingTable.AddHitGroupShaderTable<void>(Scene::InstanceLimit);

	ShaderBindingTable.Generate(RenderDevice.GetDevice());

	Result = Buffer(
		RenderDevice.GetDevice(),
		sizeof(int),
		sizeof(int),
		D3D12_HEAP_TYPE_DEFAULT,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	Readback =
		Buffer(RenderDevice.GetDevice(), sizeof(int), sizeof(int), D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_FLAG_NONE);

	Entities.reserve(Scene::InstanceLimit);
}

void Picking::UpdateShaderTable(
	const RaytracingAccelerationStructure& RaytracingAccelerationStructure,
	CommandContext&						   Context)
{
	HitGroupShaderTable->Reset();
	Entities.clear();
	for (auto [i, meshRenderer] : enumerate(RaytracingAccelerationStructure.MeshRenderers))
	{
		HitGroupShaderTable->AddShaderRecord(g_DefaultSID);

		Entities.emplace_back(meshRenderer->Handle, meshRenderer->pScene);
	}

	ShaderBindingTable.Write();
	ShaderBindingTable.CopyToGPU(Context);
}

void Picking::ShootPickingRay(
	D3D12_GPU_VIRTUAL_ADDRESS			   SystemConstants,
	const RaytracingAccelerationStructure& Scene,
	CommandContext&						   Context)
{
	Context.CommandListHandle.GetGraphicsCommandList4()->SetPipelineState1(RTPSO);
	Context->SetComputeRootSignature(GlobalRS);
	Context->SetComputeRootConstantBufferView(0, SystemConstants);
	Context->SetComputeRootShaderResourceView(1, Scene);
	Context->SetComputeRootUnorderedAccessView(2, Result.GetGPUVirtualAddress());

	D3D12_DISPATCH_RAYS_DESC Desc = ShaderBindingTable.GetDispatchRaysDesc(0, 0);
	Desc.Width					  = 1;
	Desc.Height					  = 1;
	Desc.Depth					  = 1;

	Context.DispatchRays(&Desc);

	Context->CopyBufferRegion(Readback.GetResource(), 0, Result.GetResource(), 0, sizeof(int));
}

std::optional<Entity> Picking::GetSelectedEntity()
{
	INT	 InstanceID		   = -1;
	INT* CPUVirtualAddress = nullptr;
	if (SUCCEEDED(Readback.GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&CPUVirtualAddress))))
	{
		InstanceID = CPUVirtualAddress[0];
		Readback.GetResource()->Unmap(0, nullptr);
	}

	if (InstanceID != -1 && InstanceID < Entities.size())
	{
		return Entities[InstanceID];
	}

	return {};
}
