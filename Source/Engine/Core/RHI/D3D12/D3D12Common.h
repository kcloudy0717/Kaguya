#pragma once
#include "Core/RHI/RHICore.h"
#include "d3dx12.h"
#include "D3D12Config.h"
#include "D3D12Profiler.h"

// Custom resource states
constexpr D3D12_RESOURCE_STATES D3D12_RESOURCE_STATE_UNKNOWN	   = static_cast<D3D12_RESOURCE_STATES>(-1);
constexpr D3D12_RESOURCE_STATES D3D12_RESOURCE_STATE_UNINITIALIZED = static_cast<D3D12_RESOURCE_STATES>(-2);

enum class RHID3D12CommandQueueType
{
	Direct,
	AsyncCompute,
	Copy1, // High frequency copies from upload to default heap
	Copy2, // Data initialization during resource creation
};

#define D3D12_BUILTIN_TRIANGLE_INTERSECTION_ATTRIBUTES (8)

// Root signature entry cost: https://docs.microsoft.com/en-us/windows/win32/direct3d12/root-signature-limits
// Local root descriptor table cost:
// https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html#shader-table-memory-initialization
#define D3D12_GLOBAL_ROOT_DESCRIPTOR_TABLE_COST		   (1)
#define D3D12_LOCAL_ROOT_DESCRIPTOR_TABLE_COST		   (2)
#define D3D12_ROOT_CONSTANT_COST					   (1)
#define D3D12_ROOT_DESCRIPTOR_COST					   (2)

#define D3D12_GLOBAL_ROOT_DESCRIPTOR_TABLE_LIMIT	   ((D3D12_MAX_ROOT_COST) / (D3D12_GLOBAL_ROOT_DESCRIPTOR_TABLE_COST))

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

#define VERIFY_D3D12_API(expr)                            \
	do                                                    \
	{                                                     \
		HRESULT hr = expr;                                \
		if (FAILED(hr))                                   \
		{                                                 \
			throw D3D12Exception(__FILE__, __LINE__, hr); \
		}                                                 \
	} while (false)

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
class D3D12Fence;

class D3D12SyncHandle
{
public:
	D3D12SyncHandle() noexcept
		: Fence(nullptr)
		, Value(0)
	{
	}
	D3D12SyncHandle(D3D12Fence* Fence, UINT64 Value) noexcept
		: Fence(Fence)
		, Value(Value)
	{
	}

	D3D12SyncHandle(std::nullptr_t) noexcept
		: D3D12SyncHandle()
	{
	}

	D3D12SyncHandle& operator=(std::nullptr_t) noexcept
	{
		Fence = nullptr;
		Value = 0;
		return *this;
	}

	explicit operator bool() const noexcept;

	[[nodiscard]] auto GetValue() const noexcept -> UINT64;
	[[nodiscard]] auto IsComplete() const -> bool;
	auto			   WaitForCompletion() const -> void;

private:
	friend class D3D12CommandQueue;

	D3D12Fence* Fence;
	UINT64		Value;
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

class D3D12InputLayout
{
public:
	D3D12InputLayout() noexcept = default;
	D3D12InputLayout(size_t NumElements) { InputElements.reserve(NumElements); }

	void AddVertexLayoutElement(std::string_view SemanticName, UINT SemanticIndex, DXGI_FORMAT Format, UINT InputSlot);

	std::vector<D3D12_INPUT_ELEMENT_DESC> InputElements;
};

// https://microsoft.github.io/DirectX-Specs/d3d/CPUEfficiency.html#subresource-state-tracking
class CResourceState
{
public:
	enum class ETrackingMode
	{
		PerResource,
		PerSubresource
	};

	CResourceState() noexcept
		: TrackingMode(ETrackingMode::PerResource)
		, ResourceState(D3D12_RESOURCE_STATE_UNINITIALIZED)
	{
	}
	explicit CResourceState(UINT NumSubresources)
		: CResourceState()
	{
		SubresourceStates.resize(NumSubresources);
	}

	[[nodiscard]] auto begin() const noexcept { return SubresourceStates.begin(); }
	[[nodiscard]] auto end() const noexcept { return SubresourceStates.end(); }

	[[nodiscard]] bool IsUninitialized() const noexcept { return ResourceState == D3D12_RESOURCE_STATE_UNINITIALIZED; }
	[[nodiscard]] bool IsUnknown() const noexcept { return ResourceState == D3D12_RESOURCE_STATE_UNKNOWN; }

	// Returns true if all subresources have the same state
	[[nodiscard]] bool IsUniform() const noexcept { return TrackingMode == ETrackingMode::PerResource; }

	[[nodiscard]] D3D12_RESOURCE_STATES GetSubresourceState(UINT Subresource) const;

	void SetSubresourceState(UINT Subresource, D3D12_RESOURCE_STATES State);

private:
	ETrackingMode					   TrackingMode;
	D3D12_RESOURCE_STATES			   ResourceState;
	std::vector<D3D12_RESOURCE_STATES> SubresourceStates;
};

template<typename TResourceType>
class CFencePool
{
public:
	CFencePool(bool ThreadSafe = false) noexcept
		: Mutex(ThreadSafe ? std::make_unique<std::mutex>() : nullptr)
	{
	}
	CFencePool(CFencePool&& CFencePool) noexcept
		: Pool(std::exchange(CFencePool.Pool, {}))
		, Mutex(std::exchange(CFencePool.Mutex, {}))
	{
	}
	CFencePool& operator=(CFencePool&& CFencePool) noexcept
	{
		if (this == &CFencePool)
		{
			return *this;
		}

		Pool  = std::exchange(CFencePool.Pool, {});
		Mutex = std::exchange(CFencePool.Mutex, {});
		return *this;
	}

	CFencePool(const CFencePool&) = delete;
	CFencePool& operator=(const CFencePool&) = delete;

	void ReturnToPool(TResourceType&& Resource, D3D12SyncHandle SyncHandle) noexcept
	{
		try
		{
			auto Lock = Mutex ? std::unique_lock(*Mutex) : std::unique_lock<std::mutex>();
			Pool.emplace_back(SyncHandle, std::move(Resource)); // throw( bad_alloc )
		}
		catch (std::bad_alloc&)
		{
			// Just drop the error
			// All uses of this pool use Arc, which will release the resource
		}
	}

	template<typename PFNCreateNew, typename... TArgs>
	TResourceType RetrieveFromPool(PFNCreateNew CreateNew, TArgs&&... Args) noexcept(false)
	{
		auto Lock = Mutex ? std::unique_lock(*Mutex) : std::unique_lock<std::mutex>();
		auto Head = Pool.begin();
		if (Head == Pool.end() || !Head->first.IsComplete())
		{
			return std::move(CreateNew(std::forward<TArgs>(Args)...));
		}

		assert(Head->second);
		TResourceType Resource = std::move(Head->second);
		Pool.erase(Head);
		return std::move(Resource);
	}

protected:
	using TPoolEntry = std::pair<D3D12SyncHandle, TResourceType>;
	using TPool		 = std::list<TPoolEntry>;

	TPool						Pool;
	std::unique_ptr<std::mutex> Mutex;
};
