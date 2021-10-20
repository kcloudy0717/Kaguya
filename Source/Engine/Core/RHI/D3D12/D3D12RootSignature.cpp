#include "D3D12RootSignature.h"

D3D12_ROOT_SIGNATURE_DESC1 RootSignatureBuilder::Build() noexcept
{
	// Go through all the parameters, and set the actual addresses of the heap range descriptors based
	// on their indices in the range indices vector
	for (size_t i = 0; i < Parameters.size(); ++i)
	{
		if (Parameters[i].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
		{
			Parameters[i].DescriptorTable = DescriptorTables[DescriptorTableIndices[i]];
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

void RootSignatureBuilder::AllowInputLayout() noexcept
{
	Flags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
}

void RootSignatureBuilder::DenyVSAccess() noexcept
{
	Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS;
}

void RootSignatureBuilder::DenyHSAccess() noexcept
{
	Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;
}

void RootSignatureBuilder::DenyDSAccess() noexcept
{
	Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;
}

void RootSignatureBuilder::DenyTessellationShaderAccess() noexcept
{
	DenyHSAccess();
	DenyDSAccess();
}

void RootSignatureBuilder::DenyGSAccess() noexcept
{
	Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
}

void RootSignatureBuilder::DenyPSAccess() noexcept
{
	Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;
}

void RootSignatureBuilder::AllowStreamOutput() noexcept
{
	Flags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT;
}

void RootSignatureBuilder::SetAsLocalRootSignature() noexcept
{
	Flags |= D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
}

void RootSignatureBuilder::DenyASAccess() noexcept
{
	Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS;
}

void RootSignatureBuilder::DenyMSAccess() noexcept
{
	Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;
}

void RootSignatureBuilder::AllowResourceDescriptorHeapIndexing() noexcept
{
	Flags |= D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;
}

void RootSignatureBuilder::AllowSampleDescriptorHeapIndexing() noexcept
{
	Flags |= D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED;
}

D3D12RootSignature::D3D12RootSignature(D3D12Device* Parent, RootSignatureBuilder& Builder)
	: D3D12DeviceChild(Parent)
{
	Desc = { .Version = D3D_ROOT_SIGNATURE_VERSION_1_1, .Desc_1_1 = Builder.Build() };

	// Serialize the root signature
	Microsoft::WRL::ComPtr<ID3DBlob> SerializedRootSignatureBlob;
	Microsoft::WRL::ComPtr<ID3DBlob> ErrorBlob;
	VERIFY_D3D12_API(D3D12SerializeVersionedRootSignature(
		&Desc,
		SerializedRootSignatureBlob.ReleaseAndGetAddressOf(),
		ErrorBlob.ReleaseAndGetAddressOf()));

	// Create the root signature
	VERIFY_D3D12_API(Parent->GetD3D12Device()->CreateRootSignature(
		0,
		SerializedRootSignatureBlob->GetBufferPointer(),
		SerializedRootSignatureBlob->GetBufferSize(),
		IID_PPV_ARGS(RootSignature.ReleaseAndGetAddressOf())));

	for (UINT i = 0; i < Desc.Desc_1_1.NumParameters; ++i)
	{
		const D3D12_ROOT_PARAMETER1& RootParameter = Desc.Desc_1_1.pParameters[i];

		if (RootParameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
		{
			const D3D12_ROOT_DESCRIPTOR_TABLE1& DescriptorTable1 = RootParameter.DescriptorTable;

			// Don't care about the rest, just the first range is enough
			switch (DescriptorTable1.pDescriptorRanges[0].RangeType)
			{
			case D3D12_DESCRIPTOR_RANGE_TYPE_CBV:
			case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
			case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
				ResourceDescriptorTableBitMask.set(i, true);
				break;
			case D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER:
				SamplerTableBitMask.set(i, true);
				break;
			}

			// Calculate total number of descriptors in the descriptor table.
			for (UINT j = 0; j < DescriptorTable1.NumDescriptorRanges; ++j)
			{
				NumDescriptorsPerTable[i] += DescriptorTable1.pDescriptorRanges[j].NumDescriptors;
			}
		}
	}
}

std::bitset<D3D12_GLOBAL_ROOT_DESCRIPTOR_TABLE_LIMIT> D3D12RootSignature::GetDescriptorTableBitMask(
	D3D12_DESCRIPTOR_HEAP_TYPE Type) const noexcept
{
	switch (Type)
	{
	case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
		return ResourceDescriptorTableBitMask;
	case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
		return SamplerTableBitMask;
	default:
		return {};
	}
}

UINT D3D12RootSignature::GetNumDescriptors(UINT RootParameterIndex) const noexcept
{
	assert(RootParameterIndex < D3D12_GLOBAL_ROOT_DESCRIPTOR_TABLE_LIMIT);
	return NumDescriptorsPerTable[RootParameterIndex];
}
