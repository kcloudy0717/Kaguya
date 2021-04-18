#include "pch.h"
#include "RootSignature.h"
#include "RootSignatureBuilder.h"

//----------------------------------------------------------------------------------------------------
RootParameter::RootParameter(Type Type)
{
	switch (Type)
	{
	case RootParameter::Type::DescriptorTable:	m_RootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; break;
	case RootParameter::Type::Constants:		m_RootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS; break;
	case RootParameter::Type::CBV:				m_RootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; break;
	case RootParameter::Type::SRV:				m_RootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV; break;
	case RootParameter::Type::UAV:				m_RootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV; break;
	}
	m_RootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
}

//----------------------------------------------------------------------------------------------------
RootDescriptorTable::RootDescriptorTable()
	: RootParameter(RootParameter::Type::DescriptorTable)
{

}

void RootDescriptorTable::AddDescriptorRange(DescriptorRange::Type Type, const DescriptorRange& DescriptorRange)
{
	D3D12_DESCRIPTOR_RANGE1 Range1 = {};
	switch (Type)
	{
	case DescriptorRange::Type::SRV:		Range1.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; break;
	case DescriptorRange::Type::UAV:		Range1.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV; break;
	case DescriptorRange::Type::CBV:		Range1.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV; break;
	case DescriptorRange::Type::Sampler:	Range1.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER; break;
	}
	Range1.NumDescriptors						= DescriptorRange.NumDescriptors;
	Range1.BaseShaderRegister					= DescriptorRange.BaseShaderRegister;
	Range1.RegisterSpace						= DescriptorRange.RegisterSpace;
	Range1.Flags								= DescriptorRange.Flags;
	Range1.OffsetInDescriptorsFromTableStart	= DescriptorRange.OffsetInDescriptorsFromTableStart;

	m_DescriptorRanges.push_back(Range1);

	// Revalidate pointers in case of reallocation of memory
	m_RootParameter.DescriptorTable.NumDescriptorRanges = m_DescriptorRanges.size();
	m_RootParameter.DescriptorTable.pDescriptorRanges	= m_DescriptorRanges.data();
}

//----------------------------------------------------------------------------------------------------
RootSignature::RootSignature(ID3D12Device* pDevice, RootSignatureBuilder& Builder)
{
	D3D12_VERSIONED_ROOT_SIGNATURE_DESC VersionedRootSignatureDesc	= {};
	VersionedRootSignatureDesc.Version								= D3D_ROOT_SIGNATURE_VERSION_1_1;
	VersionedRootSignatureDesc.Desc_1_1								= Builder.Build();

	NumParameters		= VersionedRootSignatureDesc.Desc_1_1.NumParameters;
	NumStaticSamplers	= VersionedRootSignatureDesc.Desc_1_1.NumStaticSamplers;

	// Serialize the root signature.
	Microsoft::WRL::ComPtr<ID3DBlob> pSerializedRootSignatureBlob;
	Microsoft::WRL::ComPtr<ID3DBlob> pErrorBlob;
	if (FAILED(::D3D12SerializeVersionedRootSignature(&VersionedRootSignatureDesc, &pSerializedRootSignatureBlob, &pErrorBlob)))
	{
		LOG_ERROR("{} Failed: {}", "D3D12SerializeVersionedRootSignature", (char*)pErrorBlob->GetBufferPointer());
		throw std::exception("Failed to serialize root signature");
	}

	// Create the root signature.
	ThrowIfFailed(pDevice->CreateRootSignature(0, pSerializedRootSignatureBlob->GetBufferPointer(),
		pSerializedRootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature)));
}