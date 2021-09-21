#include "D3D12RaytracingPipelineState.h"

RaytracingPipelineStateBuilder::RaytracingPipelineStateBuilder() noexcept
	: Desc(D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE)
	, GlobalRootSignature(nullptr)
	, ShaderConfig()
	, PipelineConfig()
{
}

D3D12_STATE_OBJECT_DESC RaytracingPipelineStateBuilder::Build()
{
	// Add all the DXIL libraries
	for (const auto& [Library, Symbols] : Libraries)
	{
		auto DxilLibrarySubobject = Desc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
		DxilLibrarySubobject->SetDXILLibrary(&Library);
		for (const auto& Symbol : Symbols)
		{
			DxilLibrarySubobject->DefineExport(Symbol.data());
		}
	}

	// Add all the hit group declarations
	for (const auto& [HitGroupName, AnyHit, ClosestHit, Intersection] : HitGroups)
	{
		auto HitGroupSubobject = Desc.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
		HitGroupSubobject->SetHitGroupExport(HitGroupName.data());
		HitGroupSubobject->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);

		HitGroupSubobject->SetAnyHitShaderImport(!AnyHit.empty() ? AnyHit.data() : nullptr);
		HitGroupSubobject->SetClosestHitShaderImport(!ClosestHit.empty() ? ClosestHit.data() : nullptr);
		HitGroupSubobject->SetIntersectionShaderImport(!Intersection.empty() ? Intersection.data() : nullptr);
	}

	// Add 2 subobjects for every root signature association
	for (const auto& [RootSignature, Symbols] : RootSignatureAssociations)
	{
		// The root signature association requires two objects for each: one to declare the root
		// signature, and another to associate that root signature to a set of symbols

		// Add a subobject to declare the local root signature
		auto LocalRootSignatureSubobject = Desc.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
		LocalRootSignatureSubobject->SetRootSignature(RootSignature);

		// Add a subobject for the association between the exported shader symbols and the local root signature
		auto SubobjectToExportsAssociationSubobject =
			Desc.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
		for (const auto& Symbol : Symbols)
		{
			SubobjectToExportsAssociationSubobject->AddExport(Symbol.data());
		}
		SubobjectToExportsAssociationSubobject->SetSubobjectToAssociate(*LocalRootSignatureSubobject);
	}

	// Add a subobject for global root signature
	auto GlobalRootSignatureSubobject = Desc.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
	GlobalRootSignatureSubobject->SetRootSignature(GlobalRootSignature);

	// Add a subobject for the raytracing shader config
	auto RaytracingShaderConfigSubobject = Desc.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
	RaytracingShaderConfigSubobject->Config(ShaderConfig.MaxPayloadSizeInBytes, ShaderConfig.MaxAttributeSizeInBytes);

	// Add a subobject for the association between shaders and the payload
	std::vector<std::wstring> ExportedSymbols = BuildShaderExportList();
	auto					  SubobjectToExportsAssociationSubobject =
		Desc.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
	for (const auto& Symbol : ExportedSymbols)
	{
		SubobjectToExportsAssociationSubobject->AddExport(Symbol.data());
	}
	SubobjectToExportsAssociationSubobject->SetSubobjectToAssociate(*RaytracingShaderConfigSubobject);

	// Add a subobject for the raytracing pipeline configuration
	auto RaytracingPipelineConfigSubobject = Desc.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
	RaytracingPipelineConfigSubobject->Config(PipelineConfig.MaxTraceRecursionDepth);

	return Desc;
}

void RaytracingPipelineStateBuilder::AddLibrary(
	const D3D12_SHADER_BYTECODE&	 Library,
	const std::vector<std::wstring>& Symbols)
{
	// Add a DXIL library to the pipeline. The exported symbols must correspond exactly to the
	// names of the shaders declared in the library, although unused ones can be omitted.
	Libraries.emplace_back(DxilLibrary(Library, Symbols));
}

void RaytracingPipelineStateBuilder::AddHitGroup(
	std::wstring_view				 HitGroupName,
	std::optional<std::wstring_view> AnyHitSymbol,
	std::optional<std::wstring_view> ClosestHitSymbol,
	std::optional<std::wstring_view> IntersectionSymbol)
{
	// In DXR the hit-related shaders are grouped into hit groups. Such shaders are:
	// - The intersection shader, which can be used to intersect custom geometry, and is called upon
	//   hitting the bounding box the the object. A default one exists to intersect triangles
	// - The any hit shader, called on each intersection, which can be used to perform early
	//   alpha-testing and allow the ray to continue if needed. Default is a pass-through.
	// - The closest hit shader, invoked on the hit point closest to the ray start.
	// The shaders in a hit group share the same root signature, and are only referred to by the
	// hit group name in other places of the program.
	HitGroups.emplace_back(HitGroup(HitGroupName, AnyHitSymbol, ClosestHitSymbol, IntersectionSymbol));

	// 3 different shaders can be invoked to obtain an intersection: an
	// intersection shader is called when hitting the bounding box of non-triangular geometry.
	// An any-hit shader is called on potential intersections.
	// This shader can, for example, perform alpha-testing and
	// discard some intersections. Finally, the closest-hit program is invoked on
	// the intersection point closest to the ray origin. Those 3 shaders are bound
	// together into a hit group.

	// Note that for triangular geometry the intersection shader is built-in. An
	// empty any-hit shader is also defined by default, so in our simple case each
	// hit group contains only the closest hit shader. Note that since the
	// exported symbols are defined above the shaders can be simply referred to by
	// name.
}

void RaytracingPipelineStateBuilder::AddRootSignatureAssociation(
	ID3D12RootSignature*			 RootSignature,
	const std::vector<std::wstring>& Symbols)
{
	RootSignatureAssociations.emplace_back(RootSignatureAssociation(RootSignature, Symbols));
}

void RaytracingPipelineStateBuilder::SetGlobalRootSignature(ID3D12RootSignature* GlobalRootSignature)
{
	this->GlobalRootSignature = GlobalRootSignature;
}

void RaytracingPipelineStateBuilder::SetRaytracingShaderConfig(UINT MaxPayloadSizeInBytes, UINT MaxAttributeSizeInBytes)
{
	ShaderConfig.MaxPayloadSizeInBytes	 = MaxPayloadSizeInBytes;
	ShaderConfig.MaxAttributeSizeInBytes = MaxAttributeSizeInBytes;
}

void RaytracingPipelineStateBuilder::SetRaytracingPipelineConfig(UINT MaxTraceRecursionDepth)
{
	PipelineConfig.MaxTraceRecursionDepth = MaxTraceRecursionDepth;
}

std::vector<std::wstring> RaytracingPipelineStateBuilder::BuildShaderExportList()
{
	// Get all names from libraries
	// Get names associated to hit groups
	// Return list of libraries + hit group names - shaders in hit groups
	std::unordered_set<std::wstring> Exports;

	// Add all the symbols exported by the libraries
	for (const auto& Library : Libraries)
	{
		for (const auto& Symbol : Library.Symbols)
		{
#ifdef _DEBUG
			// Sanity check in debug mode: check that no name is exported more than once
			if (Exports.find(Symbol) != Exports.end())
			{
				throw std::logic_error("Multiple definition of a symbol in the imported DXIL libraries");
			}
#endif
			Exports.insert(Symbol);
		}
	}

#ifdef _DEBUG
	// Sanity check in debug mode: verify that the hit groups do not reference an unknown shader name
	{
		std::unordered_set<std::wstring> AllExports = Exports;

		for (const auto& HitGroup : HitGroups)
		{
			if (!HitGroup.AnyHitSymbol.empty() && Exports.find(HitGroup.AnyHitSymbol) == Exports.end())
			{
				throw std::logic_error("Any hit symbol not found in the imported DXIL libraries");
			}

			if (!HitGroup.ClosestHitSymbol.empty() && Exports.find(HitGroup.ClosestHitSymbol) == Exports.end())
			{
				throw std::logic_error("Closest hit symbol not found in the imported DXIL libraries");
			}

			if (!HitGroup.IntersectionSymbol.empty() && Exports.find(HitGroup.IntersectionSymbol) == Exports.end())
			{
				throw std::logic_error("Intersection symbol not found in the imported DXIL libraries");
			}

			AllExports.insert(HitGroup.HitGroupName);
		}

		// Sanity check in debug mode: verify that the root signature associations do not reference an
		// unknown shader or hit group name
		for (const auto& RootSignatureAssociation : RootSignatureAssociations)
		{
			for (const auto& Symbol : RootSignatureAssociation.Symbols)
			{
				if (!Symbol.empty() && AllExports.find(Symbol) == AllExports.end())
				{
					throw std::logic_error(
						"Root association symbol not found in the imported DXIL libraries and hit group names");
				}
			}
		}
	}
#endif

	// Go through all hit groups and remove the symbols corresponding to intersection, any hit and
	// closest hit shaders from the symbol set
	for (const auto& HitGroup : HitGroups)
	{
		if (!HitGroup.ClosestHitSymbol.empty())
		{
			Exports.erase(HitGroup.ClosestHitSymbol);
		}

		if (!HitGroup.AnyHitSymbol.empty())
		{
			Exports.erase(HitGroup.AnyHitSymbol);
		}

		if (!HitGroup.IntersectionSymbol.empty())
		{
			Exports.erase(HitGroup.IntersectionSymbol);
		}

		Exports.insert(HitGroup.HitGroupName);
	}

	return std::vector(Exports.begin(), Exports.end());
}

D3D12RaytracingPipelineState::D3D12RaytracingPipelineState(
	ID3D12Device5*					Device,
	RaytracingPipelineStateBuilder& Builder)
{
	D3D12_STATE_OBJECT_DESC Desc = Builder.Build();

	VERIFY_D3D12_API(Device->CreateStateObject(&Desc, IID_PPV_ARGS(StateObject.ReleaseAndGetAddressOf())));
	// Query state object properties
	VERIFY_D3D12_API(StateObject.As(&StateObjectProperties));
}

void* D3D12RaytracingPipelineState::GetShaderIdentifier(std::wstring_view ExportName) const
{
	void* ShaderIdentifier = StateObjectProperties->GetShaderIdentifier(ExportName.data());
	assert(ShaderIdentifier && "Invalid ExportName");
	return ShaderIdentifier;
}
