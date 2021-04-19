#include "pch.h"
#include "RootSignature.h"

RootSignatureBuilder::RootSignatureBuilder() noexcept
{
	m_Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
}

D3D12_ROOT_SIGNATURE_DESC1 RootSignatureBuilder::Build() noexcept
{
	// Go through all the parameters, and set the actual addresses of the heap range descriptors based
	// on their indices in the range indices vector
	for (size_t i = 0; i < m_Parameters.size(); ++i)
	{
		if (m_Parameters[i].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
		{
			m_Parameters[i].DescriptorTable.pDescriptorRanges = m_DescriptorRanges[m_DescriptorRangeIndices[i]].data();
		}
	}

	D3D12_ROOT_SIGNATURE_DESC1 desc = {};
	desc.NumParameters = static_cast<UINT>(m_Parameters.size());
	desc.pParameters = m_Parameters.data();
	desc.NumStaticSamplers = static_cast<UINT>(m_StaticSamplers.size());
	desc.pStaticSamplers = m_StaticSamplers.data();
	desc.Flags = m_Flags;
	return desc;
}

void RootSignatureBuilder::AllowInputLayout() noexcept
{
	m_Flags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
}

void RootSignatureBuilder::DenyVSAccess() noexcept
{
	m_Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS;
}

void RootSignatureBuilder::DenyHSAccess() noexcept
{
	m_Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;
}

void RootSignatureBuilder::DenyDSAccess() noexcept
{
	m_Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;
}

void RootSignatureBuilder::DenyTessellationShaderAccess() noexcept
{
	DenyHSAccess();
	DenyDSAccess();
}

void RootSignatureBuilder::DenyGSAccess() noexcept
{
	m_Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
}

void RootSignatureBuilder::DenyPSAccess() noexcept
{
	m_Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;
}

void RootSignatureBuilder::AllowStreamOutput() noexcept
{
	m_Flags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT;
}

void RootSignatureBuilder::SetAsLocalRootSignature() noexcept
{
	m_Flags |= D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
}

void RootSignatureBuilder::DenyASAccess() noexcept
{
	m_Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS;
}

void RootSignatureBuilder::DenyMSAccess() noexcept
{
	m_Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;
}

RootSignature::RootSignature(
	_In_ ID3D12Device* pDevice,
	_In_ RootSignatureBuilder& Builder)
{
	m_Desc = {
		.Version = D3D_ROOT_SIGNATURE_VERSION_1_1,
		.Desc_1_1 = Builder.Build()
	};

	// Serialize the root signature.
	Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSignatureBlob;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
	ThrowIfFailed(::D3D12SerializeVersionedRootSignature(
		&m_Desc,
		&serializedRootSignatureBlob,
		&errorBlob));

	// Create the root signature.
	ThrowIfFailed(pDevice->CreateRootSignature(
		0,
		serializedRootSignatureBlob->GetBufferPointer(),
		serializedRootSignatureBlob->GetBufferSize(),
		IID_PPV_ARGS(&m_RootSignature)));
}
