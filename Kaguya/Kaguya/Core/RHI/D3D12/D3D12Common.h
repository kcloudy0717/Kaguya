#pragma once
#include <d3d12.h>
#include "d3dx12.h"
#include "D3D12Utility.h"
#include "D3D12Profiler.h"
#include "Aftermath/AftermathCrashTracker.h"

#define D3D12_BUILTIN_TRIANGLE_INTERSECTION_ATTRIBUTES (8)

// Root signature entry cost: https://docs.microsoft.com/en-us/windows/win32/direct3d12/root-signature-limits
// Local root descriptor table cost:
// https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html#shader-table-memory-initialization
#define D3D12_GLOBAL_ROOT_DESCRIPTOR_TABLE_COST		   (1)
#define D3D12_LOCAL_ROOT_DESCRIPTOR_TABLE_COST		   (2)
#define D3D12_ROOT_CONSTANT_COST					   (1)
#define D3D12_ROOT_DESCRIPTOR_COST					   (2)

#define D3D12_GLOBAL_ROOT_DESCRIPTOR_TABLE_LIMIT	   ((D3D12_MAX_ROOT_COST) / (D3D12_GLOBAL_ROOT_DESCRIPTOR_TABLE_COST))

#ifdef _DEBUG
	#define D3D12_DEBUG_RESOURCE_STATES

// The D3D debug layer (as well as Microsoft PIX and other graphics debugger
// tools using an injection library) is not compatible with Nsight Aftermath!
// If Aftermath detects that any of these tools are present it will fail
// initialization.

// Feel free to comment this out
//#define NVIDIA_NSIGHT_AFTERMATH
#endif

enum class ED3D12CommandQueueType
{
	Direct,
	AsyncCompute,

	Copy1, // High frequency copies from upload to default heap
	Copy2, // Data initialization during resource creation

	NumCommandQueues
};

LPCWSTR GetCommandQueueTypeString(ED3D12CommandQueueType CommandQueueType);
LPCWSTR GetCommandQueueTypeFenceString(ED3D12CommandQueueType CommandQueueType);

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
	const HRESULT ErrorCode;
};

#define VERIFY_D3D12_API(expr)                                                                                         \
	{                                                                                                                  \
		HRESULT hr = expr;                                                                                             \
		if (FAILED(hr))                                                                                                \
		{                                                                                                              \
			throw D3D12Exception(__FILE__, __LINE__, hr);                                                              \
		}                                                                                                              \
	}

class D3D12Device;

class D3D12DeviceChild
{
public:
	D3D12DeviceChild() noexcept
		: Parent(nullptr)
	{
	}
	D3D12DeviceChild(D3D12Device* Parent) noexcept
		: Parent(Parent)
	{
	}

	auto GetParentDevice() const noexcept -> D3D12Device* { return Parent; }

	void SetParentDevice(D3D12Device* Parent) noexcept
	{
		assert(this->Parent == nullptr);
		this->Parent = Parent;
	}

protected:
	D3D12Device* Parent;
};

class D3D12LinkedDevice;

class D3D12LinkedDeviceChild
{
public:
	D3D12LinkedDeviceChild() noexcept
		: Parent(nullptr)
	{
	}
	D3D12LinkedDeviceChild(D3D12LinkedDevice* Parent) noexcept
		: Parent(Parent)
	{
	}

	auto GetParentLinkedDevice() const noexcept -> D3D12LinkedDevice* { return Parent; }

	void SetParentLinkedDevice(D3D12LinkedDevice* Parent) noexcept
	{
		assert(this->Parent == nullptr);
		this->Parent = Parent;
	}

protected:
	D3D12LinkedDevice* Parent;
};

// Represents a Fence and Value pair, similar to that of a coroutine handle
// you can query the status of a command execution point and wait for it
class D3D12CommandSyncPoint
{
public:
	D3D12CommandSyncPoint() noexcept
		: Fence(nullptr)
		, Value(0)
	{
	}
	D3D12CommandSyncPoint(ID3D12Fence* Fence, UINT64 Value) noexcept
		: Fence(Fence)
		, Value(Value)
	{
	}

	auto IsValid() const noexcept -> bool;
	auto GetValue() const noexcept -> UINT64;
	auto IsComplete() const -> bool;
	auto WaitForCompletion() const -> void;

private:
	friend class D3D12CommandQueue;

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

	constexpr auto operator->() const noexcept -> const Type* { return &FeatureSupportData; }
};
