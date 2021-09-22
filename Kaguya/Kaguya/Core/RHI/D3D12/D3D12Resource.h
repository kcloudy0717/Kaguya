#pragma once
#include "D3D12Common.h"
#include "D3D12Descriptor.h"

// Custom resource states
constexpr D3D12_RESOURCE_STATES D3D12_RESOURCE_STATE_UNKNOWN	   = static_cast<D3D12_RESOURCE_STATES>(-1);
constexpr D3D12_RESOURCE_STATES D3D12_RESOURCE_STATE_UNINITIALIZED = static_cast<D3D12_RESOURCE_STATES>(-2);

// https://microsoft.github.io/DirectX-Specs/d3d/CPUEfficiency.html#subresource-state-tracking
class CResourceState
{
public:
	enum class ETrackingMode
	{
		PerResource,
		PerSubresource
	};

	CResourceState()
		: TrackingMode(ETrackingMode::PerResource)
		, ResourceState(D3D12_RESOURCE_STATE_UNINITIALIZED)
	{
	}
	explicit CResourceState(UINT NumSubresources)
		: CResourceState()
	{
		SubresourceStates.resize(NumSubresources);
	}

	[[nodiscard]] auto begin() { return SubresourceStates.begin(); }
	[[nodiscard]] auto end() { return SubresourceStates.end(); }

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

class D3D12Resource : public D3D12LinkedDeviceChild
{
public:
	D3D12Resource() noexcept = default;
	D3D12Resource(
		D3D12LinkedDevice*				 Parent,
		D3D12_RESOURCE_DESC				 Desc,
		std::optional<D3D12_CLEAR_VALUE> ClearValue,
		UINT							 NumSubresources)
		: D3D12LinkedDeviceChild(Parent)
		, Desc(Desc)
		, ClearValue(ClearValue)
		, NumSubresources(NumSubresources)
	{
	}

								  operator ID3D12Resource*() const { return Resource.Get(); }
	[[nodiscard]] ID3D12Resource* GetResource() const { return Resource.Get(); }

	[[nodiscard]] const D3D12_RESOURCE_DESC& GetDesc() const noexcept { return Desc; }
	[[nodiscard]] UINT						 GetNumSubresources() const noexcept { return NumSubresources; }
	[[nodiscard]] CResourceState&			 GetResourceState() { return ResourceState; }

	// https://docs.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12#implicit-state-transitions
	// https://devblogs.microsoft.com/directx/a-look-inside-d3d12-resource-state-barriers/
	// Can this resource be promoted to State from common
	[[nodiscard]] bool ImplicitStatePromotion(D3D12_RESOURCE_STATES State) const noexcept;

	// Can this resource decay back to common
	// 4 Cases:
	// 1. Resources being accessed on a Copy queue, or
	// 2. Buffer resources on any queue type, or
	// 3. Texture resources on any queue type that have the D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS flag set, or
	// 4. Any resource implicitly promoted to a read-only state.
	[[nodiscard]] bool ImplicitStateDecay(D3D12_RESOURCE_STATES State, D3D12_COMMAND_LIST_TYPE AccessedQueueType)
		const noexcept;

protected:
	Microsoft::WRL::ComPtr<ID3D12Resource> Resource;
	D3D12_RESOURCE_DESC					   Desc;
	std::optional<D3D12_CLEAR_VALUE>	   ClearValue;
	UINT								   NumSubresources;
	CResourceState						   ResourceState;
};

class D3D12ASBuffer : public D3D12Resource
{
public:
	D3D12ASBuffer() noexcept = default;
	D3D12ASBuffer(D3D12LinkedDevice* Parent, UINT64 SizeInBytes);

	[[nodiscard]] D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress() const;
};

class D3D12Buffer : public D3D12Resource
{
public:
	D3D12Buffer() noexcept = default;
	D3D12Buffer(
		D3D12LinkedDevice*	 Parent,
		UINT64				 SizeInBytes,
		UINT				 Stride,
		D3D12_HEAP_TYPE		 HeapType,
		D3D12_RESOURCE_FLAGS ResourceFlags);
	~D3D12Buffer();

	// Call this for upload heap to map a cpu pointer
	void Initialize();

	[[nodiscard]] D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress() const;
	[[nodiscard]] D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress(UINT Index) const;
	[[nodiscard]] UINT						GetStride() const { return Stride; }
	template<typename T>
	[[nodiscard]] T* GetCpuVirtualAddress() const
	{
		assert(CpuVirtualAddress && "Invalid CPUVirtualAddress");
		return reinterpret_cast<T*>(CpuVirtualAddress);
	}

	template<typename T>
	void CopyData(UINT Index, const T& Data)
	{
		assert(CpuVirtualAddress && "Invalid CPUVirtualAddress");
		memcpy(&CpuVirtualAddress[Index * Stride], &Data, sizeof(T));
	}

private:
	D3D12_HEAP_TYPE HeapType		  = {};
	UINT			Stride			  = 0;
	BYTE*			CpuVirtualAddress = nullptr;
};

template<typename T>
class StructuredBuffer : public D3D12Buffer
{
public:
	StructuredBuffer() noexcept = default;
	StructuredBuffer(D3D12LinkedDevice* Parent, UINT64 NumElements, D3D12_HEAP_TYPE HeapType)
		: D3D12Buffer(Parent, NumElements * sizeof(T), sizeof(T), HeapType, D3D12_RESOURCE_FLAG_NONE)
	{
	}
};

template<typename T>
class RWStructuredBuffer : public D3D12Buffer
{
public:
	RWStructuredBuffer() noexcept = default;
	RWStructuredBuffer(D3D12LinkedDevice* Parent, UINT64 NumElements, D3D12_HEAP_TYPE HeapType)
		: D3D12Buffer(Parent, NumElements * sizeof(T), sizeof(T), HeapType, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
	{
	}
};

class D3D12Texture : public D3D12Resource
{
public:
	D3D12Texture() noexcept = default;
	D3D12Texture(
		D3D12LinkedDevice*				 Parent,
		const D3D12_RESOURCE_DESC&		 Desc,
		std::optional<D3D12_CLEAR_VALUE> ClearValue = std::nullopt);

	[[nodiscard]] UINT GetSubresourceIndex(
		std::optional<UINT> OptArraySlice = std::nullopt,
		std::optional<UINT> OptMipSlice	  = std::nullopt,
		std::optional<UINT> OptPlaneSlice = std::nullopt) const noexcept;

	void CreateShaderResourceView(
		D3D12ShaderResourceView& ShaderResourceView,
		std::optional<UINT>		 OptMostDetailedMip = std::nullopt,
		std::optional<UINT>		 OptMipLevels		= std::nullopt) const;

	void CreateUnorderedAccessView(
		D3D12UnorderedAccessView& UnorderedAccessView,
		std::optional<UINT>		  OptArraySlice = std::nullopt,
		std::optional<UINT>		  OptMipSlice	= std::nullopt) const;

	void CreateRenderTargetView(
		D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetView,
		std::optional<UINT>			OptArraySlice = std::nullopt,
		std::optional<UINT>			OptMipSlice	  = std::nullopt,
		std::optional<UINT>			OptArraySize  = std::nullopt,
		bool						sRGB		  = false) const;

	void CreateDepthStencilView(
		D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView,
		std::optional<UINT>			OptArraySlice = std::nullopt,
		std::optional<UINT>			OptMipSlice	  = std::nullopt,
		std::optional<UINT>			OptArraySize  = std::nullopt) const;
};
