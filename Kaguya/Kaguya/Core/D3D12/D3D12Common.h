#pragma once
#include <d3d12.h>
#include "d3dx12.h"
#include "D3D12Utility.h"

#define D3D12_BUILTIN_TRIANGLE_INTERSECTION_ATTRIBUTES 8

enum class ECommandQueueType
{
	Direct,
	AsyncCompute,

	Copy1, // High frequency copies from upload to default heap
	Copy2, // Data initialization during resource creation

	NumCommandQueues
};

LPCWSTR GetCommandQueueTypeString(ECommandQueueType CommandQueueType);

class D3D12Exception : public CoreException
{
public:
	D3D12Exception(const char* File, int Line, HRESULT ErrorCode)
		: CoreException(File, Line)
		, ErrorCode(ErrorCode)
	{
	}

	const char* GetErrorType() const noexcept override;
	std::string GetError() const override;

private:
	HRESULT ErrorCode;
};

#define ASSERTD3D12APISUCCEEDED(expr)                                                                                  \
	{                                                                                                                  \
		HRESULT hr = expr;                                                                                             \
		if (FAILED(hr))                                                                                                \
		{                                                                                                              \
			throw D3D12Exception(__FILE__, __LINE__, hr);                                                              \
		}                                                                                                              \
	}

class Adapter;

class AdapterChild
{
public:
	AdapterChild() noexcept
		: Parent(nullptr)
	{
	}

	AdapterChild(Adapter* Parent) noexcept
		: Parent(Parent)
	{
	}

	Adapter* GetParentAdapter() const { return Parent; }

	void SetParentAdapter(Adapter* Parent)
	{
		assert(this->Parent == nullptr);
		this->Parent = Parent;
	}

protected:
	Adapter* Parent;
};

class Device;

class DeviceChild
{
public:
	DeviceChild() noexcept
		: Parent(nullptr)
	{
	}

	DeviceChild(Device* Parent) noexcept
		: Parent(Parent)
	{
	}

	Device* GetParentDevice() const { return Parent; }

	void SetParentDevice(Device* Parent)
	{
		assert(this->Parent == nullptr);
		this->Parent = Parent;
	}

protected:
	Device* Parent;
};

// Represents a Fence and Value pair, similar to that of a coroutine handle
// you can query the status of a command execution point and wait for it
class CommandSyncPoint
{
public:
	CommandSyncPoint()
		: Fence(nullptr)
		, Value(0)
	{
	}
	CommandSyncPoint(ID3D12Fence* Fence, UINT64 Value)
		: Fence(Fence)
		, Value(Value)
	{
	}

	bool   IsValid() const;
	UINT64 GetValue() const;
	bool   IsComplete() const;
	void   WaitForCompletion() const;

private:
	friend class CommandQueue;

	ID3D12Fence* Fence;
	UINT64		 Value;
};

// clang-format off
template<D3D12_FEATURE> struct D3D12FeatureTraits										{ using Type = void; };
template<> struct D3D12FeatureTraits<D3D12_FEATURE_D3D12_OPTIONS>						{ using Type = D3D12_FEATURE_DATA_D3D12_OPTIONS; };
template<> struct D3D12FeatureTraits<D3D12_FEATURE_ARCHITECTURE>						{ using Type = D3D12_FEATURE_DATA_ARCHITECTURE; };
template<> struct D3D12FeatureTraits<D3D12_FEATURE_FEATURE_LEVELS>						{ using Type = D3D12_FEATURE_DATA_FEATURE_LEVELS; };
template<> struct D3D12FeatureTraits<D3D12_FEATURE_FORMAT_SUPPORT>						{ using Type = D3D12_FEATURE_DATA_FORMAT_SUPPORT; };
template<> struct D3D12FeatureTraits<D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS>			{ using Type = D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS; };
template<> struct D3D12FeatureTraits<D3D12_FEATURE_FORMAT_INFO>							{ using Type = D3D12_FEATURE_DATA_FORMAT_INFO; };
template<> struct D3D12FeatureTraits<D3D12_FEATURE_GPU_VIRTUAL_ADDRESS_SUPPORT>			{ using Type = D3D12_FEATURE_DATA_GPU_VIRTUAL_ADDRESS_SUPPORT; };
template<> struct D3D12FeatureTraits<D3D12_FEATURE_SHADER_MODEL>						{ using Type = D3D12_FEATURE_DATA_SHADER_MODEL; };
template<> struct D3D12FeatureTraits<D3D12_FEATURE_D3D12_OPTIONS1>						{ using Type = D3D12_FEATURE_DATA_D3D12_OPTIONS1; };
template<> struct D3D12FeatureTraits<D3D12_FEATURE_PROTECTED_RESOURCE_SESSION_SUPPORT>	{ using Type = D3D12_FEATURE_DATA_PROTECTED_RESOURCE_SESSION_SUPPORT; };
template<> struct D3D12FeatureTraits<D3D12_FEATURE_ROOT_SIGNATURE>						{ using Type = D3D12_FEATURE_DATA_ROOT_SIGNATURE; };
template<> struct D3D12FeatureTraits<D3D12_FEATURE_ARCHITECTURE1>						{ using Type = D3D12_FEATURE_DATA_ARCHITECTURE1; };
template<> struct D3D12FeatureTraits<D3D12_FEATURE_D3D12_OPTIONS2>						{ using Type = D3D12_FEATURE_DATA_D3D12_OPTIONS2; };
template<> struct D3D12FeatureTraits<D3D12_FEATURE_SHADER_CACHE>						{ using Type = D3D12_FEATURE_DATA_SHADER_CACHE; };
template<> struct D3D12FeatureTraits<D3D12_FEATURE_COMMAND_QUEUE_PRIORITY>				{ using Type = D3D12_FEATURE_DATA_COMMAND_QUEUE_PRIORITY; };
template<> struct D3D12FeatureTraits<D3D12_FEATURE_D3D12_OPTIONS3>						{ using Type = D3D12_FEATURE_DATA_D3D12_OPTIONS3; };
template<> struct D3D12FeatureTraits<D3D12_FEATURE_EXISTING_HEAPS>						{ using Type = D3D12_FEATURE_DATA_EXISTING_HEAPS; };
template<> struct D3D12FeatureTraits<D3D12_FEATURE_D3D12_OPTIONS4>						{ using Type = D3D12_FEATURE_DATA_D3D12_OPTIONS4; };
template<> struct D3D12FeatureTraits<D3D12_FEATURE_SERIALIZATION>						{ using Type = D3D12_FEATURE_DATA_SERIALIZATION; };
template<> struct D3D12FeatureTraits<D3D12_FEATURE_CROSS_NODE>							{ using Type = D3D12_FEATURE_DATA_CROSS_NODE; };
template<> struct D3D12FeatureTraits<D3D12_FEATURE_D3D12_OPTIONS5>						{ using Type = D3D12_FEATURE_DATA_D3D12_OPTIONS5; };
template<> struct D3D12FeatureTraits<D3D12_FEATURE_D3D12_OPTIONS6>						{ using Type = D3D12_FEATURE_DATA_D3D12_OPTIONS6; };
template<> struct D3D12FeatureTraits<D3D12_FEATURE_QUERY_META_COMMAND>					{ using Type = D3D12_FEATURE_DATA_QUERY_META_COMMAND; };
// clang-format on

template<D3D12_FEATURE Feature>
struct D3D12Feature
{
	static constexpr D3D12_FEATURE Feature = Feature;
	using Type							   = D3D12FeatureTraits<Feature>::Type;

	Type FeatureSupportData;

	const Type* operator->() const { return &FeatureSupportData; }
};
