#include "pch.h"
#include "Resource.h"
#include "Device.h"
#include "d3dx12.h"
#include "D3D12Utility.h"

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

bool Resource::ImplicitStatePromotion(D3D12_RESOURCE_STATES State) const
{
	// All buffer resources as well as textures with the D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS flag set are
	// implicitly promoted from D3D12_RESOURCE_STATE_COMMON to the relevant state on first GPU access, including
	// GENERIC_READ to cover any read scenario.

	// When this access occurs the promotion acts like an implicit resource barrier. For subsequent accesses, resource
	// barriers will be required to change the resource state if necessary. Note that promotion from one promoted read
	// state into multiple read state is valid, but this is not the case for write states.
	if (m_Desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
	{
		return true;
	}
	else
	{
		// Simultaneous-Access Textures
		if (m_Desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS)
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

bool Resource::ImplicitStateDecay(D3D12_RESOURCE_STATES State, D3D12_COMMAND_LIST_TYPE AccessedQueueType) const
{
	// 1. Resources being accessed on a Copy queue
	if (AccessedQueueType == D3D12_COMMAND_LIST_TYPE_COPY)
	{
		return true;
	}

	// 2. Buffer resources on any queue type
	if (m_Desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
	{
		return true;
	}

	// 3. Texture resources on any queue type that have the D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS flag set
	if (m_Desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS)
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

Texture::Texture(Device* Device, const D3D12_RESOURCE_DESC& Desc, std::optional<D3D12_CLEAR_VALUE> ClearValue)
	: Texture(Device)
{
	m_Desc		 = Desc;
	m_ClearValue = ClearValue;

	const D3D12_HEAP_PROPERTIES HeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	const D3D12_RESOURCE_STATES InitialResourceState =
		ResourceStateDeterminer(Desc, D3D12_HEAP_TYPE_DEFAULT).GetOptimalInitialState();
	const D3D12_CLEAR_VALUE* OptimizedClearValue = ClearValue.has_value() ? &(*ClearValue) : nullptr;

	Device->GetDevice()->CreateCommittedResource(
		&HeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&Desc,
		InitialResourceState,
		OptimizedClearValue,
		IID_PPV_ARGS(m_Resource.ReleaseAndGetAddressOf()));

	UINT8 PlaneCount  = D3D12GetFormatPlaneCount(Device->GetDevice(), Desc.Format);
	m_NumSubresources = Desc.MipLevels * Desc.DepthOrArraySize * PlaneCount;
	m_ResourceState	  = CResourceState(m_NumSubresources);
	m_ResourceState.SetSubresourceState(D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, InitialResourceState);

	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.Format							= GetValidSRVFormat(Desc.Format);
	SRVDesc.Shader4ComponentMapping			= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	switch (Desc.Dimension)
	{
	case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
		if (Desc.DepthOrArraySize > 1)
		{
			SRVDesc.ViewDimension					   = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
			SRVDesc.Texture2DArray.MostDetailedMip	   = 0;
			SRVDesc.Texture2DArray.MipLevels		   = Desc.MipLevels;
			SRVDesc.Texture2DArray.ArraySize		   = Desc.DepthOrArraySize;
			SRVDesc.Texture2DArray.PlaneSlice		   = 0;
			SRVDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
		}
		else
		{
			SRVDesc.ViewDimension				  = D3D12_SRV_DIMENSION_TEXTURE2D;
			SRVDesc.Texture2D.MostDetailedMip	  = 0;
			SRVDesc.Texture2D.MipLevels			  = Desc.MipLevels;
			SRVDesc.Texture2D.PlaneSlice		  = 0;
			SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		}
		break;

	default:
		break;
	}

	if (!(Desc.Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE))
	{
		SRV = ShaderResourceView(Device, SRVDesc, m_Resource.Get());
	}

	if (Desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
	{
		UAV = UnorderedAccessView(Device, m_Resource.Get());
	}
}

RenderTarget::RenderTarget(Device* Device, UINT Width, UINT Height, DXGI_FORMAT Format, const FLOAT Color[4])
	: Texture(
		  Device,
		  CD3DX12_RESOURCE_DESC::Tex2D(Format, Width, Height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET),
		  CD3DX12_CLEAR_VALUE(Format, Color))
{
	RTV = RenderTargetView(Device, m_Resource.Get());
}

DepthStencil::DepthStencil(Device* Device, UINT Width, UINT Height, DXGI_FORMAT Format, FLOAT Depth, UINT8 Stencil)
	: Texture(
		  Device,
		  CD3DX12_RESOURCE_DESC::Tex2D(Format, Width, Height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
		  CD3DX12_CLEAR_VALUE(Format, Depth, Stencil))
{
	D3D12_DEPTH_STENCIL_VIEW_DESC DSVDesc = {};
	DSVDesc.Format						  = GetValidDepthStencilViewFormat(Format);
	DSVDesc.Flags						  = D3D12_DSV_FLAG_NONE;

	switch (m_Desc.Dimension)
	{
	case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
		if (m_Desc.DepthOrArraySize > 1)
		{
			DSVDesc.ViewDimension				   = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
			DSVDesc.Texture2DArray.MipSlice		   = 0;
			DSVDesc.Texture2DArray.FirstArraySlice = 0;
			DSVDesc.Texture2DArray.ArraySize	   = m_Desc.DepthOrArraySize;
		}
		else
		{
			DSVDesc.ViewDimension	   = D3D12_DSV_DIMENSION_TEXTURE2D;
			DSVDesc.Texture2D.MipSlice = 0;
		}
		break;

	case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
		throw std::invalid_argument("Invalid D3D12_RESOURCE_DIMENSION. Dimension: D3D12_RESOURCE_DIMENSION_TEXTURE3D");

	default:
		break;
	}

	DSV = DepthStencilView(Device, DSVDesc, m_Resource.Get());
}

ASBuffer::ASBuffer(Device* Device, UINT64 SizeInBytes)
	: Resource(Device)
{
	m_Desc = CD3DX12_RESOURCE_DESC::Buffer(SizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	const D3D12_HEAP_PROPERTIES HeapProperties		 = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	const D3D12_RESOURCE_STATES InitialResourceState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;

	Device->GetDevice()->CreateCommittedResource(
		&HeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&m_Desc,
		InitialResourceState,
		nullptr,
		IID_PPV_ARGS(m_Resource.ReleaseAndGetAddressOf()));

	m_NumSubresources = 1;
	m_ResourceState	  = CResourceState(m_NumSubresources);
	m_ResourceState.SetSubresourceState(D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, InitialResourceState);
}

Buffer::Buffer(
	Device*				 Device,
	UINT64				 SizeInBytes,
	UINT				 Stride,
	D3D12_HEAP_TYPE		 HeapType,
	D3D12_RESOURCE_FLAGS ResourceFlags)
	: Buffer(Device)
{
	m_Desc	 = CD3DX12_RESOURCE_DESC::Buffer(SizeInBytes, ResourceFlags);
	m_Stride = Stride;

	const D3D12_HEAP_PROPERTIES HeapProperties = CD3DX12_HEAP_PROPERTIES(HeapType);

	const D3D12_RESOURCE_STATES InitialResourceState =
		ResourceStateDeterminer(m_Desc, HeapType).GetOptimalInitialState();

	Device->GetDevice()->CreateCommittedResource(
		&HeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&m_Desc,
		InitialResourceState,
		nullptr,
		IID_PPV_ARGS(m_Resource.ReleaseAndGetAddressOf()));

	m_NumSubresources = 1;
	m_ResourceState	  = CResourceState(m_NumSubresources);
	m_ResourceState.SetSubresourceState(D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, InitialResourceState);

	if (HeapType == D3D12_HEAP_TYPE_UPLOAD)
	{
		ThrowIfFailed(m_Resource->Map(0, nullptr, reinterpret_cast<void**>(&m_CPUVirtualAddress)));

		// We do not need to unmap until we are done with the resource.  However, we must not write to
		// the resource while it is in use by the GPU (so we must use synchronization techniques).
	}
}

Buffer::~Buffer()
{
	if (m_Resource && m_CPUVirtualAddress)
	{
		m_Resource->Unmap(0, nullptr);
		m_CPUVirtualAddress = nullptr;
	}
}
