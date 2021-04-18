#pragma once
#include <d3d12.h>
#include <wrl/client.h>

//----------------------------------------------------------------------------------------------------
class RootSignatureBuilder;

//----------------------------------------------------------------------------------------------------
class RootParameter
{
public:
	enum class Type
	{
		DescriptorTable,
		Constants,
		CBV,
		SRV,
		UAV
	};

	RootParameter(Type Type);

	inline auto GetD3DRootParameter() const { return m_RootParameter; }
protected:
	D3D12_ROOT_PARAMETER1 m_RootParameter;
};

//----------------------------------------------------------------------------------------------------
struct DescriptorRange
{
	enum class Type
	{
		SRV,
		UAV,
		CBV,
		Sampler
	};

	DescriptorRange
	(
		UINT NumDescriptors,
		UINT BaseShaderRegister,
		UINT RegisterSpace,
		D3D12_DESCRIPTOR_RANGE_FLAGS Flags,
		UINT OffsetInDescriptorsFromTableStart
	)
		: NumDescriptors(NumDescriptors),
		BaseShaderRegister(BaseShaderRegister),
		RegisterSpace(RegisterSpace),
		Flags(Flags),
		OffsetInDescriptorsFromTableStart(OffsetInDescriptorsFromTableStart)
	{

	}

	UINT NumDescriptors;
	UINT BaseShaderRegister;
	UINT RegisterSpace;
	D3D12_DESCRIPTOR_RANGE_FLAGS Flags;
	UINT OffsetInDescriptorsFromTableStart;
};

//----------------------------------------------------------------------------------------------------
class RootDescriptorTable : public RootParameter
{
public:
	RootDescriptorTable();

	void AddDescriptorRange(DescriptorRange::Type Type, const DescriptorRange& DescriptorRange);

	inline auto GetDescriptorRanges() const { return m_DescriptorRanges; }
private:
	std::vector<D3D12_DESCRIPTOR_RANGE1> m_DescriptorRanges;
};

//----------------------------------------------------------------------------------------------------
template<typename T>
class RootConstants : public RootParameter
{
public:
	RootConstants(UINT ShaderRegister, UINT RegisterSpace)
		: RootParameter(RootParameter::Type::Constants)
	{
		m_RootParameter.Constants.ShaderRegister = ShaderRegister;
		m_RootParameter.Constants.RegisterSpace = RegisterSpace;
		m_RootParameter.Constants.Num32BitValues = sizeof(T) / 4;
	}
};

//----------------------------------------------------------------------------------------------------
template<>
class RootConstants<void> : public RootParameter
{
public:
	RootConstants(UINT ShaderRegister, UINT RegisterSpace, UINT Num32BitValues)
		: RootParameter(RootParameter::Type::Constants)
	{
		m_RootParameter.Constants.ShaderRegister = ShaderRegister;
		m_RootParameter.Constants.RegisterSpace = RegisterSpace;
		m_RootParameter.Constants.Num32BitValues = Num32BitValues;
	}
};

//----------------------------------------------------------------------------------------------------
class RootCBV : public RootParameter
{
public:
	RootCBV(UINT ShaderRegister, UINT RegisterSpace)
		: RootParameter(RootParameter::Type::CBV)
	{
		m_RootParameter.Descriptor.ShaderRegister = ShaderRegister;
		m_RootParameter.Descriptor.RegisterSpace = RegisterSpace;
		m_RootParameter.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;
	}
};

//----------------------------------------------------------------------------------------------------
class RootSRV : public RootParameter
{
public:
	RootSRV(UINT ShaderRegister, UINT RegisterSpace)
		: RootParameter(RootParameter::Type::SRV)
	{
		m_RootParameter.Descriptor.ShaderRegister = ShaderRegister;
		m_RootParameter.Descriptor.RegisterSpace = RegisterSpace;
		m_RootParameter.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;
	}
};

//----------------------------------------------------------------------------------------------------
class RootUAV : public RootParameter
{
public:
	RootUAV(UINT ShaderRegister, UINT RegisterSpace)
		: RootParameter(RootParameter::Type::UAV)
	{
		m_RootParameter.Descriptor.ShaderRegister = ShaderRegister;
		m_RootParameter.Descriptor.RegisterSpace = RegisterSpace;
		m_RootParameter.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;
	}
};

//----------------------------------------------------------------------------------------------------
class RootSignature
{
public:
	enum : UINT
	{
		UnboundDescriptorSize = UINT_MAX
	};

	RootSignature() = default;
	RootSignature(ID3D12Device* pDevice, RootSignatureBuilder& Builder);

	operator auto() const { return m_RootSignature.Get(); }

	UINT NumParameters = 0;
	UINT NumStaticSamplers = 0;
private:
	Microsoft::WRL::ComPtr<ID3D12RootSignature>	m_RootSignature;
};