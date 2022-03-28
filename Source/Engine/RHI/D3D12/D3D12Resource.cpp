#include "D3D12Resource.h"
#include "D3D12LinkedDevice.h"

namespace RHI
{
	// Helper struct used to infer optimal resource state
	struct ResourceStateDeterminer
	{
		ResourceStateDeterminer(const D3D12_RESOURCE_DESC& Desc, D3D12_HEAP_TYPE HeapType) noexcept
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

		[[nodiscard]] D3D12_RESOURCE_STATES InferInitialState() const noexcept
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

	CResourceState::CResourceState(u32 NumSubresources, D3D12_RESOURCE_STATES InitialResourceState)
		: ResourceState(InitialResourceState)
		, SubresourceStates(NumSubresources, InitialResourceState)
	{
		if (NumSubresources == 1)
		{
			TrackingMode = ETrackingMode::PerResource;
		}
		else
		{
			TrackingMode = ETrackingMode::PerSubresource;
		}
	}

	D3D12_RESOURCE_STATES CResourceState::GetSubresourceState(u32 Subresource) const
	{
		if (TrackingMode == ETrackingMode::PerResource)
		{
			return ResourceState;
		}

		return SubresourceStates[Subresource];
	}

	void CResourceState::SetSubresourceState(u32 Subresource, D3D12_RESOURCE_STATES State)
	{
		// If setting all subresources, or the resource only has a single subresource, set the per-resource state
		if (Subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES || SubresourceStates.size() == 1)
		{
			TrackingMode  = ETrackingMode::PerResource;
			ResourceState = State;
		}
		else
		{
			// If we previous tracked resource per resource level, we need to update all
			// all subresource states before proceeding
			if (TrackingMode == ETrackingMode::PerResource)
			{
				TrackingMode = ETrackingMode::PerSubresource;
				for (auto& SubresourceState : SubresourceStates)
				{
					SubresourceState = ResourceState;
				}
			}
			SubresourceStates[Subresource] = State;
		}
	}

	D3D12Resource::D3D12Resource(
		D3D12LinkedDevice*	  Parent,
		Arc<ID3D12Resource>&& Resource,
		D3D12_CLEAR_VALUE	  ClearValue,
		D3D12_RESOURCE_STATES InitialResourceState)
		: D3D12LinkedDeviceChild(Parent)
		, Resource(std::move(Resource))
		, ClearValue(ClearValue)
		, Desc(this->Resource->GetDesc())
		, PlaneCount(D3D12GetFormatPlaneCount(Parent->GetDevice(), Desc.Format))
		, NumSubresources(CalculateNumSubresources())
		, ResourceState(NumSubresources, InitialResourceState)
	{
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
		, ResourceState(NumSubresources, InitialResourceState)
	{
	}

	bool D3D12Resource::ImplicitStatePromotion(D3D12_RESOURCE_STATES State) const noexcept
	{
		// All buffer resources as well as textures with the D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS flag set are
		// implicitly promoted from D3D12_RESOURCE_STATE_COMMON to the relevant state on first GPU access, including
		// GENERIC_READ to cover any read scenario.

		// When this access occurs the promotion acts like an implicit resource barrier. For subsequent accesses, resource
		// barriers will be required to change the resource state if necessary. Note that promotion from one promoted read
		// state into multiple read state is valid, but this is not the case for write states.
		if (Desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
		{
			return true;
		}
		else
		{
			// Simultaneous-Access Textures
			if (Desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS)
			{
				// *Depth-stencil resources must be non-simultaneous-access textures and thus can never be implicitly
				// promoted.
				constexpr D3D12_RESOURCE_STATES NonPromotableStates = D3D12_RESOURCE_STATE_DEPTH_WRITE | D3D12_RESOURCE_STATE_DEPTH_READ;
				if (State & NonPromotableStates)
				{
					return false;
				}
				return true;
			}
			// Non-Simultaneous-Access Textures
			else
			{
				constexpr D3D12_RESOURCE_STATES NonPromotableStates =
					D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER |
					D3D12_RESOURCE_STATE_INDEX_BUFFER |
					D3D12_RESOURCE_STATE_RENDER_TARGET |
					D3D12_RESOURCE_STATE_UNORDERED_ACCESS |
					D3D12_RESOURCE_STATE_DEPTH_WRITE |
					D3D12_RESOURCE_STATE_DEPTH_READ |
					D3D12_RESOURCE_STATE_STREAM_OUT |
					D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT |
					D3D12_RESOURCE_STATE_RESOLVE_DEST |
					D3D12_RESOURCE_STATE_RESOLVE_SOURCE |
					D3D12_RESOURCE_STATE_PREDICATION;

				constexpr D3D12_RESOURCE_STATES PromotableStates =
					D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
					D3D12_RESOURCE_STATE_COPY_DEST |
					D3D12_RESOURCE_STATE_COPY_SOURCE;

				if (State & NonPromotableStates)
				{
					return false;
				}
				else
				{
					UNREFERENCED_PARAMETER(PromotableStates);
					return true;
				}
			}
		}
	}

	bool D3D12Resource::ImplicitStateDecay(D3D12_RESOURCE_STATES State, D3D12_COMMAND_LIST_TYPE AccessedQueueType) const noexcept
	{
		// 1. Resources being accessed on a Copy queue
		if (AccessedQueueType == D3D12_COMMAND_LIST_TYPE_COPY)
		{
			return true;
		}

		// 2. Buffer resources on any queue type
		if (Desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
		{
			return true;
		}

		// 3. Texture resources on any queue type that have the D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS flag set
		if (Desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS)
		{
			return true;
		}

		// 4. Any resource implicitly promoted to a read-only state
		// NOTE: We dont care about buffers here because buffer will decay due to Case 2, only textures, so any read state
		// that is supported by Non-Simultaneous-Access Textures can decay to common
		constexpr D3D12_RESOURCE_STATES ReadOnlyStates =
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
			D3D12_RESOURCE_STATE_COPY_SOURCE;
		if (State & ReadOnlyStates)
		{
			return true;
		}

		return false;
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
		D3D12_CLEAR_VALUE	  ClearValue,
		D3D12_RESOURCE_STATES InitialResourceState)
		: D3D12Resource(Parent, std::move(Resource), ClearValue, InitialResourceState)
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
		// Textures can only be in device local heap (for discrete GPUs) UMA case is not handled
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
