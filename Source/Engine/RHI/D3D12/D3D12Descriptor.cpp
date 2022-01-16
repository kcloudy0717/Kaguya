#include "D3D12Descriptor.h"
#include "D3D12LinkedDevice.h"

CSubresourceSubset::CSubresourceSubset(
	UINT8  NumMips,
	UINT16 NumArraySlices,
	UINT8  NumPlanes,
	UINT8  FirstMip /*= 0*/,
	UINT16 FirstArraySlice /*= 0*/,
	UINT8  FirstPlane /*= 0*/) noexcept
	: BeginArray(FirstArraySlice)
	, EndArray(FirstArraySlice + NumArraySlices)
	, BeginMip(FirstMip)
	, EndMip(FirstMip + NumMips)
	, BeginPlane(FirstPlane)
	, EndPlane(FirstPlane + NumPlanes)
{
	assert(NumMips > 0 && NumArraySlices > 0 && NumPlanes > 0);
}

CSubresourceSubset::CSubresourceSubset(const D3D12_SHADER_RESOURCE_VIEW_DESC& Desc) noexcept
	: BeginArray(0)
	, EndArray(1)
	, BeginMip(0)
	, EndMip(1)
	, BeginPlane(0)
	, EndPlane(1)
{
	switch (Desc.ViewDimension)
	{
	case D3D12_SRV_DIMENSION_BUFFER:
		break;

	case D3D12_SRV_DIMENSION_TEXTURE1D:
		BeginMip = static_cast<UINT8>(Desc.Texture1D.MostDetailedMip);
		EndMip	 = static_cast<UINT8>(BeginMip + Desc.Texture1D.MipLevels);
		break;

	case D3D12_SRV_DIMENSION_TEXTURE1DARRAY:
		BeginArray = static_cast<UINT16>(Desc.Texture1DArray.FirstArraySlice);
		EndArray   = static_cast<UINT16>(BeginArray + Desc.Texture1DArray.ArraySize);
		BeginMip   = static_cast<UINT8>(Desc.Texture1DArray.MostDetailedMip);
		EndMip	   = static_cast<UINT8>(BeginMip + Desc.Texture1DArray.MipLevels);
		break;

	case D3D12_SRV_DIMENSION_TEXTURE2D:
		BeginMip   = static_cast<UINT8>(Desc.Texture2D.MostDetailedMip);
		EndMip	   = static_cast<UINT8>(BeginMip + Desc.Texture2D.MipLevels);
		BeginPlane = static_cast<UINT8>(Desc.Texture2D.PlaneSlice);
		EndPlane   = static_cast<UINT8>(Desc.Texture2D.PlaneSlice + 1);
		break;

	case D3D12_SRV_DIMENSION_TEXTURE2DARRAY:
		BeginArray = static_cast<UINT16>(Desc.Texture2DArray.FirstArraySlice);
		EndArray   = static_cast<UINT16>(BeginArray + Desc.Texture2DArray.ArraySize);
		BeginMip   = static_cast<UINT8>(Desc.Texture2DArray.MostDetailedMip);
		EndMip	   = static_cast<UINT8>(BeginMip + Desc.Texture2DArray.MipLevels);
		BeginPlane = static_cast<UINT8>(Desc.Texture2DArray.PlaneSlice);
		EndPlane   = static_cast<UINT8>(Desc.Texture2DArray.PlaneSlice + 1);
		break;

	case D3D12_SRV_DIMENSION_TEXTURE2DMS:
		break;

	case D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY:
		BeginArray = static_cast<UINT16>(Desc.Texture2DMSArray.FirstArraySlice);
		EndArray   = static_cast<UINT16>(BeginArray + Desc.Texture2DMSArray.ArraySize);
		break;

	case D3D12_SRV_DIMENSION_TEXTURE3D:
		EndArray = static_cast<UINT16>(-1); // all slices
		BeginMip = static_cast<UINT8>(Desc.Texture3D.MostDetailedMip);
		EndMip	 = static_cast<UINT8>(BeginMip + Desc.Texture3D.MipLevels);
		break;

	case D3D12_SRV_DIMENSION_TEXTURECUBE:
		BeginMip   = static_cast<UINT8>(Desc.TextureCube.MostDetailedMip);
		EndMip	   = static_cast<UINT8>(BeginMip + Desc.TextureCube.MipLevels);
		BeginArray = 0;
		EndArray   = 6;
		break;

	case D3D12_SRV_DIMENSION_TEXTURECUBEARRAY:
		BeginArray = static_cast<UINT16>(Desc.TextureCubeArray.First2DArrayFace);
		EndArray   = static_cast<UINT16>(BeginArray + Desc.TextureCubeArray.NumCubes * 6);
		BeginMip   = static_cast<UINT8>(Desc.TextureCubeArray.MostDetailedMip);
		EndMip	   = static_cast<UINT8>(BeginMip + Desc.TextureCubeArray.MipLevels);
		break;

	case D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE:
		break;

	default:
		assert(false && "Corrupt Resource Type on Shader Resource View");
		break;
	}
}

CSubresourceSubset::CSubresourceSubset(const D3D12_UNORDERED_ACCESS_VIEW_DESC& Desc) noexcept
	: BeginArray(0)
	, EndArray(1)
	, BeginMip(0)
	, BeginPlane(0)
	, EndPlane(1)
{
	switch (Desc.ViewDimension)
	{
	case D3D12_UAV_DIMENSION_BUFFER:
		break;

	case D3D12_UAV_DIMENSION_TEXTURE1D:
		BeginMip = static_cast<UINT8>(Desc.Texture1D.MipSlice);
		break;

	case D3D12_UAV_DIMENSION_TEXTURE1DARRAY:
		BeginArray = static_cast<UINT16>(Desc.Texture1DArray.FirstArraySlice);
		EndArray   = static_cast<UINT16>(BeginArray + Desc.Texture1DArray.ArraySize);
		BeginMip   = static_cast<UINT8>(Desc.Texture1DArray.MipSlice);
		break;

	case D3D12_UAV_DIMENSION_TEXTURE2D:
		BeginMip   = static_cast<UINT8>(Desc.Texture2D.MipSlice);
		BeginPlane = static_cast<UINT8>(Desc.Texture2D.PlaneSlice);
		EndPlane   = static_cast<UINT8>(Desc.Texture2D.PlaneSlice + 1);
		break;

	case D3D12_UAV_DIMENSION_TEXTURE2DARRAY:
		BeginArray = static_cast<UINT16>(Desc.Texture2DArray.FirstArraySlice);
		EndArray   = static_cast<UINT16>(BeginArray + Desc.Texture2DArray.ArraySize);
		BeginMip   = static_cast<UINT8>(Desc.Texture2DArray.MipSlice);
		BeginPlane = static_cast<UINT8>(Desc.Texture2DArray.PlaneSlice);
		EndPlane   = static_cast<UINT8>(Desc.Texture2DArray.PlaneSlice + 1);
		break;

	case D3D12_UAV_DIMENSION_TEXTURE3D:
		BeginArray = static_cast<UINT16>(Desc.Texture3D.FirstWSlice);
		EndArray   = static_cast<UINT16>(BeginArray + Desc.Texture3D.WSize);
		BeginMip   = static_cast<UINT8>(Desc.Texture3D.MipSlice);
		break;

	default:
		assert(false && "Corrupt Resource Type on Unordered Access View");
		break;
	}

	EndMip = BeginMip + 1;
}

CSubresourceSubset::CSubresourceSubset(const D3D12_RENDER_TARGET_VIEW_DESC& Desc) noexcept
	: BeginArray(0)
	, EndArray(1)
	, BeginMip(0)
	, BeginPlane(0)
	, EndPlane(1)
{
	switch (Desc.ViewDimension)
	{
	case D3D12_RTV_DIMENSION_BUFFER:
		break;

	case D3D12_RTV_DIMENSION_TEXTURE1D:
		BeginMip = static_cast<UINT8>(Desc.Texture1D.MipSlice);
		break;

	case D3D12_RTV_DIMENSION_TEXTURE1DARRAY:
		BeginArray = static_cast<UINT16>(Desc.Texture1DArray.FirstArraySlice);
		EndArray   = static_cast<UINT16>(BeginArray + Desc.Texture1DArray.ArraySize);
		BeginMip   = static_cast<UINT8>(Desc.Texture1DArray.MipSlice);
		break;

	case D3D12_RTV_DIMENSION_TEXTURE2D:
		BeginMip   = static_cast<UINT8>(Desc.Texture2D.MipSlice);
		BeginPlane = static_cast<UINT8>(Desc.Texture2D.PlaneSlice);
		EndPlane   = static_cast<UINT8>(Desc.Texture2D.PlaneSlice + 1);
		break;

	case D3D12_RTV_DIMENSION_TEXTURE2DMS:
		break;

	case D3D12_RTV_DIMENSION_TEXTURE2DARRAY:
		BeginArray = static_cast<UINT16>(Desc.Texture2DArray.FirstArraySlice);
		EndArray   = static_cast<UINT16>(BeginArray + Desc.Texture2DArray.ArraySize);
		BeginMip   = static_cast<UINT8>(Desc.Texture2DArray.MipSlice);
		BeginPlane = static_cast<UINT8>(Desc.Texture2DArray.PlaneSlice);
		EndPlane   = static_cast<UINT8>(Desc.Texture2DArray.PlaneSlice + 1);
		break;

	case D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY:
		BeginArray = static_cast<UINT16>(Desc.Texture2DMSArray.FirstArraySlice);
		EndArray   = static_cast<UINT16>(BeginArray + Desc.Texture2DMSArray.ArraySize);
		break;

	case D3D12_RTV_DIMENSION_TEXTURE3D:
		BeginArray = static_cast<UINT16>(Desc.Texture3D.FirstWSlice);
		EndArray   = static_cast<UINT16>(BeginArray + Desc.Texture3D.WSize);
		BeginMip   = static_cast<UINT8>(Desc.Texture3D.MipSlice);
		break;

	default:
		assert(false && "Corrupt Resource Type on Render Target View");
		break;
	}

	EndMip = BeginMip + 1;
}

CSubresourceSubset::CSubresourceSubset(const D3D12_DEPTH_STENCIL_VIEW_DESC& Desc) noexcept
	: BeginArray(0)
	, EndArray(1)
	, BeginMip(0)
	, BeginPlane(0)
	, EndPlane(1)
{
	switch (Desc.ViewDimension)
	{
	case D3D12_DSV_DIMENSION_TEXTURE1D:
		BeginMip = static_cast<UINT8>(Desc.Texture1D.MipSlice);
		break;

	case D3D12_DSV_DIMENSION_TEXTURE1DARRAY:
		BeginArray = static_cast<UINT16>(Desc.Texture1DArray.FirstArraySlice);
		EndArray   = static_cast<UINT16>(BeginArray + Desc.Texture1DArray.ArraySize);
		BeginMip   = static_cast<UINT8>(Desc.Texture1DArray.MipSlice);
		break;

	case D3D12_DSV_DIMENSION_TEXTURE2D:
		BeginMip = static_cast<UINT8>(Desc.Texture2D.MipSlice);
		break;

	case D3D12_DSV_DIMENSION_TEXTURE2DMS:
		break;

	case D3D12_DSV_DIMENSION_TEXTURE2DARRAY:
		BeginArray = static_cast<UINT16>(Desc.Texture2DArray.FirstArraySlice);
		EndArray   = static_cast<UINT16>(BeginArray + Desc.Texture2DArray.ArraySize);
		BeginMip   = static_cast<UINT8>(Desc.Texture2DArray.MipSlice);
		break;

	case D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY:
		BeginArray = static_cast<UINT16>(Desc.Texture2DMSArray.FirstArraySlice);
		EndArray   = static_cast<UINT16>(BeginArray + Desc.Texture2DMSArray.ArraySize);
		break;

	default:
		assert(false && "Corrupt Resource Type on Depth Stencil View");
		break;
	}

	EndMip = BeginMip + 1;
}

bool CSubresourceSubset::DoesNotOverlap(const CSubresourceSubset& CSubresourceSubset) const noexcept
{
	if (EndArray <= CSubresourceSubset.BeginArray)
	{
		return true;
	}

	if (CSubresourceSubset.EndArray <= BeginArray)
	{
		return true;
	}

	if (EndMip <= CSubresourceSubset.BeginMip)
	{
		return true;
	}

	if (CSubresourceSubset.EndMip <= BeginMip)
	{
		return true;
	}

	if (EndPlane <= CSubresourceSubset.BeginPlane)
	{
		return true;
	}

	if (CSubresourceSubset.EndPlane <= BeginPlane)
	{
		return true;
	}

	return false;
}

CViewSubresourceSubset::CViewSubresourceSubset(const CSubresourceSubset& Subresources, UINT8 MipLevels, UINT16 ArraySize, UINT8 PlaneCount)
	: CSubresourceSubset(Subresources)
	, MipLevels(MipLevels)
	, ArraySlices(ArraySize)
	, PlaneCount(PlaneCount)
{
}

CViewSubresourceSubset::CViewSubresourceSubset(
	const D3D12_SHADER_RESOURCE_VIEW_DESC& Desc,
	UINT8								   MipLevels,
	UINT16								   ArraySize,
	UINT8								   PlaneCount)
	: CSubresourceSubset(Desc)
	, MipLevels(MipLevels)
	, ArraySlices(ArraySize)
	, PlaneCount(PlaneCount)
{
	if (Desc.ViewDimension == D3D12_SRV_DIMENSION_TEXTURE3D)
	{
		assert(BeginArray == 0);
		EndArray = 1;
	}
	Reduce();
}

CViewSubresourceSubset::CViewSubresourceSubset(
	const D3D12_UNORDERED_ACCESS_VIEW_DESC& Desc,
	UINT8									MipLevels,
	UINT16									ArraySize,
	UINT8									PlaneCount)
	: CSubresourceSubset(Desc)
	, MipLevels(MipLevels)
	, ArraySlices(ArraySize)
	, PlaneCount(PlaneCount)
{
	if (Desc.ViewDimension == D3D12_UAV_DIMENSION_TEXTURE3D)
	{
		BeginArray = 0;
		EndArray   = 1;
	}
	Reduce();
}

CViewSubresourceSubset::CViewSubresourceSubset(
	const D3D12_RENDER_TARGET_VIEW_DESC& Desc,
	UINT8								 MipLevels,
	UINT16								 ArraySize,
	UINT8								 PlaneCount)
	: CSubresourceSubset(Desc)
	, MipLevels(MipLevels)
	, ArraySlices(ArraySize)
	, PlaneCount(PlaneCount)
{
	if (Desc.ViewDimension == D3D12_RTV_DIMENSION_TEXTURE3D)
	{
		BeginArray = 0;
		EndArray   = 1;
	}
	Reduce();
}

CViewSubresourceSubset::CViewSubresourceSubset(
	const D3D12_DEPTH_STENCIL_VIEW_DESC& Desc,
	UINT8								 MipLevels,
	UINT16								 ArraySize,
	UINT8								 PlaneCount,
	DepthStencilMode					 DSMode)
	: CSubresourceSubset(Desc)
	, MipLevels(MipLevels)
	, ArraySlices(ArraySize)
	, PlaneCount(PlaneCount)
{
	// When this class is used by 11on12 for depthstencil formats, it treats
	// them as planar When binding DSVs of planar resources, additional view
	// subresource subsets will be constructed
	if (PlaneCount == 2)
	{
		if (DSMode != ReadOrWrite)
		{
			bool bWritable = DSMode == WriteOnly;
			bool bDepth	   = !(Desc.Flags & D3D12_DSV_FLAG_READ_ONLY_DEPTH) == bWritable;
			bool bStencil  = !(Desc.Flags & D3D12_DSV_FLAG_READ_ONLY_STENCIL) == bWritable;
			BeginPlane	   = (bDepth ? 0 : 1);
			EndPlane	   = (bStencil ? 2 : 1);
		}
		else
		{
			BeginPlane = 0;
			EndPlane   = 2;
		}
	}

	Reduce();
}

CViewSubresourceSubset::CViewSubresourceIterator CViewSubresourceSubset::begin() const
{
	return CViewSubresourceIterator(*this, BeginArray, BeginPlane);
}

CViewSubresourceSubset::CViewSubresourceIterator CViewSubresourceSubset::end() const
{
	return CViewSubresourceIterator(*this, BeginArray, EndPlane);
}

UINT CViewSubresourceSubset::MinSubresource() const
{
	return (*begin()).first;
}

UINT CViewSubresourceSubset::MaxSubresource() const
{
	return (*(--end())).second;
}

D3D12ShaderResourceView::D3D12ShaderResourceView(
	D3D12LinkedDevice*					   Device,
	const D3D12_SHADER_RESOURCE_VIEW_DESC& Desc,
	D3D12Resource*						   Resource)
	: D3D12View(Device, Desc, Resource)
{
	RecreateView();
}

D3D12ShaderResourceView::D3D12ShaderResourceView(
	D3D12LinkedDevice* Device,
	D3D12ASBuffer*	   ASBuffer)
	: D3D12ShaderResourceView(Device, GetDesc(ASBuffer), ASBuffer)
{
}

D3D12ShaderResourceView::D3D12ShaderResourceView(
	D3D12LinkedDevice* Device,
	D3D12Buffer*	   Buffer,
	bool			   Raw,
	UINT			   FirstElement,
	UINT			   NumElements)
	: D3D12ShaderResourceView(Device, GetDesc(Buffer, Raw, FirstElement, NumElements), Buffer)
{
}

D3D12ShaderResourceView::D3D12ShaderResourceView(
	D3D12LinkedDevice*	Device,
	D3D12Texture*		Texture,
	bool				sRGB,
	std::optional<UINT> OptMostDetailedMip,
	std::optional<UINT> OptMipLevels)
	: D3D12ShaderResourceView(Device, GetDesc(Texture, sRGB, OptMostDetailedMip, OptMipLevels), Texture)
{
}

void D3D12ShaderResourceView::RecreateView()
{
	ID3D12Resource* D3D12Resource = nullptr;
	if (Resource)
	{
		D3D12Resource = Resource->GetResource();
		if (Desc.ViewDimension == D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE)
		{
			D3D12Resource = nullptr;
		}
	}
	Descriptor.CreateView(Desc, D3D12Resource);
}

D3D12_SHADER_RESOURCE_VIEW_DESC D3D12ShaderResourceView::GetDesc(
	D3D12ASBuffer* ASBuffer)
{
	// When creating descriptor heap based acceleration structure SRVs, the
	// resource parameter must be NULL, as the memory location comes as a GPUVA
	// from the view description (D3D12_RAYTRACING_ACCELERATION_STRUCTURE_SRV)
	// shown below. E.g. CreateShaderResourceView(NULL,pViewDesc).

	D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc			 = {};
	SrvDesc.Format									 = DXGI_FORMAT_UNKNOWN;
	SrvDesc.Shader4ComponentMapping					 = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SrvDesc.ViewDimension							 = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
	SrvDesc.RaytracingAccelerationStructure.Location = NULL;
	if (ASBuffer)
	{
		SrvDesc.RaytracingAccelerationStructure.Location = ASBuffer->GetGpuVirtualAddress();
	}
	return SrvDesc;
}

D3D12_SHADER_RESOURCE_VIEW_DESC D3D12ShaderResourceView::GetDesc(
	D3D12Buffer* Buffer,
	bool		 Raw,
	UINT		 FirstElement,
	UINT		 NumElements)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc = {};
	SrvDesc.Format							= DXGI_FORMAT_UNKNOWN;
	SrvDesc.Shader4ComponentMapping			= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SrvDesc.ViewDimension					= D3D12_SRV_DIMENSION_BUFFER;
	SrvDesc.Buffer.FirstElement				= FirstElement;
	SrvDesc.Buffer.NumElements				= NumElements;
	SrvDesc.Buffer.StructureByteStride		= Buffer->GetStride();
	SrvDesc.Buffer.Flags					= D3D12_BUFFER_SRV_FLAG_NONE;

	if (Raw)
	{
		SrvDesc.Format					   = DXGI_FORMAT_R32_TYPELESS;
		SrvDesc.Buffer.FirstElement		   = FirstElement / 4;
		SrvDesc.Buffer.NumElements		   = NumElements / 4;
		SrvDesc.Buffer.StructureByteStride = 0;
		SrvDesc.Buffer.Flags			   = D3D12_BUFFER_SRV_FLAG_RAW;
	}

	return SrvDesc;
}

D3D12_SHADER_RESOURCE_VIEW_DESC D3D12ShaderResourceView::GetDesc(
	D3D12Texture*		Texture,
	bool				sRGB,
	std::optional<UINT> OptMostDetailedMip,
	std::optional<UINT> OptMipLevels)
{
	D3D12_RESOURCE_DESC Desc = Texture->GetDesc();

	UINT MostDetailedMip = OptMostDetailedMip.value_or(0);
	UINT MipLevels		 = OptMipLevels.value_or(Desc.MipLevels);

	D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc = {};
	SrvDesc.Format							= [](DXGI_FORMAT Format, bool sRGB)
	{
		if (sRGB)
		{
			return D3D12RHIUtils::MakeSRGB(Format);
		}
		// TODO: Add more
		switch (Format)
		{
		case DXGI_FORMAT_D32_FLOAT:
			return DXGI_FORMAT_R32_FLOAT;
		}

		return Format;
	}(Desc.Format, sRGB);
	SrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	switch (Desc.Dimension)
	{
	case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
	{
		if (Desc.DepthOrArraySize > 1)
		{
			SrvDesc.ViewDimension					   = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
			SrvDesc.Texture2DArray.MostDetailedMip	   = MostDetailedMip;
			SrvDesc.Texture2DArray.MipLevels		   = MipLevels;
			SrvDesc.Texture2DArray.ArraySize		   = Desc.DepthOrArraySize;
			SrvDesc.Texture2DArray.PlaneSlice		   = 0;
			SrvDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
		}
		else
		{
			SrvDesc.ViewDimension				  = D3D12_SRV_DIMENSION_TEXTURE2D;
			SrvDesc.Texture2D.MostDetailedMip	  = MostDetailedMip;
			SrvDesc.Texture2D.MipLevels			  = MipLevels;
			SrvDesc.Texture2D.PlaneSlice		  = 0;
			SrvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		}

		if (Texture->IsCubemap())
		{
			SrvDesc.ViewDimension					= D3D12_SRV_DIMENSION_TEXTURECUBE;
			SrvDesc.TextureCube.MostDetailedMip		= MostDetailedMip;
			SrvDesc.TextureCube.MipLevels			= MipLevels;
			SrvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
		}
	}
	break;

	default:
		break;
	}

	return SrvDesc;
}

D3D12UnorderedAccessView::D3D12UnorderedAccessView(
	D3D12LinkedDevice*						Device,
	const D3D12_UNORDERED_ACCESS_VIEW_DESC& Desc,
	D3D12Resource*							Resource,
	D3D12Resource*							CounterResource /*= nullptr*/)
	: D3D12View(Device, Desc, Resource)
	, CounterResource(CounterResource)
{
	RecreateView();
}

D3D12UnorderedAccessView::D3D12UnorderedAccessView(
	D3D12LinkedDevice* Device,
	D3D12Buffer*	   Buffer,
	UINT			   NumElements,
	UINT64			   CounterOffsetInBytes)
	: D3D12UnorderedAccessView(Device, GetDesc(Buffer, NumElements, CounterOffsetInBytes), Buffer, Buffer)
{
}

D3D12UnorderedAccessView::D3D12UnorderedAccessView(
	D3D12LinkedDevice*	Device,
	D3D12Texture*		Texture,
	std::optional<UINT> OptArraySlice,
	std::optional<UINT> OptMipSlice)
	: D3D12UnorderedAccessView(Device, GetDesc(Texture, OptArraySlice, OptMipSlice), Texture, nullptr)
{
}

void D3D12UnorderedAccessView::RecreateView()
{
	Descriptor.CreateView(Desc, Resource->GetResource(), CounterResource ? CounterResource->GetResource() : nullptr);
}

D3D12_UNORDERED_ACCESS_VIEW_DESC D3D12UnorderedAccessView::GetDesc(
	D3D12Buffer* Buffer,
	UINT		 NumElements,
	UINT64		 CounterOffsetInBytes)
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC UavDesc = {};
	UavDesc.Format							 = DXGI_FORMAT_UNKNOWN;
	UavDesc.ViewDimension					 = D3D12_UAV_DIMENSION_BUFFER;
	UavDesc.Buffer.FirstElement				 = 0;
	UavDesc.Buffer.NumElements				 = NumElements;
	UavDesc.Buffer.StructureByteStride		 = Buffer->GetStride();
	UavDesc.Buffer.CounterOffsetInBytes		 = CounterOffsetInBytes;
	UavDesc.Buffer.Flags					 = D3D12_BUFFER_UAV_FLAG_NONE;

	return UavDesc;
}

D3D12_UNORDERED_ACCESS_VIEW_DESC D3D12UnorderedAccessView::GetDesc(
	D3D12Texture*		Texture,
	std::optional<UINT> OptArraySlice,
	std::optional<UINT> OptMipSlice)
{
	D3D12_RESOURCE_DESC Desc = Texture->GetDesc();

	UINT ArraySlice = OptArraySlice.value_or(0);
	UINT MipSlice	= OptMipSlice.value_or(0);

	D3D12_UNORDERED_ACCESS_VIEW_DESC UavDesc = {};
	UavDesc.Format							 = Desc.Format;

	switch (Desc.Dimension)
	{
	case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
		if (Desc.DepthOrArraySize > 1)
		{
			UavDesc.ViewDimension				   = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
			UavDesc.Texture2DArray.MipSlice		   = MipSlice;
			UavDesc.Texture2DArray.FirstArraySlice = ArraySlice;
			UavDesc.Texture2DArray.ArraySize	   = Desc.DepthOrArraySize;
			UavDesc.Texture2DArray.PlaneSlice	   = 0;
		}
		else
		{
			UavDesc.ViewDimension		 = D3D12_UAV_DIMENSION_TEXTURE2D;
			UavDesc.Texture2D.MipSlice	 = MipSlice;
			UavDesc.Texture2D.PlaneSlice = 0;
		}
		break;

	default:
		break;
	}

	return UavDesc;
}
