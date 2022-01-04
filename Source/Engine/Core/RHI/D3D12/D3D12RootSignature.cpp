#include "D3D12RootSignature.h"

RootSignatureDesc::RootSignatureDesc()
{
	// Worst case number of root parameters is 64
	// Preallocate all possible parameters to avoid cost for push back
	Parameters.reserve(D3D12_MAX_ROOT_COST);
	DescriptorTableIndices.reserve(D3D12_GLOBAL_ROOT_DESCRIPTOR_TABLE_LIMIT);
	DescriptorTables.reserve(D3D12_GLOBAL_ROOT_DESCRIPTOR_TABLE_LIMIT);
}

RootSignatureDesc& RootSignatureDesc::AddDescriptorTable(const D3D12DescriptorTable& DescriptorTable)
{
	// The descriptor table descriptor ranges require a pointer to the descriptor ranges. Since new
	// ranges can be dynamically added in the vector, we separately store the index of the range set.
	// The actual address will be solved when generating the actual root signature
	CD3DX12_ROOT_PARAMETER1& Parameter = Parameters.emplace_back();
	Parameter.InitAsDescriptorTable(DescriptorTable.size(), nullptr);
	DescriptorTableIndices.push_back(static_cast<UINT>(DescriptorTables.size()));
	DescriptorTables.push_back(DescriptorTable);
	return *this;
}

RootSignatureDesc& RootSignatureDesc::AllowInputLayout() noexcept
{
	Flags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	return *this;
}

RootSignatureDesc& RootSignatureDesc::DenyVSAccess() noexcept
{
	Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS;
	return *this;
}

RootSignatureDesc& RootSignatureDesc::DenyHSAccess() noexcept
{
	Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;
	return *this;
}

RootSignatureDesc& RootSignatureDesc::DenyDSAccess() noexcept
{
	Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;
	return *this;
}

RootSignatureDesc& RootSignatureDesc::DenyTessellationShaderAccess() noexcept
{
	DenyHSAccess();
	DenyDSAccess();
	return *this;
}

RootSignatureDesc& RootSignatureDesc::DenyGSAccess() noexcept
{
	Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
	return *this;
}

RootSignatureDesc& RootSignatureDesc::DenyPSAccess() noexcept
{
	Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;
	return *this;
}

RootSignatureDesc& RootSignatureDesc::AsLocalRootSignature() noexcept
{
	Flags |= D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
	return *this;
}

RootSignatureDesc& RootSignatureDesc::DenyASAccess() noexcept
{
	Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS;
	return *this;
}

RootSignatureDesc& RootSignatureDesc::DenyMSAccess() noexcept
{
	Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;
	return *this;
}

RootSignatureDesc& RootSignatureDesc::AllowResourceDescriptorHeapIndexing() noexcept
{
	Flags |= D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;
	return *this;
}

RootSignatureDesc& RootSignatureDesc::AllowSampleDescriptorHeapIndexing() noexcept
{
	Flags |= D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED;
	return *this;
}

bool RootSignatureDesc::IsLocal() const noexcept
{
	return Flags & D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
}

D3D12_ROOT_SIGNATURE_DESC1 RootSignatureDesc::Build() noexcept
{
	// Go through all the parameters, and set the actual addresses of the heap range descriptors based
	// on their indices in the range indices vector
	for (size_t i = 0; i < Parameters.size(); ++i)
	{
		if (Parameters[i].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
		{
			Parameters[i].DescriptorTable.NumDescriptorRanges = DescriptorTables[DescriptorTableIndices[i]].size();
			Parameters[i].DescriptorTable.pDescriptorRanges	  = DescriptorTables[DescriptorTableIndices[i]].data();
		}
	}

	D3D12_ROOT_SIGNATURE_DESC1 Desc = {};
	Desc.NumParameters				= static_cast<UINT>(Parameters.size());
	Desc.pParameters				= Parameters.data();
	Desc.NumStaticSamplers			= static_cast<UINT>(StaticSamplers.size());
	Desc.pStaticSamplers			= StaticSamplers.data();
	Desc.Flags						= Flags;
	return Desc;
}

RootSignatureDesc& RootSignatureDesc::AddParameter(D3D12_ROOT_PARAMETER1 Parameter)
{
	Parameters.emplace_back(Parameter);
	DescriptorTableIndices.emplace_back(0xDEADBEEF);
	return *this;
}

D3D12RootSignature::D3D12RootSignature(
	D3D12Device*	   Parent,
	RootSignatureDesc& Desc)
	: D3D12DeviceChild(Parent)
{
	D3D12_VERSIONED_ROOT_SIGNATURE_DESC ApiDesc = {};
	ApiDesc.Version								= D3D_ROOT_SIGNATURE_VERSION_1_1;
	ApiDesc.Desc_1_1							= Desc.Build();
	NumParameters								= ApiDesc.Desc_1_1.NumParameters;

	// Serialize the root signature
	Microsoft::WRL::ComPtr<ID3DBlob> SerializedRootSignatureBlob;
	Microsoft::WRL::ComPtr<ID3DBlob> ErrorBlob;
	HRESULT							 Result = D3D12SerializeVersionedRootSignature(
		 &ApiDesc,
		 &SerializedRootSignatureBlob,
		 &ErrorBlob);
	if (FAILED(Result))
	{
		assert(ErrorBlob);
		LOG_ERROR("{}", static_cast<const char*>(ErrorBlob->GetBufferPointer()));
	}
	VERIFY_D3D12_API(Result);

	// Create the root signature
	VERIFY_D3D12_API(Parent->GetD3D12Device()->CreateRootSignature(
		0,
		SerializedRootSignatureBlob->GetBufferPointer(),
		SerializedRootSignatureBlob->GetBufferSize(),
		IID_PPV_ARGS(&RootSignature)));
}
