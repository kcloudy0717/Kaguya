#include "D3D12Resource.h"
#include "D3D12LinkedDevice.h"

D3D12_RESOURCE_STATES CResourceState::GetSubresourceState(UINT Subresource) const
{
	if (TrackingMode == ETrackingMode::PerResource)
	{
		return ResourceState;
	}

	return SubresourceStates[Subresource];
}

void CResourceState::SetSubresourceState(UINT Subresource, D3D12_RESOURCE_STATES State)
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
			// clang-format off
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
			// clang-format on

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
	D3D12_HEAP_PROPERTIES HeapProperties	   = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_RESOURCE_STATES InitialResourceState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;

	VERIFY_D3D12_API(Parent->GetDevice()->CreateCommittedResource(
		&HeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&Desc,
		InitialResourceState,
		nullptr,
		IID_PPV_ARGS(Resource.ReleaseAndGetAddressOf())));

	ResourceState = CResourceState(NumSubresources);
	ResourceState.SetSubresourceState(D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, InitialResourceState);
}

D3D12_GPU_VIRTUAL_ADDRESS D3D12ASBuffer::GetGpuVirtualAddress() const
{
	return Resource->GetGPUVirtualAddress();
}

void D3D12ASBuffer::CreateShaderResourceView(D3D12ShaderResourceView& ShaderResourceView) const
{
	D3D12_SHADER_RESOURCE_VIEW_DESC ShaderResourceViewDesc = {};
	ShaderResourceViewDesc.Format						   = DXGI_FORMAT_UNKNOWN;
	ShaderResourceViewDesc.Shader4ComponentMapping		   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	ShaderResourceViewDesc.ViewDimension				   = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
	ShaderResourceViewDesc.RaytracingAccelerationStructure.Location = GetGpuVirtualAddress();

	// When creating descriptor heap based acceleration structure SRVs, the resource parameter must be NULL, as the
	// memory location comes as a GPUVA from the view description (D3D12_RAYTRACING_ACCELERATION_STRUCTURE_SRV) shown
	// below. E.g. CreateShaderResourceView(NULL,pViewDesc).
	ShaderResourceView.Desc = ShaderResourceViewDesc;
	ShaderResourceView.Descriptor.CreateView(ShaderResourceViewDesc, nullptr);
}

void D3D12ASBuffer::CreateNullShaderResourceView(D3D12ShaderResourceView& ShaderResourceView)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC ShaderResourceViewDesc = {};
	ShaderResourceViewDesc.Format						   = DXGI_FORMAT_UNKNOWN;
	ShaderResourceViewDesc.Shader4ComponentMapping		   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	ShaderResourceViewDesc.ViewDimension				   = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
	ShaderResourceViewDesc.RaytracingAccelerationStructure.Location = NULL;

	ShaderResourceView.Desc = ShaderResourceViewDesc;
	ShaderResourceView.Descriptor.CreateView(ShaderResourceViewDesc, nullptr);
}

D3D12Buffer::D3D12Buffer(
	D3D12LinkedDevice*	 Parent,
	UINT64				 SizeInBytes,
	UINT				 Stride,
	D3D12_HEAP_TYPE		 HeapType,
	D3D12_RESOURCE_FLAGS ResourceFlags)
	: D3D12Resource(Parent, CD3DX12_RESOURCE_DESC::Buffer(SizeInBytes, ResourceFlags), std::nullopt, 1)
	, HeapType(HeapType)
	, Stride(Stride)
{
	D3D12_HEAP_PROPERTIES HeapProperties	   = CD3DX12_HEAP_PROPERTIES(HeapType);
	D3D12_RESOURCE_STATES InitialResourceState = ResourceStateDeterminer(Desc, HeapType).InferInitialState();

	VERIFY_D3D12_API(Parent->GetDevice()->CreateCommittedResource(
		&HeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&Desc,
		InitialResourceState,
		nullptr,
		IID_PPV_ARGS(Resource.ReleaseAndGetAddressOf())));

	ResourceState = CResourceState(NumSubresources);
	ResourceState.SetSubresourceState(D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, InitialResourceState);
}

D3D12Buffer::~D3D12Buffer()
{
	if (CpuVirtualAddress)
	{
		Resource->Unmap(0, nullptr);
		CpuVirtualAddress = nullptr;
	}
}

void D3D12Buffer::Initialize()
{
	if (HeapType == D3D12_HEAP_TYPE_UPLOAD)
	{
		VERIFY_D3D12_API(Resource->Map(0, nullptr, reinterpret_cast<void**>(&CpuVirtualAddress)));

		// We do not need to unmap until we are done with the resource.  However, we must not write to
		// the resource while it is in use by the GPU (so we must use synchronization techniques).
	}
}

D3D12_GPU_VIRTUAL_ADDRESS D3D12Buffer::GetGpuVirtualAddress() const
{
	return Resource->GetGPUVirtualAddress();
}

D3D12_GPU_VIRTUAL_ADDRESS D3D12Buffer::GetGpuVirtualAddress(UINT Index) const
{
	return Resource->GetGPUVirtualAddress() + Index * Stride;
}

void D3D12Buffer::CreateShaderResourceView(
	D3D12ShaderResourceView& ShaderResourceView,
	bool					 Raw,
	UINT					 FirstElement,
	UINT					 NumElements) const
{
	D3D12_SHADER_RESOURCE_VIEW_DESC ShaderResourceViewDesc = {};
	ShaderResourceViewDesc.Format						   = DXGI_FORMAT_UNKNOWN;
	ShaderResourceViewDesc.Shader4ComponentMapping		   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	ShaderResourceViewDesc.ViewDimension				   = D3D12_SRV_DIMENSION_BUFFER;
	ShaderResourceViewDesc.Buffer.FirstElement			   = FirstElement;
	ShaderResourceViewDesc.Buffer.NumElements			   = NumElements;
	ShaderResourceViewDesc.Buffer.StructureByteStride	   = Stride;
	ShaderResourceViewDesc.Buffer.Flags					   = D3D12_BUFFER_SRV_FLAG_NONE;

	if (Raw)
	{
		ShaderResourceViewDesc.Format					  = DXGI_FORMAT_R32_TYPELESS;
		ShaderResourceViewDesc.Buffer.FirstElement		  = FirstElement / 4;
		ShaderResourceViewDesc.Buffer.NumElements		  = NumElements / 4;
		ShaderResourceViewDesc.Buffer.StructureByteStride = 0;
		ShaderResourceViewDesc.Buffer.Flags				  = D3D12_BUFFER_SRV_FLAG_RAW;
	}

	ShaderResourceView.Desc = ShaderResourceViewDesc;
	ShaderResourceView.Descriptor.CreateView(ShaderResourceViewDesc, Resource.Get());
}

void D3D12Buffer::CreateUnorderedAccessView(
	D3D12UnorderedAccessView& UnorderedAccessView,
	UINT					  NumElements,
	UINT64					  CounterOffsetInBytes) const
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC UnorderedAccessViewDesc = {};
	UnorderedAccessViewDesc.Format							 = DXGI_FORMAT_UNKNOWN;
	UnorderedAccessViewDesc.ViewDimension					 = D3D12_UAV_DIMENSION_BUFFER;
	UnorderedAccessViewDesc.Buffer.FirstElement				 = 0;
	UnorderedAccessViewDesc.Buffer.NumElements				 = NumElements;
	UnorderedAccessViewDesc.Buffer.StructureByteStride		 = Stride;
	UnorderedAccessViewDesc.Buffer.CounterOffsetInBytes		 = CounterOffsetInBytes;
	UnorderedAccessViewDesc.Buffer.Flags					 = D3D12_BUFFER_UAV_FLAG_NONE;

	UnorderedAccessView.Desc = UnorderedAccessViewDesc;
	UnorderedAccessView.Descriptor.CreateView(UnorderedAccessViewDesc, Resource.Get(), Resource.Get());
}

D3D12Texture::D3D12Texture(
	D3D12LinkedDevice*				 Parent,
	const D3D12_RESOURCE_DESC&		 Desc,
	std::optional<D3D12_CLEAR_VALUE> ClearValue /*= std::nullopt*/,
	bool							 IsCubemap /*= false*/)
	: D3D12Resource(Parent, Desc, ClearValue, 0)
	, IsCubemap(IsCubemap)
{
	// Textures can only be in device local heap
	constexpr D3D12_HEAP_TYPE HeapType = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_HEAP_PROPERTIES HeapProperties	   = CD3DX12_HEAP_PROPERTIES(HeapType);
	D3D12_RESOURCE_STATES InitialResourceState = ResourceStateDeterminer(Desc, HeapType).InferInitialState();
	D3D12_CLEAR_VALUE*	  OptimizedClearValue  = ClearValue.has_value() ? &(*ClearValue) : nullptr;

	VERIFY_D3D12_API(Parent->GetDevice()->CreateCommittedResource(
		&HeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&Desc,
		InitialResourceState,
		OptimizedClearValue,
		IID_PPV_ARGS(Resource.ReleaseAndGetAddressOf())));

	UINT8 PlaneCount = D3D12GetFormatPlaneCount(Parent->GetDevice(), Desc.Format);
	NumSubresources	 = Desc.MipLevels * Desc.DepthOrArraySize * PlaneCount;
	ResourceState	 = CResourceState(NumSubresources);
	ResourceState.SetSubresourceState(D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, InitialResourceState);
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

void D3D12Texture::CreateShaderResourceView(
	D3D12ShaderResourceView& ShaderResourceView,
	std::optional<UINT>		 OptMostDetailedMip /*= std::nullopt*/,
	std::optional<UINT>		 OptMipLevels /*= std::nullopt*/) const
{
	UINT MostDetailedMip = OptMostDetailedMip.value_or(0);
	UINT MipLevels		 = OptMipLevels.value_or(Desc.MipLevels);

	D3D12_SHADER_RESOURCE_VIEW_DESC ShaderResourceViewDesc = {};
	ShaderResourceViewDesc.Format						   = GetValidSRVFormat(Desc.Format);
	ShaderResourceViewDesc.Shader4ComponentMapping		   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	switch (Desc.Dimension)
	{
	case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
	{
		if (Desc.DepthOrArraySize > 1)
		{
			ShaderResourceViewDesc.ViewDimension					  = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
			ShaderResourceViewDesc.Texture2DArray.MostDetailedMip	  = MostDetailedMip;
			ShaderResourceViewDesc.Texture2DArray.MipLevels			  = MipLevels;
			ShaderResourceViewDesc.Texture2DArray.ArraySize			  = Desc.DepthOrArraySize;
			ShaderResourceViewDesc.Texture2DArray.PlaneSlice		  = 0;
			ShaderResourceViewDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
		}
		else
		{
			ShaderResourceViewDesc.ViewDimension				 = D3D12_SRV_DIMENSION_TEXTURE2D;
			ShaderResourceViewDesc.Texture2D.MostDetailedMip	 = MostDetailedMip;
			ShaderResourceViewDesc.Texture2D.MipLevels			 = MipLevels;
			ShaderResourceViewDesc.Texture2D.PlaneSlice			 = 0;
			ShaderResourceViewDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		}

		if (IsCubemap)
		{
			ShaderResourceViewDesc.ViewDimension				   = D3D12_SRV_DIMENSION_TEXTURECUBE;
			ShaderResourceViewDesc.TextureCube.MostDetailedMip	   = MostDetailedMip;
			ShaderResourceViewDesc.TextureCube.MipLevels		   = MipLevels;
			ShaderResourceViewDesc.TextureCube.ResourceMinLODClamp = 0.0f;
		}
	}
	break;

	default:
		break;
	}

	ShaderResourceView.Desc = ShaderResourceViewDesc;
	ShaderResourceView.Descriptor.CreateView(ShaderResourceViewDesc, Resource.Get());
}

void D3D12Texture::CreateUnorderedAccessView(
	D3D12UnorderedAccessView& UnorderedAccessView,
	std::optional<UINT>		  OptArraySlice /*= std::nullopt*/,
	std::optional<UINT>		  OptMipSlice /*= std::nullopt*/) const
{
	UINT ArraySlice = OptArraySlice.value_or(0);
	UINT MipSlice	= OptMipSlice.value_or(0);

	D3D12_UNORDERED_ACCESS_VIEW_DESC UnorderedAccessViewDesc = {};
	UnorderedAccessViewDesc.Format							 = Desc.Format;

	switch (Desc.Dimension)
	{
	case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
		if (Desc.DepthOrArraySize > 1)
		{
			UnorderedAccessViewDesc.ViewDimension				   = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
			UnorderedAccessViewDesc.Texture2DArray.MipSlice		   = MipSlice;
			UnorderedAccessViewDesc.Texture2DArray.FirstArraySlice = ArraySlice;
			UnorderedAccessViewDesc.Texture2DArray.ArraySize	   = Desc.DepthOrArraySize;
			UnorderedAccessViewDesc.Texture2DArray.PlaneSlice	   = 0;
		}
		else
		{
			UnorderedAccessViewDesc.ViewDimension		 = D3D12_UAV_DIMENSION_TEXTURE2D;
			UnorderedAccessViewDesc.Texture2D.MipSlice	 = MipSlice;
			UnorderedAccessViewDesc.Texture2D.PlaneSlice = 0;
		}
		break;

	default:
		break;
	}

	UnorderedAccessView.Desc = UnorderedAccessViewDesc;
	UnorderedAccessView.Descriptor.CreateView(UnorderedAccessViewDesc, Resource.Get(), nullptr);
}

void D3D12Texture::CreateRenderTargetView(
	D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetView,
	std::optional<UINT>			OptArraySlice /*= std::nullopt*/,
	std::optional<UINT>			OptMipSlice /*= std::nullopt*/,
	std::optional<UINT>			OptArraySize /*= std::nullopt*/,
	bool						sRGB /*= false*/) const
{
	UINT ArraySlice = OptArraySlice.value_or(0);
	UINT MipSlice	= OptMipSlice.value_or(0);
	UINT ArraySize	= OptArraySize.value_or(Desc.DepthOrArraySize);

	D3D12_RENDER_TARGET_VIEW_DESC RenderTargetViewDesc = {};
	RenderTargetViewDesc.Format						   = sRGB ? DirectX::MakeSRGB(Desc.Format) : Desc.Format;

	// TODO: Add 1D/3D support
	switch (Desc.Dimension)
	{
	case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
		if (Desc.DepthOrArraySize > 1)
		{
			RenderTargetViewDesc.ViewDimension					= D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
			RenderTargetViewDesc.Texture2DArray.MipSlice		= MipSlice;
			RenderTargetViewDesc.Texture2DArray.FirstArraySlice = ArraySlice;
			RenderTargetViewDesc.Texture2DArray.ArraySize		= ArraySize;
			RenderTargetViewDesc.Texture2DArray.PlaneSlice		= 0;
		}
		else
		{
			RenderTargetViewDesc.ViewDimension		  = D3D12_RTV_DIMENSION_TEXTURE2D;
			RenderTargetViewDesc.Texture2D.MipSlice	  = MipSlice;
			RenderTargetViewDesc.Texture2D.PlaneSlice = 0;
		}
		break;

	default:
		break;
	}
	GetParentLinkedDevice()->GetDevice()->CreateRenderTargetView(
		Resource.Get(),
		&RenderTargetViewDesc,
		RenderTargetView);
}

void D3D12Texture::CreateDepthStencilView(
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView,
	std::optional<UINT>			OptArraySlice /*= std::nullopt*/,
	std::optional<UINT>			OptMipSlice /*= std::nullopt*/,
	std::optional<UINT>			OptArraySize /*= std::nullopt*/) const
{
	UINT ArraySlice = OptArraySlice.value_or(0);
	UINT MipSlice	= OptMipSlice.value_or(0);
	UINT ArraySize	= OptArraySize.value_or(Desc.DepthOrArraySize);

	D3D12_DEPTH_STENCIL_VIEW_DESC DepthStencilViewDesc = {};
	DepthStencilViewDesc.Format						   = GetValidDepthStencilViewFormat(Desc.Format);
	DepthStencilViewDesc.Flags						   = D3D12_DSV_FLAG_NONE;

	switch (Desc.Dimension)
	{
	case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
		if (Desc.DepthOrArraySize > 1)
		{
			DepthStencilViewDesc.ViewDimension					= D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
			DepthStencilViewDesc.Texture2DArray.MipSlice		= MipSlice;
			DepthStencilViewDesc.Texture2DArray.FirstArraySlice = ArraySlice;
			DepthStencilViewDesc.Texture2DArray.ArraySize		= ArraySize;
		}
		else
		{
			DepthStencilViewDesc.ViewDimension		= D3D12_DSV_DIMENSION_TEXTURE2D;
			DepthStencilViewDesc.Texture2D.MipSlice = MipSlice;
		}
		break;

	case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
		assert(false && "Invalid D3D12_RESOURCE_DIMENSION. Dimension: D3D12_RESOURCE_DIMENSION_TEXTURE3D");
		break;

	default:
		break;
	}
	GetParentLinkedDevice()->GetDevice()->CreateDepthStencilView(
		Resource.Get(),
		&DepthStencilViewDesc,
		DepthStencilView);
}
