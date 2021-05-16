#pragma once
#include "d3dx12.h"

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

/*
* Describes ranges of descriptors that might have offsets relative to the initial range descriptor
* I use this to emulate bindless resource, SM6.6 provides global variables for DescriptorHeaps
* that you can directly index into
*/
class DescriptorTable
{
public:
	DescriptorTable() noexcept = default;

	template<UINT BaseShaderRegister, UINT RegisterSpace>
	void AddDescriptorRange(
		_In_ D3D12_DESCRIPTOR_RANGE_TYPE Type,
		_In_ UINT NumDescriptors,
		_In_ D3D12_DESCRIPTOR_RANGE_FLAGS Flags,
		_In_ UINT OffsetInDescriptorsFromTableStart)
	{
		CD3DX12_DESCRIPTOR_RANGE1 range = {};
		range.Init(Type, NumDescriptors, BaseShaderRegister, RegisterSpace, Flags, OffsetInDescriptorsFromTableStart);

		m_DescriptorRanges.emplace_back(range);
	}

	auto GetDescriptorRanges() const noexcept { return m_DescriptorRanges; }
	size_t size() const noexcept { return m_DescriptorRanges.size(); }

private:
	std::vector<D3D12_DESCRIPTOR_RANGE1> m_DescriptorRanges;
};

class RootSignatureBuilder
{
public:
	RootSignatureBuilder() noexcept;

	D3D12_ROOT_SIGNATURE_DESC1 Build() noexcept;

	void AddDescriptorTable(
		_In_ const DescriptorTable& DescriptorTable)
	{
		CD3DX12_ROOT_PARAMETER1 parameter = {};
		parameter.InitAsDescriptorTable(DescriptorTable.size(), DescriptorTable.GetDescriptorRanges().data());

		// Add the root parameter to the set of parameters,
		m_Parameters.push_back(parameter);
		// The descriptor table descriptor ranges require a pointer to the descriptor ranges. Since new
		// ranges can be dynamically added in the vector, we separately store the index of the range set.
		// The actual address will be solved when generating the actual root signature
		m_DescriptorRangeIndices.push_back(m_DescriptorRanges.size());
		m_DescriptorRanges.push_back(DescriptorTable.GetDescriptorRanges());
	}

	template<UINT ShaderRegister, UINT RegisterSpace>
	void Add32BitConstants(
		_In_ UINT Num32BitValues)
	{
		CD3DX12_ROOT_PARAMETER1 parameter = {};
		parameter.InitAsConstants(Num32BitValues, ShaderRegister, RegisterSpace);

		AddParameter(parameter);
	}

	template<UINT ShaderRegister, UINT RegisterSpace>
	void AddConstantBufferView(
		_In_ D3D12_ROOT_DESCRIPTOR_FLAGS Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE)
	{
		CD3DX12_ROOT_PARAMETER1 parameter = {};
		parameter.InitAsConstantBufferView(ShaderRegister, RegisterSpace, Flags);

		AddParameter(parameter);
	}

	template<UINT ShaderRegister, UINT RegisterSpace>
	void AddShaderResourceView(
		_In_ D3D12_ROOT_DESCRIPTOR_FLAGS Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE)
	{
		CD3DX12_ROOT_PARAMETER1 parameter = {};
		parameter.InitAsShaderResourceView(ShaderRegister, RegisterSpace, Flags);

		AddParameter(parameter);
	}

	template<UINT ShaderRegister, UINT RegisterSpace>
	void AddUnorderedAccessView(
		_In_ D3D12_ROOT_DESCRIPTOR_FLAGS Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE)
	{
		CD3DX12_ROOT_PARAMETER1 parameter = {};
		parameter.InitAsUnorderedAccessView(ShaderRegister, RegisterSpace, Flags);

		AddParameter(parameter);
	}

	template<UINT ShaderRegister, UINT RegisterSpace>
	void AddStaticSampler(
		_In_ D3D12_FILTER Filter,
		_In_ D3D12_TEXTURE_ADDRESS_MODE AddressUVW,
		_In_ UINT MaxAnisotropy,
		_In_ D3D12_COMPARISON_FUNC ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL,
		_In_ D3D12_STATIC_BORDER_COLOR BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE)
	{
		CD3DX12_STATIC_SAMPLER_DESC desc = {};
		desc.Init(ShaderRegister, Filter, AddressUVW, AddressUVW, AddressUVW, 0.0f, MaxAnisotropy, ComparisonFunc, BorderColor);
		desc.RegisterSpace = RegisterSpace;
		m_StaticSamplers.push_back(desc);
	}

	void AllowInputLayout() noexcept;
	void DenyVSAccess() noexcept;
	void DenyHSAccess() noexcept;
	void DenyDSAccess() noexcept;
	void DenyTessellationShaderAccess() noexcept;
	void DenyGSAccess() noexcept;
	void DenyPSAccess() noexcept;
	void AllowStreamOutput() noexcept;
	void SetAsLocalRootSignature() noexcept;
	void DenyASAccess() noexcept;
	void DenyMSAccess() noexcept;
	void AllowResourceDescriptorHeapIndexing() noexcept;
	void AllowSampleDescriptorHeapIndexing() noexcept;
private:
	void AddParameter(
		_In_ D3D12_ROOT_PARAMETER1 Parameter)
	{
		// Add the root parameter to the set of parameters,
		m_Parameters.emplace_back(Parameter);
		// and indicate that there will be no range
		// location to indicate since this parameter is not part of the heap
		m_DescriptorRangeIndices.emplace_back(~0);
	}

private:
	std::vector<D3D12_ROOT_PARAMETER1> m_Parameters;
	std::vector<D3D12_STATIC_SAMPLER_DESC> m_StaticSamplers;
	D3D12_ROOT_SIGNATURE_FLAGS m_Flags;

	std::vector<UINT> m_DescriptorRangeIndices;
	std::vector<std::vector<D3D12_DESCRIPTOR_RANGE1>> m_DescriptorRanges;
};

class RootSignature
{
public:
	RootSignature() noexcept = default;
	RootSignature(
		_In_ ID3D12Device* pDevice,
		_In_ RootSignatureBuilder& Builder);

	RootSignature(RootSignature&&) noexcept = default;
	RootSignature& operator=(RootSignature&&) noexcept = default;

	RootSignature(const RootSignature&) = delete;
	RootSignature& operator=(const RootSignature&) = delete;

	operator ID3D12RootSignature* () const { return m_RootSignature.Get(); }
	ID3D12RootSignature* operator->() const { return m_RootSignature.Get(); }

	D3D12_ROOT_SIGNATURE_DESC1 GetDesc() const noexcept { return m_Desc.Desc_1_1; }

private:
	Microsoft::WRL::ComPtr<ID3D12RootSignature>	m_RootSignature;
	D3D12_VERSIONED_ROOT_SIGNATURE_DESC m_Desc = {};
};
