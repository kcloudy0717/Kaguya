#include "RaytracingPipelineState.h"

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
	for (const auto& library : Libraries)
	{
		auto pLibrarySubobject = Desc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();

		pLibrarySubobject->SetDXILLibrary(&library.Library);

		for (const auto& symbol : library.Symbols)
		{
			pLibrarySubobject->DefineExport(symbol.data());
		}
	}

	// Add all the hit group declarations
	for (const auto& hitGroup : HitGroups)
	{
		auto pHitgroupSubobject = Desc.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();

		pHitgroupSubobject->SetHitGroupExport(hitGroup.HitGroupName.data());
		pHitgroupSubobject->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);

		LPCWSTR pAnyHitSymbolImport = hitGroup.AnyHitSymbol.empty() ? nullptr : hitGroup.AnyHitSymbol.data();
		pHitgroupSubobject->SetAnyHitShaderImport(pAnyHitSymbolImport);

		LPCWSTR pClosestHitShaderImport =
			hitGroup.ClosestHitSymbol.empty() ? nullptr : hitGroup.ClosestHitSymbol.data();
		pHitgroupSubobject->SetClosestHitShaderImport(pClosestHitShaderImport);

		LPCWSTR pIntersectionShaderImport =
			hitGroup.IntersectionSymbol.empty() ? nullptr : hitGroup.IntersectionSymbol.data();
		pHitgroupSubobject->SetIntersectionShaderImport(pIntersectionShaderImport);
	}

	// Add 2 subobjects for every root signature association
	for (auto& rootSignatureAssociation : RootSignatureAssociations)
	{
		// The root signature association requires two objects for each: one to declare the root
		// signature, and another to associate that root signature to a set of symbols

		// Add a subobject to declare the local root signature
		auto pLocalRootSignatureSubobject = Desc.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();

		pLocalRootSignatureSubobject->SetRootSignature(rootSignatureAssociation.pRootSignature);

		// Add a subobject for the association between the exported shader symbols and the local root signature
		auto pAssociationSubobject = Desc.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();

		for (const auto& symbols : rootSignatureAssociation.Symbols)
		{
			pAssociationSubobject->AddExport(symbols.data());
		}
		pAssociationSubobject->SetSubobjectToAssociate(*pLocalRootSignatureSubobject);
	}

	// Add a subobject for global root signature
	auto pGlobalRootSignatureSubobject = Desc.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
	pGlobalRootSignatureSubobject->SetRootSignature(GlobalRootSignature);

	// Add a subobject for the raytracing shader config
	auto pShaderConfigSubobject = Desc.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
	pShaderConfigSubobject->Config(ShaderConfig.MaxPayloadSizeInBytes, ShaderConfig.MaxAttributeSizeInBytes);

	// Add a subobject for the association between shaders and the payload
	std::vector<std::wstring> exportedSymbols = BuildShaderExportList();
	auto pAssociationSubobject = Desc.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();

	for (const auto& symbols : exportedSymbols)
	{
		pAssociationSubobject->AddExport(symbols.data());
	}
	pAssociationSubobject->SetSubobjectToAssociate(*pShaderConfigSubobject);

	// Add a subobject for the raytracing pipeline configuration
	auto pPipelineConfigSubobject = Desc.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
	pPipelineConfigSubobject->Config(PipelineConfig.MaxTraceRecursionDepth);

	return Desc;
}

void RaytracingPipelineStateBuilder::AddLibrary(
	const D3D12_SHADER_BYTECODE&	 Library,
	const std::vector<std::wstring>& Symbols)
{
	// Add a DXIL library to the pipeline. Note that this library has to be
	// compiled with dxc, using a lib_6_3 target. The exported symbols must correspond exactly to the
	// names of the shaders declared in the library, although unused ones can be omitted.
	Libraries.emplace_back(DXILLibrary(Library, Symbols));
}

void RaytracingPipelineStateBuilder::AddHitGroup(
	std::optional<std::wstring_view> pHitGroupName,
	std::optional<std::wstring_view> pAnyHitSymbol,
	std::optional<std::wstring_view> pClosestHitSymbol,
	std::optional<std::wstring_view> pIntersectionSymbol)
{
	// In DXR the hit-related shaders are grouped into hit groups. Such shaders are:
	// - The intersection shader, which can be used to intersect custom geometry, and is called upon
	//   hitting the bounding box the the object. A default one exists to intersect triangles
	// - The any hit shader, called on each intersection, which can be used to perform early
	//   alpha-testing and allow the ray to continue if needed. Default is a pass-through.
	// - The closest hit shader, invoked on the hit point closest to the ray start.
	// The shaders in a hit group share the same root signature, and are only referred to by the
	// hit group name in other places of the program.
	HitGroups.emplace_back(HitGroup(pHitGroupName, pAnyHitSymbol, pClosestHitSymbol, pIntersectionSymbol));

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
	ID3D12RootSignature*			 pRootSignature,
	const std::vector<std::wstring>& Symbols)
{
	RootSignatureAssociations.emplace_back(RootSignatureAssociation(pRootSignature, Symbols));
}

void RaytracingPipelineStateBuilder::SetGlobalRootSignature(ID3D12RootSignature* pGlobalRootSignature)
{
	GlobalRootSignature = pGlobalRootSignature;
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
	std::unordered_set<std::wstring> exports;

	// Add all the symbols exported by the libraries
	for (const auto& library : Libraries)
	{
		for (const auto& symbol : library.Symbols)
		{
#ifdef _DEBUG
			// Sanity check in debug mode: check that no name is exported more than once
			if (exports.find(symbol) != exports.end())
			{
				throw std::logic_error("Multiple definition of a symbol in the imported DXIL libraries");
			}
#endif
			exports.insert(symbol);
		}
	}

#ifdef _DEBUG
	// Sanity check in debug mode: verify that the hit groups do not reference an unknown shader name
	{
		std::unordered_set<std::wstring> all_exports = exports;

		for (const auto& hitGroup : HitGroups)
		{
			if (!hitGroup.AnyHitSymbol.empty() && exports.find(hitGroup.AnyHitSymbol) == exports.end())
			{
				throw std::logic_error("Any hit symbol not found in the imported DXIL libraries");
			}

			if (!hitGroup.ClosestHitSymbol.empty() && exports.find(hitGroup.ClosestHitSymbol) == exports.end())
			{
				throw std::logic_error("Closest hit symbol not found in the imported DXIL libraries");
			}

			if (!hitGroup.IntersectionSymbol.empty() && exports.find(hitGroup.IntersectionSymbol) == exports.end())
			{
				throw std::logic_error("Intersection symbol not found in the imported DXIL libraries");
			}

			all_exports.insert(hitGroup.HitGroupName);
		}

		// Sanity check in debug mode: verify that the root signature associations do not reference an
		// unknown shader or hit group name
		for (const auto& rootSignatureAssociation : RootSignatureAssociations)
		{
			for (const auto& symbol : rootSignatureAssociation.Symbols)
			{
				if (!symbol.empty() && all_exports.find(symbol) == all_exports.end())
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
	for (const auto& hitGroup : HitGroups)
	{
		if (!hitGroup.ClosestHitSymbol.empty())
		{
			exports.erase(hitGroup.ClosestHitSymbol);
		}

		if (!hitGroup.AnyHitSymbol.empty())
		{
			exports.erase(hitGroup.AnyHitSymbol);
		}

		if (!hitGroup.IntersectionSymbol.empty())
		{
			exports.erase(hitGroup.IntersectionSymbol);
		}

		exports.insert(hitGroup.HitGroupName);
	}

	// Finally build a vector containing ray generation and miss shaders, plus the hit group names
	std::vector<std::wstring> exportedSymbols;
	for (const auto& name : exports)
	{
		exportedSymbols.push_back(name);
	}

	return exportedSymbols;
}

RaytracingPipelineState::RaytracingPipelineState(ID3D12Device5* pDevice, RaytracingPipelineStateBuilder& Builder)
{
	D3D12_STATE_OBJECT_DESC Desc = Builder.Build();

	ASSERTD3D12APISUCCEEDED(pDevice->CreateStateObject(&Desc, IID_PPV_ARGS(StateObject.ReleaseAndGetAddressOf())));
	// Query the state object properties
	ASSERTD3D12APISUCCEEDED(StateObject.As(&StateObjectProperties));
}

void* RaytracingPipelineState::GetShaderIdentifier(std::wstring_view pExportName)
{
	void* pShaderIdentifier = StateObjectProperties->GetShaderIdentifier(pExportName.data());
	if (!pShaderIdentifier)
	{
		throw std::invalid_argument("Invalid pExportName");
	}

	return pShaderIdentifier;
}
