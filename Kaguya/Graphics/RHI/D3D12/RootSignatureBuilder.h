#pragma once
#include <d3d12.h>
#include "d3dx12.h"
#include <vector>
#include "RootSignature.h"

/*
	The maximum size of a root signature is 64 DWORDs.

	This maximum size is chosen to prevent abuse of the root signature as a way of storing bulk data. Each entry in the root signature has a cost towards this 64 DWORD limit:

	Descriptor tables cost 1 DWORD each.
	Root constants cost 1 DWORD each, since they are 32-bit values.
	Root descriptors (64-bit GPU virtual addresses) cost 2 DWORDs each.
	Static samplers do not have any cost in the size of the root signature.

	Use a small a root signature as necessary, though balance this with the flexibility of a larger root signature.
	Arrange parameters in a large root signature so that the parameters most likely to change often, or if low access latency for a given parameter is important, occur first.
	If convenient, use root constants or root constant buffer views over putting constant buffer views in a descriptor heap.
*/
class RootSignatureBuilder
{
public:
	RootSignatureBuilder();

	/*
		Ensure the instance of RootSignatureBuilder alive until you create the actual RootSignature
	*/
	D3D12_ROOT_SIGNATURE_DESC1 Build();

	void AddRootDescriptorTableParameter(const RootDescriptorTable& RootDescriptorTable);
	template<typename T>
	void AddRootConstantsParameter(const RootConstants<T>& RootConstants)
	{
		static_assert(std::is_trivially_copyable<T>::value, "typename T must be trivially copyable!");
		static_assert(sizeof(T) % 4 == 0, "typename T must be 4 byte aligned");

		// Add the root parameter to the set of parameters,
		m_Parameters.push_back(RootConstants.GetD3DRootParameter());

		// and indicate that there will be no range
		// location to indicate since this parameter is not part of the heap
		m_DescriptorRangeIndices.push_back(~0);
	}
	template<>
	void AddRootConstantsParameter(const RootConstants<void>& RootConstants)
	{
		// Add the root parameter to the set of parameters,
		m_Parameters.push_back(RootConstants.GetD3DRootParameter());

		// and indicate that there will be no range
		// location to indicate since this parameter is not part of the heap
		m_DescriptorRangeIndices.push_back(~0);
	}

	void AddRootCBVParameter(const RootCBV& RootCBV);
	void AddRootSRVParameter(const RootSRV& RootSRV);
	void AddRootUAVParameter(const RootUAV& RootUAV);

	// Register Space: space0
	void AddStaticSampler
	(
		UINT ShaderRegister,
		D3D12_FILTER Filter,
		D3D12_TEXTURE_ADDRESS_MODE AddressUVW,
		UINT MaxAnisotropy,
		D3D12_COMPARISON_FUNC ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL,
		D3D12_STATIC_BORDER_COLOR BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE
	);

	void AllowInputLayout();
	void DenyVSAccess();
	void DenyHSAccess();
	void DenyDSAccess();
	void DenyTessellationShaderAccess();
	void DenyGSAccess();
	void DenyPSAccess();
	void AllowStreamOutput();
	void SetAsLocalRootSignature();
	void DenyASAccess();
	void DenyMSAccess();
private:
	std::vector<D3D12_ROOT_PARAMETER1>					m_Parameters;
	std::vector<D3D12_STATIC_SAMPLER_DESC>				m_StaticSamplers;
	D3D12_ROOT_SIGNATURE_FLAGS							m_Flags;

	std::vector<UINT>									m_DescriptorRangeIndices;
	std::vector<std::vector<D3D12_DESCRIPTOR_RANGE1>>	m_DescriptorRanges;
};