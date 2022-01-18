#pragma once
#include "D3D12RHI.h"
#include "ARC.h"
#include "RHICore.h"
#include "System/System.h"

namespace D3D12RHIUtils
{
	template<typename T, typename U>
	constexpr T AlignUp(T Size, U Alignment)
	{
		return (T)(((size_t)Size + (size_t)Alignment - 1) & ~((size_t)Alignment - 1));
	}

	constexpr DXGI_FORMAT MakeSRGB(DXGI_FORMAT Format) noexcept
	{
		switch (Format)
		{
		case DXGI_FORMAT_R8G8B8A8_UNORM:
			return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		case DXGI_FORMAT_BC1_UNORM:
			return DXGI_FORMAT_BC1_UNORM_SRGB;
		case DXGI_FORMAT_BC2_UNORM:
			return DXGI_FORMAT_BC2_UNORM_SRGB;
		case DXGI_FORMAT_BC3_UNORM:
			return DXGI_FORMAT_BC3_UNORM_SRGB;
		case DXGI_FORMAT_B8G8R8A8_UNORM:
			return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
		case DXGI_FORMAT_B8G8R8X8_UNORM:
			return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
		case DXGI_FORMAT_BC7_UNORM:
			return DXGI_FORMAT_BC7_UNORM_SRGB;
		default:
			return Format;
		}
	}
} // namespace D3D12RHIUtils

enum class RHID3D12CommandQueueType
{
	Direct,
	AsyncCompute,
	Copy1, // High frequency copies from upload to default heap
	Copy2, // Data initialization during resource creation
};

class D3D12Exception : public Exception
{
public:
	D3D12Exception(const char* File, int Line, HRESULT ErrorCode);

	const char* GetErrorType() const noexcept override;
	std::string GetError() const override;

private:
	const HRESULT ErrorCode;
};

class D3D12Device;

class D3D12DeviceChild
{
public:
	D3D12DeviceChild() noexcept = default;
	explicit D3D12DeviceChild(D3D12Device* Parent) noexcept
		: Parent(Parent)
	{
	}

	[[nodiscard]] auto GetParentDevice() const noexcept -> D3D12Device* { return Parent; }

protected:
	D3D12Device* Parent = nullptr;
};

class D3D12LinkedDevice;

class D3D12LinkedDeviceChild
{
public:
	D3D12LinkedDeviceChild() noexcept = default;
	explicit D3D12LinkedDeviceChild(D3D12LinkedDevice* Parent) noexcept
		: Parent(Parent)
	{
	}

	[[nodiscard]] auto GetParentLinkedDevice() const noexcept -> D3D12LinkedDevice* { return Parent; }

protected:
	D3D12LinkedDevice* Parent = nullptr;
};

// Represents a Fence and Value pair, similar to that of a coroutine handle
// you can query the status of a command execution point and wait for it
class D3D12Fence;

class D3D12SyncHandle
{
public:
	D3D12SyncHandle() noexcept = default;
	explicit D3D12SyncHandle(D3D12Fence* Fence, UINT64 Value) noexcept
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

	D3D12Fence* Fence = nullptr;
	UINT64		Value = 0;
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
			// All uses of this pool use ARC, which will release the resource
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

class D3D12InputLayout
{
public:
	D3D12InputLayout() noexcept = default;
	D3D12InputLayout(size_t NumElements) { InputElements.reserve(NumElements); }

	void AddVertexLayoutElement(std::string_view SemanticName, UINT SemanticIndex, DXGI_FORMAT Format, UINT InputSlot)
	{
		D3D12_INPUT_ELEMENT_DESC& Desc = InputElements.emplace_back();
		Desc.SemanticName			   = SemanticName.data();
		Desc.SemanticIndex			   = SemanticIndex;
		Desc.Format					   = Format;
		Desc.InputSlot				   = InputSlot;
		Desc.AlignedByteOffset		   = D3D12_APPEND_ALIGNED_ELEMENT;
		Desc.InputSlotClass			   = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		Desc.InstanceDataStepRate	   = 0;
	}

	std::vector<D3D12_INPUT_ELEMENT_DESC> InputElements;
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
