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
};

LPCWSTR GetCommandQueueTypeString(ED3D12CommandQueueType CommandQueueType);
LPCWSTR GetCommandQueueTypeFenceString(ED3D12CommandQueueType CommandQueueType);

LPCWSTR GetAutoBreadcrumbOpString(D3D12_AUTO_BREADCRUMB_OP Op);
LPCWSTR GetDredAllocationTypeString(D3D12_DRED_ALLOCATION_TYPE Type);

class D3D12Exception : public Exception
{
public:
	D3D12Exception(const char* File, int Line, HRESULT ErrorCode);

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

	[[nodiscard]] auto GetParentDevice() const noexcept -> D3D12Device* { return Parent; }

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

	[[nodiscard]] auto GetParentLinkedDevice() const noexcept -> D3D12LinkedDevice* { return Parent; }

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

	[[nodiscard]] auto IsValid() const noexcept -> bool;
	[[nodiscard]] auto GetValue() const noexcept -> UINT64;
	[[nodiscard]] auto IsComplete() const -> bool;
	auto			   WaitForCompletion() const -> void;

private:
	friend class D3D12CommandQueue;

	ID3D12Fence* Fence;
	UINT64		 Value;
};

template<typename TFunc>
struct D3D12ScopedMap
{
	D3D12ScopedMap(ID3D12Resource* Resource, UINT Subresource, D3D12_RANGE ReadRange, TFunc Func)
		: Resource(Resource)
	{
		void* Data = nullptr;
		if (SUCCEEDED(Resource->Map(Subresource, &ReadRange, &Data)))
		{
			Func(Data);
		}
		else
		{
			Resource = nullptr;
		}
	}
	~D3D12ScopedMap()
	{
		if (Resource)
		{
			Resource->Unmap(0, nullptr);
		}
	}

	ID3D12Resource* Resource = nullptr;
};
