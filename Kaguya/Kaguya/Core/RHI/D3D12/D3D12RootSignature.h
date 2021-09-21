#pragma once
#include "D3D12Common.h"

/*
	The maximum size of a root signature is 64 DWORDs.

	This maximum size is chosen to prevent abuse of the root signature as a way of storing bulk data. Each entry in the
	root signature has a cost towards this 64 DWORD limit:

	Descriptor tables cost 1 DWORD each.
	Root constants cost 1 DWORD each, since they are 32-bit values.
	Root descriptors (64-bit GPU virtual addresses) cost 2 DWORDs each.
	Static samplers do not have any cost in the size of the root signature.

	Use a small a root signature as necessary, though balance this with the flexibility of a larger root signature.
	Arrange parameters in a large root signature so that the parameters most likely to change often, or if low access
	latency for a given parameter is important, occur first. If convenient, use root constants or root constant buffer
	views over putting constant buffer views in a descriptor heap.
*/

/*
 * Describes ranges of descriptors that might have offsets relative to the initial range descriptor
 * I use this to emulate bindless resource, SM6.6 provides global variables for DescriptorHeaps
 * that you can directly index into
 */
class D3D12DescriptorTable
{
public:
	D3D12DescriptorTable() noexcept = default;

	template<UINT BaseShaderRegister, UINT RegisterSpace>
	void AddSRVRange(
		UINT						 NumDescriptors,
		D3D12_DESCRIPTOR_RANGE_FLAGS Flags,
		UINT						 OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND)
	{
		AddDescriptorRange<BaseShaderRegister, RegisterSpace>(
			D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
			NumDescriptors,
			Flags,
			OffsetInDescriptorsFromTableStart);
	}

	template<UINT BaseShaderRegister, UINT RegisterSpace>
	void AddUAVRange(
		UINT						 NumDescriptors,
		D3D12_DESCRIPTOR_RANGE_FLAGS Flags,
		UINT						 OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND)
	{
		AddDescriptorRange<BaseShaderRegister, RegisterSpace>(
			D3D12_DESCRIPTOR_RANGE_TYPE_UAV,
			NumDescriptors,
			Flags,
			OffsetInDescriptorsFromTableStart);
	}

	template<UINT BaseShaderRegister, UINT RegisterSpace>
	void AddCBVRange(
		UINT						 NumDescriptors,
		D3D12_DESCRIPTOR_RANGE_FLAGS Flags,
		UINT						 OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND)
	{
		AddDescriptorRange<BaseShaderRegister, RegisterSpace>(
			D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
			NumDescriptors,
			Flags,
			OffsetInDescriptorsFromTableStart);
	}

	template<UINT BaseShaderRegister, UINT RegisterSpace>
	void AddSamplerRange(
		UINT						 NumDescriptors,
		D3D12_DESCRIPTOR_RANGE_FLAGS Flags,
		UINT						 OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND)
	{
		AddDescriptorRange<BaseShaderRegister, RegisterSpace>(
			D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,
			NumDescriptors,
			Flags,
			OffsetInDescriptorsFromTableStart);
	}

	template<UINT BaseShaderRegister, UINT RegisterSpace>
	void AddDescriptorRange(
		D3D12_DESCRIPTOR_RANGE_TYPE	 Type,
		UINT						 NumDescriptors,
		D3D12_DESCRIPTOR_RANGE_FLAGS Flags,
		UINT						 OffsetInDescriptorsFromTableStart)
	{
		CD3DX12_DESCRIPTOR_RANGE1& Range = DescriptorRanges.emplace_back();
		Range.Init(Type, NumDescriptors, BaseShaderRegister, RegisterSpace, Flags, OffsetInDescriptorsFromTableStart);
	}

	operator D3D12_ROOT_DESCRIPTOR_TABLE1() const
	{
		return D3D12_ROOT_DESCRIPTOR_TABLE1{ .NumDescriptorRanges = static_cast<UINT>(DescriptorRanges.size()),
											 .pDescriptorRanges	  = DescriptorRanges.data() };
	}

	const auto& GetDescriptorRanges() const noexcept { return DescriptorRanges; }
	UINT		size() const noexcept { return static_cast<UINT>(DescriptorRanges.size()); }

private:
	std::vector<CD3DX12_DESCRIPTOR_RANGE1> DescriptorRanges;
};

class RootSignatureBuilder
{
public:
	D3D12_ROOT_SIGNATURE_DESC1 Build() noexcept;

	void AddDescriptorTable(const D3D12DescriptorTable& DescriptorTable)
	{
		CD3DX12_ROOT_PARAMETER1& Parameter = Parameters.emplace_back();
		Parameter.InitAsDescriptorTable(DescriptorTable.size(), DescriptorTable.GetDescriptorRanges().data());

		// The descriptor table descriptor ranges require a pointer to the descriptor ranges. Since new
		// ranges can be dynamically added in the vector, we separately store the index of the range set.
		// The actual address will be solved when generating the actual root signature
		DescriptorTableIndices.push_back(static_cast<UINT>(DescriptorTables.size()));
		DescriptorTables.push_back(DescriptorTable);
	}

	template<UINT ShaderRegister, UINT RegisterSpace, typename T>
	void Add32BitConstants()
	{
		static_assert(sizeof(T) % 4 == 0);
		CD3DX12_ROOT_PARAMETER1 Parameter = {};
		Parameter.InitAsConstants(sizeof(T) / 4, ShaderRegister, RegisterSpace);

		AddParameter(Parameter);
	}

	template<UINT ShaderRegister, UINT RegisterSpace>
	void Add32BitConstants(UINT Num32BitValues)
	{
		CD3DX12_ROOT_PARAMETER1 Parameter = {};
		Parameter.InitAsConstants(Num32BitValues, ShaderRegister, RegisterSpace);

		AddParameter(Parameter);
	}

	template<UINT ShaderRegister, UINT RegisterSpace>
	void AddConstantBufferView(D3D12_ROOT_DESCRIPTOR_FLAGS Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE)
	{
		CD3DX12_ROOT_PARAMETER1 Parameter = {};
		Parameter.InitAsConstantBufferView(ShaderRegister, RegisterSpace, Flags);

		AddParameter(Parameter);
	}

	template<UINT ShaderRegister, UINT RegisterSpace>
	void AddShaderResourceView(D3D12_ROOT_DESCRIPTOR_FLAGS Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE)
	{
		CD3DX12_ROOT_PARAMETER1 Parameter = {};
		Parameter.InitAsShaderResourceView(ShaderRegister, RegisterSpace, Flags);

		AddParameter(Parameter);
	}

	template<UINT ShaderRegister, UINT RegisterSpace>
	void AddUnorderedAccessView(D3D12_ROOT_DESCRIPTOR_FLAGS Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE)
	{
		CD3DX12_ROOT_PARAMETER1 Parameter = {};
		Parameter.InitAsUnorderedAccessView(ShaderRegister, RegisterSpace, Flags);

		AddParameter(Parameter);
	}

	template<UINT ShaderRegister, UINT RegisterSpace>
	void AddStaticSampler(
		D3D12_FILTER			   Filter,
		D3D12_TEXTURE_ADDRESS_MODE AddressUVW,
		UINT					   MaxAnisotropy,
		D3D12_COMPARISON_FUNC	   ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL,
		D3D12_STATIC_BORDER_COLOR  BorderColor	  = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE)
	{
		CD3DX12_STATIC_SAMPLER_DESC& Desc = StaticSamplers.emplace_back();
		Desc.Init(
			ShaderRegister,
			Filter,
			AddressUVW,
			AddressUVW,
			AddressUVW,
			0.0f,
			MaxAnisotropy,
			ComparisonFunc,
			BorderColor);
		Desc.RegisterSpace = RegisterSpace;
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

	[[nodiscard]] bool IsLocal() const noexcept { return Flags & D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE; }

private:
	void AddParameter(D3D12_ROOT_PARAMETER1 Parameter)
	{
		Parameters.emplace_back(Parameter);
		DescriptorTableIndices.emplace_back(0xDEADBEEF);
	}

private:
	std::vector<CD3DX12_ROOT_PARAMETER1>	 Parameters;
	std::vector<CD3DX12_STATIC_SAMPLER_DESC> StaticSamplers;
	D3D12_ROOT_SIGNATURE_FLAGS				 Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

	std::vector<UINT>				  DescriptorTableIndices;
	std::vector<D3D12DescriptorTable> DescriptorTables;
};

class D3D12RootSignature final : public D3D12DeviceChild
{
public:
	D3D12RootSignature() noexcept = default;
	D3D12RootSignature(D3D12Device* Parent, RootSignatureBuilder& Builder);

	operator ID3D12RootSignature*() const { return RootSignature.Get(); }

	[[nodiscard]] D3D12_ROOT_SIGNATURE_DESC1 GetDesc() const noexcept { return Desc.Desc_1_1; }

	[[nodiscard]] std::bitset<D3D12_GLOBAL_ROOT_DESCRIPTOR_TABLE_LIMIT> GetDescriptorTableBitMask(
		D3D12_DESCRIPTOR_HEAP_TYPE Type) const noexcept;

	[[nodiscard]] UINT GetNumDescriptors(UINT RootParameterIndex) const noexcept;

private:
	D3D12_VERSIONED_ROOT_SIGNATURE_DESC			Desc = {};
	Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignature;

	// Need to know the number of descriptors per descriptor table
	UINT NumDescriptorsPerTable[D3D12_GLOBAL_ROOT_DESCRIPTOR_TABLE_LIMIT] = {};

	// A bit mask that represents the root parameter indices that are
	// descriptor tables for CBV, UAV, and SRV
	std::bitset<D3D12_GLOBAL_ROOT_DESCRIPTOR_TABLE_LIMIT> ResourceDescriptorTableBitMask;
	// A bit mask that represents the root parameter indices that are
	// descriptor tables for Samplers
	std::bitset<D3D12_GLOBAL_ROOT_DESCRIPTOR_TABLE_LIMIT> SamplerTableBitMask;
};
