#include "D3D12Resource.h"
#include "D3D12LinkedDevice.h"

namespace RHI
{
	// Is this even a word?
	struct ResourceStateDeterminer
	{
		ResourceStateDeterminer(const D3D12_RESOURCE_DESC& Desc, D3D12_HEAP_TYPE HeapType)
			: ShaderResource((Desc.Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) == 0)
			, DepthStencil((Desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) != 0)
			, RenderTarget((Desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) != 0)
			, UnorderedAccess((Desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) != 0)
			, Writable(DepthStencil || RenderTarget || UnorderedAccess)
			, ShaderResourceOnly(ShaderResource && !Writable)
			, Buffer(Desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
			, DeviceLocal(HeapType == D3D12_HEAP_TYPE_DEFAULT)
			, ReadbackResource(HeapType == D3D12_HEAP_TYPE_READBACK)
		{
		}

		[[nodiscard]] D3D12_RESOURCE_STATES InferInitialState() const
		{
			if (Buffer)
			{
				if (DeviceLocal)
				{
					return D3D12_RESOURCE_STATE_COMMON;
				}

				return ReadbackResource ? D3D12_RESOURCE_STATE_COPY_DEST : D3D12_RESOURCE_STATE_GENERIC_READ;
			}
			if (ShaderResourceOnly)
			{
				return D3D12_RESOURCE_STATE_COMMON;
			}
			if (Writable)
			{
				if (DepthStencil)
				{
					return D3D12_RESOURCE_STATE_DEPTH_WRITE;
				}
				if (RenderTarget)
				{
					return D3D12_RESOURCE_STATE_RENDER_TARGET;
				}
				if (UnorderedAccess)
				{
					return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
				}
			}

			return D3D12_RESOURCE_STATE_COMMON;
		}

		bool ShaderResource;
		bool DepthStencil;
		bool RenderTarget;
		bool UnorderedAccess;
		bool Writable;
		bool ShaderResourceOnly;
		bool Buffer;
		bool DeviceLocal;
		bool ReadbackResource;
	};

	D3D12Resource::D3D12Resource(
		D3D12LinkedDevice*	  Parent,
		Arc<ID3D12Resource>&& Resource,
		D3D12_RESOURCE_STATES InitialResourceState)
		: D3D12LinkedDeviceChild(Parent)
		, Resource(std::move(Resource))
		, ClearValue(D3D12_CLEAR_VALUE{})
		, Desc(this->Resource->GetDesc())
		, PlaneCount(D3D12GetFormatPlaneCount(Parent->GetDevice(), Desc.Format))
		, NumSubresources(CalculateNumSubresources())
		, ResourceState(NumSubresources)
	{
		ResourceState.SetSubresourceState(D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, InitialResourceState);
	}

	D3D12Resource::D3D12Resource(
		D3D12LinkedDevice*				 Parent,
		D3D12_HEAP_PROPERTIES			 HeapProperties,
		D3D12_RESOURCE_DESC				 Desc,
		D3D12_RESOURCE_STATES			 InitialResourceState,
		std::optional<D3D12_CLEAR_VALUE> ClearValue)
		: D3D12LinkedDeviceChild(Parent)
		, Resource(InitializeResource(HeapProperties, Desc, InitialResourceState, ClearValue))
		, ClearValue(ClearValue)
		, Desc(Resource->GetDesc())
		, PlaneCount(D3D12GetFormatPlaneCount(Parent->GetDevice(), Desc.Format))
		, NumSubresources(CalculateNumSubresources())
		, ResourceState(NumSubresources)
	{
		ResourceState.SetSubresourceState(D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, InitialResourceState);
	}

	Arc<ID3D12Resource> D3D12Resource::InitializeResource(
		D3D12_HEAP_PROPERTIES			 HeapProperties,
		D3D12_RESOURCE_DESC				 Desc,
		D3D12_RESOURCE_STATES			 InitialResourceState,
		std::optional<D3D12_CLEAR_VALUE> ClearValue) const
	{
		D3D12_CLEAR_VALUE* OptimizedClearValue = ClearValue.has_value() ? &(*ClearValue) : nullptr;

		Arc<ID3D12Resource> Resource;
		VERIFY_D3D12_API(Parent->GetDevice()->CreateCommittedResource(
			&HeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&Desc,
			InitialResourceState,
			OptimizedClearValue,
			IID_PPV_ARGS(Resource.ReleaseAndGetAddressOf())));
		return Resource;
	}

	UINT D3D12Resource::CalculateNumSubresources() const
	{
		if (Desc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER)
		{
			return Desc.MipLevels * Desc.DepthOrArraySize * PlaneCount;
		}
		// Buffer only contains 1 subresource
		return 1;
	}

	D3D12ASBuffer::D3D12ASBuffer(D3D12LinkedDevice* Parent, UINT64 SizeInBytes)
		: D3D12Resource(
			  Parent,
			  CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT, Parent->GetNodeMask(), Parent->GetNodeMask()),
			  CD3DX12_RESOURCE_DESC::Buffer(SizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
			  D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
			  std::nullopt)
	{
	}

	D3D12_GPU_VIRTUAL_ADDRESS D3D12ASBuffer::GetGpuVirtualAddress() const
	{
		return Resource->GetGPUVirtualAddress();
	}

	D3D12Buffer::D3D12Buffer(
		D3D12LinkedDevice*	 Parent,
		UINT64				 SizeInBytes,
		UINT				 Stride,
		D3D12_HEAP_TYPE		 HeapType,
		D3D12_RESOURCE_FLAGS ResourceFlags)
		: D3D12Resource(
			  Parent,
			  CD3DX12_HEAP_PROPERTIES(HeapType, Parent->GetNodeMask(), Parent->GetNodeMask()),
			  CD3DX12_RESOURCE_DESC::Buffer(SizeInBytes, ResourceFlags),
			  ResourceStateDeterminer(CD3DX12_RESOURCE_DESC::Buffer(SizeInBytes, ResourceFlags), HeapType).InferInitialState(),
			  std::nullopt)
		, HeapType(HeapType)
		, Stride(Stride)
		, ScopedPointer(HeapType == D3D12_HEAP_TYPE_UPLOAD ? Resource.Get() : nullptr)
		// We do not need to unmap until we are done with the resource.  However, we must not write to
		// the resource while it is in use by the GPU (so we must use synchronization techniques).
		, CpuVirtualAddress(ScopedPointer.Address)
	{
	}

	D3D12_GPU_VIRTUAL_ADDRESS D3D12Buffer::GetGpuVirtualAddress() const
	{
		return Resource->GetGPUVirtualAddress();
	}

	D3D12_GPU_VIRTUAL_ADDRESS D3D12Buffer::GetGpuVirtualAddress(UINT Index) const
	{
		return Resource->GetGPUVirtualAddress() + static_cast<UINT64>(Index) * Stride;
	}

	D3D12Texture::D3D12Texture(
		D3D12LinkedDevice*	  Parent,
		Arc<ID3D12Resource>&& Resource,
		D3D12_RESOURCE_STATES InitialResourceState)
		: D3D12Resource(Parent, std::move(Resource), InitialResourceState)
	{
	}

	D3D12Texture::D3D12Texture(
		D3D12LinkedDevice*				 Parent,
		const D3D12_RESOURCE_DESC&		 Desc,
		std::optional<D3D12_CLEAR_VALUE> ClearValue /*= std::nullopt*/,
		bool							 Cubemap /*= false*/)
		: D3D12Resource(
			  Parent,
			  CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT, Parent->GetNodeMask(), Parent->GetNodeMask()),
			  Desc,
			  ResourceStateDeterminer(Desc, D3D12_HEAP_TYPE_DEFAULT).InferInitialState(),
			  ClearValue)
		, Cubemap(Cubemap)
	{
		// Textures can only be in device local heap
	}

	UINT D3D12Texture::GetSubresourceIndex(
		std::optional<UINT> OptArraySlice /*= std::nullopt*/,
		std::optional<UINT> OptMipSlice /*= std::nullopt*/,
		std::optional<UINT> OptPlaneSlice /*= std::nullopt*/) const noexcept
	{
		UINT ArraySlice = OptArraySlice.value_or(0);
		UINT MipSlice	= OptMipSlice.value_or(0);
		UINT PlaneSlice = OptPlaneSlice.value_or(0);
		return D3D12CalcSubresource(MipSlice, ArraySlice, PlaneSlice, Desc.MipLevels, Desc.DepthOrArraySize);
	}
} // namespace RHI
