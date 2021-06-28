#pragma once
#include "d3dx12.h"
#include "DeviceChild.h"
#include "Descriptor.h"

// Custom resource states
#define D3D12_RESOURCE_STATE_UNKNOWN	   (static_cast<D3D12_RESOURCE_STATES>(-1))
#define D3D12_RESOURCE_STATE_UNINITIALIZED (static_cast<D3D12_RESOURCE_STATES>(-2))

class Heap : public DeviceChild
{
public:
	Heap() noexcept = default;
	Heap(Device* Device)
		: DeviceChild(Device)
	{
	}
	Heap(Device* Device, const D3D12_HEAP_DESC& Desc);

				operator ID3D12Heap*() const { return m_Heap.Get(); }
	ID3D12Heap* GetResource() const { return m_Heap.Get(); }

private:
	Microsoft::WRL::ComPtr<ID3D12Heap> m_Heap;
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
	Resource(Device* Device)
		: DeviceChild(Device)
		, m_Desc()
		, m_NumSubresources(0)
	{
	}

	Resource(Resource&&) noexcept = default;
	Resource& operator=(Resource&&) noexcept = default;

					operator ID3D12Resource*() const { return m_Resource.Get(); }
	ID3D12Resource* GetResource() const { return m_Resource.Get(); }

	const D3D12_RESOURCE_DESC& GetDesc() const { return m_Desc; }

	CResourceState& GetResourceState() { return m_ResourceState; }

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
	Microsoft::WRL::ComPtr<ID3D12Resource> m_Resource;
	D3D12_RESOURCE_DESC					   m_Desc;
	std::optional<D3D12_CLEAR_VALUE>	   m_ClearValue;
	UINT								   m_NumSubresources;
	CResourceState						   m_ResourceState;
};

class ASBuffer : public Resource
{
public:
	ASBuffer() noexcept = default;
	ASBuffer(Device* Device, UINT64 SizeInBytes);

	ASBuffer(ASBuffer&&) noexcept = default;
	ASBuffer& operator=(ASBuffer&&) noexcept = default;

	D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const { return m_Resource->GetGPUVirtualAddress(); }
};

class Buffer : public Resource
{
public:
	Buffer() noexcept = default;
	Buffer(Device* Device)
		: Resource(Device)
	{
		m_Stride			= 0;
		m_CPUVirtualAddress = nullptr;
	}
	Buffer(
		Device*				 Device,
		UINT64				 SizeInBytes,
		UINT				 Stride,
		D3D12_HEAP_TYPE		 HeapType,
		D3D12_RESOURCE_FLAGS ResourceFlags);
	~Buffer();

	Buffer(Buffer&&) noexcept = default;
	Buffer& operator=(Buffer&&) noexcept = default;

	D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const { return m_Resource->GetGPUVirtualAddress(); }
	D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress(int Index) const
	{
		return m_Resource->GetGPUVirtualAddress() + Index * m_Stride;
	}

	UINT GetStride() const { return m_Stride; }
	template<typename T>
	T* GetCPUVirtualAddress() const
	{
		assert(m_CPUVirtualAddress && "Invalid CPUVirtualAddress");
		return reinterpret_cast<T*>(m_CPUVirtualAddress);
	}

	template<typename T>
	void CopyData(int Index, const T& Data)
	{
		memcpy(&m_CPUVirtualAddress[Index * m_Stride], &Data, sizeof(T));
	}

private:
	UINT  m_Stride			  = 0;
	BYTE* m_CPUVirtualAddress = nullptr;
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
	~StructuredBuffer() {}

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
	~RWStructuredBuffer() {}

	RWStructuredBuffer(RWStructuredBuffer&&) noexcept = default;
	RWStructuredBuffer& operator=(RWStructuredBuffer&&) noexcept = default;
};

class Texture : public Resource
{
public:
	Texture() noexcept = default;
	Texture(Device* Device)
		: Resource(Device)
	{
	}
	Texture(Device* Device, const D3D12_RESOURCE_DESC& Desc, std::optional<D3D12_CLEAR_VALUE> ClearValue);

	Texture(Texture&&) noexcept = default;
	Texture& operator=(Texture&&) noexcept = default;

	ShaderResourceView	SRV;
	UnorderedAccessView UAV;
};

class RenderTarget : public Texture
{
public:
	RenderTarget() noexcept = default;
	RenderTarget(Device* Device, UINT Width, UINT Height, DXGI_FORMAT Format, const FLOAT Color[4]);

	RenderTargetView RTV;
};

class DepthStencil : public Texture
{
public:
	DepthStencil() noexcept = default;
	DepthStencil(Device* Device, UINT Width, UINT Height, DXGI_FORMAT Format, FLOAT Depth, UINT8 Stencil);

	DepthStencil(DepthStencil&&) noexcept = default;
	DepthStencil& operator=(DepthStencil&&) noexcept = default;

	DepthStencilView DSV;
};

class ReadbackBuffer
{
public:
	ReadbackBuffer() noexcept = default;
	ReadbackBuffer(_In_ ID3D12Device* Device, _In_ UINT64 Size)
	{
		auto hp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
		auto rd = CD3DX12_RESOURCE_DESC::Buffer(Size);
		ThrowIfFailed(Device->CreateCommittedResource(
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
