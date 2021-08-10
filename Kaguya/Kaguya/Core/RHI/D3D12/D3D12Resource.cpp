#include "D3D12Resource.h"
#include "D3D12LinkedDevice.h"

struct ResourceStateDeterminer
{
	ResourceStateDeterminer(const D3D12_RESOURCE_DESC& Desc, D3D12_HEAP_TYPE HeapType)
		: SRV((Desc.Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) == 0)
		, DSV((Desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) != 0)
		, RTV((Desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) != 0)
		, UAV((Desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) != 0)
		, Writable(DSV || RTV || UAV)
		, SRVOnly(SRV && !Writable)
		, Buffer(Desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
		, DeviceLocal(HeapType == D3D12_HEAP_TYPE_DEFAULT)
		, ReadbackResource(HeapType == D3D12_HEAP_TYPE_READBACK)
	{
	}

	D3D12_RESOURCE_STATES GetOptimalInitialState() const
	{
		if (Buffer)
		{
			if (DeviceLocal)
			{
				return D3D12_RESOURCE_STATE_COMMON;
			}

			return (ReadbackResource) ? D3D12_RESOURCE_STATE_COPY_DEST : D3D12_RESOURCE_STATE_GENERIC_READ;
		}
		if (SRVOnly)
		{
			return D3D12_RESOURCE_STATE_COMMON;
		}
		if (Writable)
		{
			if (DSV)
			{
				return D3D12_RESOURCE_STATE_DEPTH_WRITE;
			}
			if (RTV)
			{
				return D3D12_RESOURCE_STATE_RENDER_TARGET;
			}
			if (UAV)
			{
				return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
			}
		}

		return D3D12_RESOURCE_STATE_COMMON;
	}

	const bool SRV;
	const bool DSV;
	const bool RTV;
	const bool UAV;
	const bool Writable;
	const bool SRVOnly;
	const bool Buffer;
	const bool DeviceLocal;
	const bool ReadbackResource;
};

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
			constexpr D3D12_RESOURCE_STATES NonPromotableStates =
				D3D12_RESOURCE_STATE_DEPTH_WRITE | D3D12_RESOURCE_STATE_DEPTH_READ;
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
				D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER | D3D12_RESOURCE_STATE_INDEX_BUFFER |
				D3D12_RESOURCE_STATE_RENDER_TARGET | D3D12_RESOURCE_STATE_UNORDERED_ACCESS |
				D3D12_RESOURCE_STATE_DEPTH_WRITE | D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_STREAM_OUT |
				D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT | D3D12_RESOURCE_STATE_RESOLVE_DEST |
				D3D12_RESOURCE_STATE_RESOLVE_SOURCE | D3D12_RESOURCE_STATE_PREDICATION;

			constexpr D3D12_RESOURCE_STATES PromotableStates =
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
				D3D12_RESOURCE_STATE_COPY_DEST | D3D12_RESOURCE_STATE_COPY_SOURCE;
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

bool D3D12Resource::ImplicitStateDecay(D3D12_RESOURCE_STATES State, D3D12_COMMAND_LIST_TYPE AccessedQueueType)
	const noexcept
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
	if (State & D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE || State & D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE ||
		State & D3D12_RESOURCE_STATE_COPY_SOURCE)
	{
		return true;
	}

	return false;
}

D3D12ASBuffer::D3D12ASBuffer(D3D12LinkedDevice* Parent, UINT64 SizeInBytes)
	: D3D12Resource(
		  Parent,
		  CD3DX12_RESOURCE_DESC::Buffer(SizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
		  std::nullopt,
		  1)
{
	const D3D12_HEAP_PROPERTIES HeapProperties		 = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	const D3D12_RESOURCE_STATES InitialResourceState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;

	VERIFY_D3D12_API(Parent->GetDevice()->CreateCommittedResource(
		&HeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&Desc,
		InitialResourceState,
		nullptr,
		IID_PPV_ARGS(pResource.ReleaseAndGetAddressOf())));

	ResourceState = CResourceState(NumSubresources);
	ResourceState.SetSubresourceState(D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, InitialResourceState);
}

D3D12Buffer::D3D12Buffer(
	D3D12LinkedDevice*	 Parent,
	UINT64				 SizeInBytes,
	UINT				 Stride,
	D3D12_HEAP_TYPE		 HeapType,
	D3D12_RESOURCE_FLAGS ResourceFlags)
	: D3D12Resource(Parent, CD3DX12_RESOURCE_DESC::Buffer(SizeInBytes, ResourceFlags), std::nullopt, 1)
	, Stride(Stride)
	, HeapType(HeapType)
{
	const D3D12_HEAP_PROPERTIES HeapProperties = CD3DX12_HEAP_PROPERTIES(HeapType);

	const D3D12_RESOURCE_STATES InitialResourceState = ResourceStateDeterminer(Desc, HeapType).GetOptimalInitialState();

	VERIFY_D3D12_API(Parent->GetDevice()->CreateCommittedResource(
		&HeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&Desc,
		InitialResourceState,
		nullptr,
		IID_PPV_ARGS(pResource.ReleaseAndGetAddressOf())));

	ResourceState = CResourceState(NumSubresources);
	ResourceState.SetSubresourceState(D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, InitialResourceState);
}

D3D12Buffer::~D3D12Buffer()
{
	if (CPUVirtualAddress)
	{
		pResource->Unmap(0, nullptr);
		CPUVirtualAddress = nullptr;
	}
}

void D3D12Buffer::Initialize()
{
	if (HeapType == D3D12_HEAP_TYPE_UPLOAD)
	{
		VERIFY_D3D12_API(pResource->Map(0, nullptr, reinterpret_cast<void**>(&CPUVirtualAddress)));

		// We do not need to unmap until we are done with the resource.  However, we must not write to
		// the resource while it is in use by the GPU (so we must use synchronization techniques).
	}
}

D3D12Texture::D3D12Texture(
	D3D12LinkedDevice*				 Parent,
	const D3D12_RESOURCE_DESC&		 Desc,
	std::optional<D3D12_CLEAR_VALUE> ClearValue /*= std::nullopt*/)
	: D3D12Resource(Parent, Desc, ClearValue, 0)
{
	const D3D12_HEAP_PROPERTIES HeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	const D3D12_RESOURCE_STATES InitialResourceState =
		ResourceStateDeterminer(Desc, D3D12_HEAP_TYPE_DEFAULT).GetOptimalInitialState();
	const D3D12_CLEAR_VALUE* OptimizedClearValue = ClearValue.has_value() ? &(*ClearValue) : nullptr;

	VERIFY_D3D12_API(Parent->GetDevice()->CreateCommittedResource(
		&HeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&Desc,
		InitialResourceState,
		OptimizedClearValue,
		IID_PPV_ARGS(pResource.ReleaseAndGetAddressOf())));

	UINT8 PlaneCount = D3D12GetFormatPlaneCount(Parent->GetDevice(), Desc.Format);
	NumSubresources	 = Desc.MipLevels * Desc.DepthOrArraySize * PlaneCount;
	ResourceState	 = CResourceState(NumSubresources);
	ResourceState.SetSubresourceState(D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, InitialResourceState);
}

void D3D12Texture::CreateShaderResourceView(
	D3D12ShaderResourceView& ShaderResourceView,
	std::optional<UINT>		 OptMostDetailedMip /*= std::nullopt*/,
	std::optional<UINT>		 OptMipLevels /*= std::nullopt*/)
{
	if (!(Desc.Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE))
	{
		const UINT MostDetailedMip = OptMostDetailedMip.value_or(0);
		const UINT MipLevels	   = OptMipLevels.value_or(Desc.MipLevels);

		D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
		SRVDesc.Format							= GetValidSRVFormat(Desc.Format);
		// SRVDesc.Format							= sRGB ? DirectX::MakeSRGB(SRVDesc.Format) : SRVDesc.Format;
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		switch (Desc.Dimension)
		{
		case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
			if (Desc.DepthOrArraySize > 1)
			{
				SRVDesc.ViewDimension					   = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
				SRVDesc.Texture2DArray.MostDetailedMip	   = MostDetailedMip;
				SRVDesc.Texture2DArray.MipLevels		   = MipLevels;
				SRVDesc.Texture2DArray.ArraySize		   = Desc.DepthOrArraySize;
				SRVDesc.Texture2DArray.PlaneSlice		   = 0;
				SRVDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
			}
			else
			{
				SRVDesc.ViewDimension				  = D3D12_SRV_DIMENSION_TEXTURE2D;
				SRVDesc.Texture2D.MostDetailedMip	  = MostDetailedMip;
				SRVDesc.Texture2D.MipLevels			  = MipLevels;
				SRVDesc.Texture2D.PlaneSlice		  = 0;
				SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;
			}
			break;

		default:
			break;
		}

		ShaderResourceView.Desc = SRVDesc;
		ShaderResourceView.Descriptor.CreateView(SRVDesc, pResource.Get());
	}
}

void D3D12Texture::CreateUnorderedAccessView(
	D3D12UnorderedAccessView& UnorderedAccessView,
	std::optional<UINT>		  OptArraySlice /*= std::nullopt*/,
	std::optional<UINT>		  OptMipSlice /*= std::nullopt*/)
{
	if (Desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
	{
		const UINT ArraySlice = OptArraySlice.value_or(0);
		const UINT MipSlice	  = OptMipSlice.value_or(0);

		D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
		UAVDesc.Format							 = Desc.Format;

		switch (Desc.Dimension)
		{
		case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
			if (Desc.DepthOrArraySize > 1)
			{
				UAVDesc.ViewDimension				   = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
				UAVDesc.Texture2DArray.MipSlice		   = MipSlice;
				UAVDesc.Texture2DArray.FirstArraySlice = ArraySlice;
				UAVDesc.Texture2DArray.ArraySize	   = Desc.DepthOrArraySize;
				UAVDesc.Texture2DArray.PlaneSlice	   = 0;
			}
			else
			{
				UAVDesc.ViewDimension		 = D3D12_UAV_DIMENSION_TEXTURE2D;
				UAVDesc.Texture2D.MipSlice	 = MipSlice;
				UAVDesc.Texture2D.PlaneSlice = 0;
			}
			break;

		default:
			break;
		}

		UnorderedAccessView.Desc = UAVDesc;
		UnorderedAccessView.Descriptor.CreateView(UAVDesc, pResource.Get(), nullptr);
	}
}

void D3D12Texture::CreateRenderTargetView(
	D3D12RenderTargetView& RenderTargetView,
	std::optional<UINT>	   OptArraySlice /*= std::nullopt*/,
	std::optional<UINT>	   OptMipSlice /*= std::nullopt*/,
	std::optional<UINT>	   OptArraySize /*= std::nullopt*/,
	bool				   sRGB /*= false*/)
{
	if (Desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
	{
		const UINT ArraySlice = OptArraySlice.value_or(0);
		const UINT MipSlice	  = OptMipSlice.value_or(0);
		const UINT ArraySize  = OptArraySize.value_or(Desc.DepthOrArraySize);

		D3D12_RENDER_TARGET_VIEW_DESC RTVDesc = {};
		RTVDesc.Format						  = sRGB ? DirectX::MakeSRGB(Desc.Format) : Desc.Format;

		// TODO: Add 1D/3D support
		switch (Desc.Dimension)
		{
		case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
			if (Desc.DepthOrArraySize > 1)
			{
				RTVDesc.ViewDimension				   = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
				RTVDesc.Texture2DArray.MipSlice		   = MipSlice;
				RTVDesc.Texture2DArray.FirstArraySlice = ArraySlice;
				RTVDesc.Texture2DArray.ArraySize	   = ArraySize;
				RTVDesc.Texture2DArray.PlaneSlice	   = 0;
			}
			else
			{
				RTVDesc.ViewDimension		 = D3D12_RTV_DIMENSION_TEXTURE2D;
				RTVDesc.Texture2D.MipSlice	 = MipSlice;
				RTVDesc.Texture2D.PlaneSlice = 0;
			}
			break;

		default:
			break;
		}

		RenderTargetView.Desc = RTVDesc;
		RenderTargetView.Descriptor.CreateView(RTVDesc, pResource.Get());
	}
}

void D3D12Texture::CreateDepthStencilView(
	D3D12DepthStencilView& DepthStencilView,
	std::optional<UINT>	   OptArraySlice /*= std::nullopt*/,
	std::optional<UINT>	   OptMipSlice /*= std::nullopt*/,
	std::optional<UINT>	   OptArraySize /*= std::nullopt*/)
{
	assert(DepthStencilView.Descriptor.IsValid());

	if (Desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
	{
		const UINT ArraySlice = OptArraySlice.value_or(0);
		const UINT MipSlice	  = OptMipSlice.value_or(0);
		const UINT ArraySize  = OptArraySize.value_or(Desc.DepthOrArraySize);

		D3D12_DEPTH_STENCIL_VIEW_DESC DSVDesc = {};
		DSVDesc.Format						  = GetValidDepthStencilViewFormat(Desc.Format);
		DSVDesc.Flags						  = D3D12_DSV_FLAG_NONE;

		switch (Desc.Dimension)
		{
		case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
			if (Desc.DepthOrArraySize > 1)
			{
				DSVDesc.ViewDimension				   = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
				DSVDesc.Texture2DArray.MipSlice		   = MipSlice;
				DSVDesc.Texture2DArray.FirstArraySlice = ArraySlice;
				DSVDesc.Texture2DArray.ArraySize	   = ArraySize;
			}
			else
			{
				DSVDesc.ViewDimension	   = D3D12_DSV_DIMENSION_TEXTURE2D;
				DSVDesc.Texture2D.MipSlice = MipSlice;
			}
			break;

		case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
			throw std::invalid_argument(
				"Invalid D3D12_RESOURCE_DIMENSION. Dimension: D3D12_RESOURCE_DIMENSION_TEXTURE3D");

		default:
			break;
		}

		DepthStencilView.Desc = DSVDesc;
		DepthStencilView.Descriptor.CreateView(DSVDesc, pResource.Get());
	}
}
