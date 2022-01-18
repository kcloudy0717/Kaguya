#include "D3D12Resource.h"
#include "D3D12LinkedDevice.h"

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
	ARC<ID3D12Resource>&& Resource,
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

ARC<ID3D12Resource> D3D12Resource::InitializeResource(
	D3D12_HEAP_PROPERTIES			 HeapProperties,
	D3D12_RESOURCE_DESC				 Desc,
	D3D12_RESOURCE_STATES			 InitialResourceState,
	std::optional<D3D12_CLEAR_VALUE> ClearValue)
{
	D3D12_CLEAR_VALUE* OptimizedClearValue = ClearValue.has_value() ? &(*ClearValue) : nullptr;

	ARC<ID3D12Resource> Resource;
	VERIFY_D3D12_API(Parent->GetDevice()->CreateCommittedResource(
		&HeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&Desc,
		InitialResourceState,
		OptimizedClearValue,
		IID_PPV_ARGS(Resource.ReleaseAndGetAddressOf())));
	return Resource;
}

UINT D3D12Resource::CalculateNumSubresources()
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
		  CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
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
		  CD3DX12_HEAP_PROPERTIES(HeapType),
		  CD3DX12_RESOURCE_DESC::Buffer(SizeInBytes, ResourceFlags),
		  ResourceStateDeterminer(CD3DX12_RESOURCE_DESC::Buffer(SizeInBytes, ResourceFlags), HeapType).InferInitialState(),
		  std::nullopt)
	, HeapType(HeapType)
	, Stride(Stride)
{
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

D3D12Texture::D3D12Texture(
	D3D12LinkedDevice*	  Parent,
	ARC<ID3D12Resource>&& Resource,
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
		  CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
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
	RenderTargetViewDesc.Format						   = sRGB ? D3D12RHIUtils::MakeSRGB(Desc.Format) : Desc.Format;

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
	DepthStencilViewDesc.Format						   = [](DXGI_FORMAT Format)
	{
		// TODO: Add more
		switch (Format)
		{
		case DXGI_FORMAT_R32_TYPELESS:
			return DXGI_FORMAT_D32_FLOAT;
		default:
			return Format;
		};
	}(Desc.Format);
	DepthStencilViewDesc.Flags = D3D12_DSV_FLAG_NONE;

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
