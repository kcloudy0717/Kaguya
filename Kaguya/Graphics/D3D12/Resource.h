#pragma once
#include "D3D12Common.h"
#include "Descriptor.h"

// Custom resource states
#define D3D12_RESOURCE_STATE_UNKNOWN	   (static_cast<D3D12_RESOURCE_STATES>(-1))
#define D3D12_RESOURCE_STATE_UNINITIALIZED (static_cast<D3D12_RESOURCE_STATES>(-2))

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
	{
		TrackingMode  = ETrackingMode::PerResource;
		ResourceState = D3D12_RESOURCE_STATE_UNINITIALIZED;
	}
	CResourceState(UINT NumSubresources)
	{
		TrackingMode  = ETrackingMode::PerResource;
		ResourceState = D3D12_RESOURCE_STATE_UNINITIALIZED;
		SubresourceState.resize(NumSubresources);
	}

	auto begin() { return SubresourceState.begin(); }
	auto end() { return SubresourceState.end(); }

	bool IsResourceStateUninitialized() const noexcept { return ResourceState == D3D12_RESOURCE_STATE_UNINITIALIZED; }
	bool IsResourceStateUnknown() const noexcept { return ResourceState == D3D12_RESOURCE_STATE_UNKNOWN; }

	// Returns true if all subresources have the same state
	bool AreAllSubresourcesSame() const noexcept
	{
		// If TrackingMode is PerResource, then all subresources have the same state
		return TrackingMode == ETrackingMode::PerResource;
	}

	D3D12_RESOURCE_STATES GetSubresourceState(_In_ UINT Subresource) const
	{
		if (TrackingMode == ETrackingMode::PerResource)
		{
			return ResourceState;
		}

		return SubresourceState[Subresource];
	}

	void SetSubresourceState(_In_ UINT Subresource, _In_ D3D12_RESOURCE_STATES State)
	{
		// If setting all subresources, or the resource only has a single subresource, set the per-resource state
		if (Subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES || SubresourceState.size() == 1)
		{
			TrackingMode = ETrackingMode::PerResource;

			ResourceState = State;
		}
		else
		{
			// If we previous tracked resource per resource level, we need to update all
			// all subresource states before proceeding
			if (TrackingMode == ETrackingMode::PerResource)
			{
				TrackingMode = ETrackingMode::PerSubresource;

				for (auto& State : SubresourceState)
				{
					State = ResourceState;
				}
			}

			SubresourceState[Subresource] = State;
		}
	}

private:
	ETrackingMode					   TrackingMode;
	D3D12_RESOURCE_STATES			   ResourceState;
	std::vector<D3D12_RESOURCE_STATES> SubresourceState;
};

class Resource : public DeviceChild
{
public:
	Resource() noexcept = default;
	Resource(
		Device*							 Device,
		D3D12_RESOURCE_DESC				 Desc,
		std::optional<D3D12_CLEAR_VALUE> ClearValue,
		UINT							 NumSubresources)
		: DeviceChild(Device)
		, Desc(Desc)
		, ClearValue(ClearValue)
		, NumSubresources(NumSubresources)
	{
	}

	Resource(Resource&&) noexcept = default;
	Resource& operator=(Resource&&) noexcept = default;

					operator ID3D12Resource*() const { return pResource.Get(); }
	ID3D12Resource* GetResource() const { return pResource.Get(); }

	const D3D12_RESOURCE_DESC& GetDesc() const { return Desc; }

	CResourceState& GetResourceState() { return ResourceState; }

	// https://docs.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12#implicit-state-transitions

	// Can this resource be promoted to State from common
	bool ImplicitStatePromotion(D3D12_RESOURCE_STATES State) const;

	// Can this resource decay back to common
	// NOTE: There are 4 cases
	// Cases:
	// 1. Resources being accessed on a Copy queue, or
	// 2. Buffer resources on any queue type, or
	// 3. Texture resources on any queue type that have the D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS flag set, or
	// 4. Any resource implicitly promoted to a read-only state.
	bool ImplicitStateDecay(D3D12_RESOURCE_STATES State, D3D12_COMMAND_LIST_TYPE AccessedQueueType) const;

protected:
	Microsoft::WRL::ComPtr<ID3D12Resource> pResource;
	D3D12_RESOURCE_DESC					   Desc;
	std::optional<D3D12_CLEAR_VALUE>	   ClearValue;
	UINT								   NumSubresources;
	CResourceState						   ResourceState;
};

class ASBuffer : public Resource
{
public:
	ASBuffer() noexcept = default;
	ASBuffer(Device* Device, UINT64 SizeInBytes);

	ASBuffer(ASBuffer&&) noexcept = default;
	ASBuffer& operator=(ASBuffer&&) noexcept = default;

	D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const { return pResource->GetGPUVirtualAddress(); }
};

class Buffer : public Resource
{
public:
	Buffer() noexcept = default;
	Buffer(
		Device*				 Device,
		UINT64				 SizeInBytes,
		UINT				 Stride,
		D3D12_HEAP_TYPE		 HeapType,
		D3D12_RESOURCE_FLAGS ResourceFlags);
	~Buffer();

	Buffer(Buffer&&) noexcept = default;
	Buffer& operator=(Buffer&&) noexcept = default;

	D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const { return pResource->GetGPUVirtualAddress(); }
	D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress(int Index) const
	{
		return pResource->GetGPUVirtualAddress() + Index * Stride;
	}

	UINT GetStride() const { return Stride; }
	template<typename T>
	T* GetCPUVirtualAddress() const
	{
		assert(CPUVirtualAddress && "Invalid CPUVirtualAddress");
		return reinterpret_cast<T*>(CPUVirtualAddress);
	}

	template<typename T>
	void CopyData(int Index, const T& Data)
	{
		memcpy(&CPUVirtualAddress[Index * Stride], &Data, sizeof(T));
	}

private:
	UINT  Stride			= 0;
	BYTE* CPUVirtualAddress = nullptr;
};

template<typename T>
class StructuredBuffer : public Buffer
{
public:
	StructuredBuffer() noexcept = default;
	StructuredBuffer(Device* Device, UINT64 NumElements, D3D12_HEAP_TYPE HeapType)
		: Buffer(Device, NumElements * sizeof(T), sizeof(T), HeapType, D3D12_RESOURCE_FLAG_NONE)
	{
	}

	StructuredBuffer(StructuredBuffer&&) noexcept = default;
	StructuredBuffer& operator=(StructuredBuffer&&) noexcept = default;
};

template<typename T>
class RWStructuredBuffer : public Buffer
{
public:
	RWStructuredBuffer() noexcept = default;
	RWStructuredBuffer(Device* Device, UINT64 NumElements, D3D12_HEAP_TYPE HeapType)
		: Buffer(Device, NumElements * sizeof(T), sizeof(T), HeapType, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
	{
	}

	RWStructuredBuffer(RWStructuredBuffer&&) noexcept = default;
	RWStructuredBuffer& operator=(RWStructuredBuffer&&) noexcept = default;
};

class Texture : public Resource
{
public:
	Texture() noexcept = default;
	Texture(Device* Device, const D3D12_RESOURCE_DESC& Desc, std::optional<D3D12_CLEAR_VALUE> ClearValue);

	Texture(Texture&&) noexcept = default;
	Texture& operator=(Texture&&) noexcept = default;

	void CreateShaderResourceView(
		ShaderResourceView& ShaderResourceView,
		std::optional<UINT> OptMostDetailedMip = {},
		std::optional<UINT> OptMipLevels	   = {});

	void CreateUnorderedAccessView(
		UnorderedAccessView& UnorderedAccessView,
		std::optional<UINT>	 OptArraySlice = {},
		std::optional<UINT>	 OptMipSlice   = {});

	void CreateRenderTargetView(
		RenderTargetView&	RenderTargetView,
		std::optional<UINT> OptArraySlice = {},
		std::optional<UINT> OptMipSlice	  = {},
		std::optional<UINT> OptArraySize  = {},
		bool				sRGB		  = false);

	void CreateDepthStencilView(
		DepthStencilView&	DepthStencilView,
		std::optional<UINT> OptArraySlice = {},
		std::optional<UINT> OptMipSlice	  = {},
		std::optional<UINT> OptArraySize  = {});
};

class ReadbackBuffer
{
public:
	ReadbackBuffer() noexcept = default;
	ReadbackBuffer(_In_ ID3D12Device* Device, _In_ UINT64 Size)
	{
		auto hp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
		auto rd = CD3DX12_RESOURCE_DESC::Buffer(Size);
		ASSERTD3D12APISUCCEEDED(Device->CreateCommittedResource(
			&hp,
			D3D12_HEAP_FLAG_NONE,
			&rd,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&Resource)));
	}

	const void* Map() const
	{
		void* data = nullptr;
		Resource->Map(0, nullptr, &data);
		return data;
	}

	template<typename T>
	const T* Map() const
	{
		return reinterpret_cast<const T*>(Map());
	};

	void Unmap() const { Resource->Unmap(0, nullptr); }

	Microsoft::WRL::ComPtr<ID3D12Resource> Resource;
};
