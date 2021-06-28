#include "pch.h"
#include "RootSignature.h"

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

RootSignature::RootSignature(_In_ ID3D12Device* pDevice, _In_ RootSignatureBuilder& Builder)
{
	Desc = { .Version = D3D_ROOT_SIGNATURE_VERSION_1_1, .Desc_1_1 = Builder.Build() };

	// Serialize the root signature
	Microsoft::WRL::ComPtr<ID3DBlob> SerializedRootSignatureBlob;
	Microsoft::WRL::ComPtr<ID3DBlob> ErrorBlob;
	ThrowIfFailed(::D3D12SerializeVersionedRootSignature(
		&Desc,
		SerializedRootSignatureBlob.ReleaseAndGetAddressOf(),
		ErrorBlob.ReleaseAndGetAddressOf()));

	// Create the root signature
	ThrowIfFailed(pDevice->CreateRootSignature(
		0,
		SerializedRootSignatureBlob->GetBufferPointer(),
		SerializedRootSignatureBlob->GetBufferSize(),
		IID_PPV_ARGS(pRootSignature.ReleaseAndGetAddressOf())));
}
