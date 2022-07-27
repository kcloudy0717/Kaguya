#include "D3D12Descriptor.h"
#include "D3D12LinkedDevice.h"

namespace RHI
{
	namespace Internal
	{
		constexpr DXGI_FORMAT MakeSRGB(DXGI_FORMAT Format) noexcept
		{
			switch (Format)
			{
			case DXGI_FORMAT_R8G8B8A8_UNORM:
				return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
			case DXGI_FORMAT_BC1_UNORM:
				return DXGI_FORMAT_BC1_UNORM_SRGB;
			case DXGI_FORMAT_BC2_UNORM:
				return DXGI_FORMAT_BC2_UNORM_SRGB;
			case DXGI_FORMAT_BC3_UNORM:
				return DXGI_FORMAT_BC3_UNORM_SRGB;
			case DXGI_FORMAT_B8G8R8A8_UNORM:
				return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
			case DXGI_FORMAT_B8G8R8X8_UNORM:
				return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
			case DXGI_FORMAT_BC7_UNORM:
				return DXGI_FORMAT_BC7_UNORM_SRGB;
			default:
				return Format;
			}
		}
	} // namespace Internal

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
				bool IsWritable = DSMode == WriteOnly;
				bool IsDepth	= !(Desc.Flags & D3D12_DSV_FLAG_READ_ONLY_DEPTH) == IsWritable;
				bool IsStencil	= !(Desc.Flags & D3D12_DSV_FLAG_READ_ONLY_STENCIL) == IsWritable;
				BeginPlane		= IsDepth ? 0 : 1;
				EndPlane		= IsStencil ? 2 : 1;
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

	void CViewSubresourceSubset::Reduce()
	{
		if (BeginMip == 0 && EndMip == MipLevels && BeginArray == 0 && EndArray == ArraySlices)
		{
			UINT StartSubresource = D3D12CalcSubresource(0, 0, BeginPlane, MipLevels, ArraySlices);
			UINT EndSubresource	  = D3D12CalcSubresource(0, 0, EndPlane, MipLevels, ArraySlices);
			// Only coalesce if the full-resolution UINTs fit in the UINT8s used
			// for storage here
			if (EndSubresource < static_cast<UINT8>(-1))
			{
				BeginArray = 0;
				EndArray   = 1;
				BeginPlane = 0;
				EndPlane   = 1;
				BeginMip   = static_cast<UINT8>(StartSubresource);
				EndMip	   = static_cast<UINT8>(EndSubresource);
			}
		}
	}

	D3D12RenderTargetView::D3D12RenderTargetView(
		D3D12LinkedDevice*					 Device,
		const D3D12_RENDER_TARGET_VIEW_DESC& Desc,
		D3D12Resource*						 Resource)
		: D3D12View(Device, Desc, Resource)
	{
		RecreateView();
	}

	D3D12RenderTargetView::D3D12RenderTargetView(
		D3D12LinkedDevice*		  Device,
		D3D12Texture*			  Texture,
		bool					  sRGB /*= false*/,
		const TextureSubresource& Subresource /*= TextureSubresource{}*/)
		: D3D12RenderTargetView(
			  Device,
			  [&]
			  {
				  D3D12_RESOURCE_DESC Desc = Texture->GetDesc();

				  UINT MipSlice	  = Subresource.MipSlice;
				  UINT ArraySlice = Subresource.ArraySlice;
				  UINT ArraySize  = Subresource.NumArraySlices == TextureSubresource::AllArraySlices ? Desc.DepthOrArraySize : Subresource.NumArraySlices;

				  D3D12_RENDER_TARGET_VIEW_DESC ViewDesc = {};
				  ViewDesc.Format						 = sRGB ? Internal::MakeSRGB(Desc.Format) : Desc.Format;
				  switch (Desc.Dimension)
				  {
				  case D3D12_RESOURCE_DIMENSION_UNKNOWN:
				  case D3D12_RESOURCE_DIMENSION_BUFFER:
				  case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
					  assert(false && "Not implemented");

				  case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
					  if (Desc.DepthOrArraySize > 1)
					  {
						  ViewDesc.ViewDimension				  = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
						  ViewDesc.Texture2DArray.MipSlice		  = MipSlice;
						  ViewDesc.Texture2DArray.FirstArraySlice = ArraySlice;
						  ViewDesc.Texture2DArray.ArraySize		  = ArraySize;
						  ViewDesc.Texture2DArray.PlaneSlice	  = 0;
					  }
					  else
					  {
						  ViewDesc.ViewDimension		= D3D12_RTV_DIMENSION_TEXTURE2D;
						  ViewDesc.Texture2D.MipSlice	= MipSlice;
						  ViewDesc.Texture2D.PlaneSlice = 0;
					  }
					  break;

				  case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
					  assert(false && "Not implemented");
				  }
				  return ViewDesc;
			  }(),
			  Texture)
	{
	}

	void D3D12RenderTargetView::RecreateView()
	{
		ID3D12Resource* D3D12Resource = nullptr;
		if (Resource)
		{
			D3D12Resource = Resource->GetResource();
		}
		Descriptor.CreateView(Desc, D3D12Resource);
	}

	D3D12DepthStencilView::D3D12DepthStencilView(
		D3D12LinkedDevice*					 Device,
		const D3D12_DEPTH_STENCIL_VIEW_DESC& Desc,
		D3D12Resource*						 Resource)
		: D3D12View(Device, Desc, Resource)
	{
		RecreateView();
	}

	D3D12DepthStencilView::D3D12DepthStencilView(
		D3D12LinkedDevice*		  Device,
		D3D12Texture*			  Texture,
		const TextureSubresource& Subresource /*= TextureSubresource{}*/)
		: D3D12DepthStencilView(
			  Device,
			  [&]
			  {
				  D3D12_RESOURCE_DESC Desc = Texture->GetDesc();

				  UINT MipSlice	  = Subresource.MipSlice;
				  UINT ArraySlice = Subresource.ArraySlice;
				  UINT ArraySize  = Subresource.NumArraySlices == TextureSubresource::AllArraySlices ? Desc.DepthOrArraySize : Subresource.NumArraySlices;

				  D3D12_DEPTH_STENCIL_VIEW_DESC ViewDesc = {};
				  ViewDesc.Format						 = [](DXGI_FORMAT Format)
				  {
					  // TODO: Add more
					  switch (Format)
					  {
					  case DXGI_FORMAT_R32_TYPELESS:
						  return DXGI_FORMAT_D32_FLOAT;
					  default:
						  return Format;
					  }
				  }(Desc.Format);
				  ViewDesc.Flags = D3D12_DSV_FLAG_NONE;
				  switch (Desc.Dimension)
				  {
				  case D3D12_RESOURCE_DIMENSION_UNKNOWN:
				  case D3D12_RESOURCE_DIMENSION_BUFFER:
				  case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
					  assert(false && "Not implemented");

				  case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
					  if (Desc.DepthOrArraySize > 1)
					  {
						  ViewDesc.ViewDimension				  = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
						  ViewDesc.Texture2DArray.MipSlice		  = MipSlice;
						  ViewDesc.Texture2DArray.FirstArraySlice = ArraySlice;
						  ViewDesc.Texture2DArray.ArraySize		  = ArraySize;
					  }
					  else
					  {
						  ViewDesc.ViewDimension	  = D3D12_DSV_DIMENSION_TEXTURE2D;
						  ViewDesc.Texture2D.MipSlice = MipSlice;
					  }
					  break;

				  case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
					  assert(false && "Invalid D3D12_RESOURCE_DIMENSION. Dimension: D3D12_RESOURCE_DIMENSION_TEXTURE3D");
					  break;
				  }
				  return ViewDesc;
			  }(),
			  Texture)
	{
	}

	void D3D12DepthStencilView::RecreateView()
	{
		ID3D12Resource* D3D12Resource = nullptr;
		if (Resource)
		{
			D3D12Resource = Resource->GetResource();
		}
		Descriptor.CreateView(Desc, D3D12Resource);
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
		: D3D12ShaderResourceView(
			  Device,
			  [&]
			  {
				  // When creating descriptor heap based acceleration structure SRVs, the
				  // resource parameter must be NULL, as the memory location comes as a GPUVA
				  // from the view description (D3D12_RAYTRACING_ACCELERATION_STRUCTURE_SRV)
				  // shown below. E.g. CreateShaderResourceView(NULL,pViewDesc).

				  D3D12_SHADER_RESOURCE_VIEW_DESC ViewDesc			= {};
				  ViewDesc.Format									= DXGI_FORMAT_UNKNOWN;
				  ViewDesc.Shader4ComponentMapping					= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
				  ViewDesc.ViewDimension							= D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
				  ViewDesc.RaytracingAccelerationStructure.Location = NULL;
				  if (ASBuffer)
				  {
					  ViewDesc.RaytracingAccelerationStructure.Location = ASBuffer->GetGpuVirtualAddress();
				  }
				  return ViewDesc;
			  }(),
			  ASBuffer)
	{
	}

	D3D12ShaderResourceView::D3D12ShaderResourceView(
		D3D12LinkedDevice* Device,
		D3D12Buffer*	   Buffer,
		bool			   Raw,
		UINT			   FirstElement,
		UINT			   NumElements)
		: D3D12ShaderResourceView(
			  Device,
			  [&]
			  {
				  D3D12_SHADER_RESOURCE_VIEW_DESC ViewDesc = {};
				  ViewDesc.Format						   = DXGI_FORMAT_UNKNOWN;
				  ViewDesc.Shader4ComponentMapping		   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
				  ViewDesc.ViewDimension				   = D3D12_SRV_DIMENSION_BUFFER;
				  ViewDesc.Buffer.FirstElement			   = FirstElement;
				  ViewDesc.Buffer.NumElements			   = NumElements;
				  ViewDesc.Buffer.StructureByteStride	   = Buffer->GetStride();
				  ViewDesc.Buffer.Flags					   = D3D12_BUFFER_SRV_FLAG_NONE;
				  if (Raw)
				  {
					  ViewDesc.Format					  = DXGI_FORMAT_R32_TYPELESS;
					  ViewDesc.Buffer.FirstElement		  = FirstElement / 4;
					  ViewDesc.Buffer.NumElements		  = NumElements / 4;
					  ViewDesc.Buffer.StructureByteStride = 0;
					  ViewDesc.Buffer.Flags				  = D3D12_BUFFER_SRV_FLAG_RAW;
				  }
				  return ViewDesc;
			  }(),
			  Buffer)
	{
	}

	D3D12ShaderResourceView::D3D12ShaderResourceView(
		D3D12LinkedDevice*		  Device,
		D3D12Texture*			  Texture,
		bool					  sRGB,
		const TextureSubresource& Subresource)
		: D3D12ShaderResourceView(
			  Device,
			  [&]
			  {
				  D3D12_RESOURCE_DESC Desc = Texture->GetDesc();

				  D3D12_SHADER_RESOURCE_VIEW_DESC ViewDesc = {};
				  ViewDesc.Format						   = [sRGB](DXGI_FORMAT Format)
				  {
					  if (sRGB)
					  {
						  return Internal::MakeSRGB(Format);
					  }
					  // TODO: Add more
					  switch (Format)
					  {
					  case DXGI_FORMAT_D32_FLOAT:
						  return DXGI_FORMAT_R32_FLOAT;
					  }

					  return Format;
				  }(Desc.Format);
				  ViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
				  switch (Desc.Dimension)
				  {
				  case D3D12_RESOURCE_DIMENSION_UNKNOWN:
				  case D3D12_RESOURCE_DIMENSION_BUFFER:
				  case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
					  assert(false && "Not implemented");

				  case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
				  {
					  if (Desc.DepthOrArraySize > 1)
					  {
						  ViewDesc.ViewDimension					  = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
						  ViewDesc.Texture2DArray.MostDetailedMip	  = Subresource.MipSlice;
						  ViewDesc.Texture2DArray.MipLevels			  = Subresource.NumMipLevels;
						  ViewDesc.Texture2DArray.FirstArraySlice	  = Subresource.ArraySlice;
						  ViewDesc.Texture2DArray.ArraySize			  = Subresource.NumArraySlices == TextureSubresource::AllArraySlices ? Desc.DepthOrArraySize : Subresource.NumArraySlices;
						  ViewDesc.Texture2DArray.PlaneSlice		  = 0;
						  ViewDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
					  }
					  else
					  {
						  ViewDesc.ViewDimension				 = D3D12_SRV_DIMENSION_TEXTURE2D;
						  ViewDesc.Texture2D.MostDetailedMip	 = Subresource.MipSlice;
						  ViewDesc.Texture2D.MipLevels			 = Subresource.NumMipLevels;
						  ViewDesc.Texture2D.PlaneSlice			 = 0;
						  ViewDesc.Texture2D.ResourceMinLODClamp = 0.0f;
					  }

					  if (Texture->IsCubemap())
					  {
						  ViewDesc.ViewDimension				   = D3D12_SRV_DIMENSION_TEXTURECUBE;
						  ViewDesc.TextureCube.MostDetailedMip	   = Subresource.MipSlice;
						  ViewDesc.TextureCube.MipLevels		   = Subresource.NumMipLevels;
						  ViewDesc.TextureCube.ResourceMinLODClamp = 0.0f;
					  }
				  }
				  break;

				  case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
					  assert(false && "Not implemented");
				  }
				  return ViewDesc;
			  }(),
			  Texture)
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

	D3D12UnorderedAccessView::D3D12UnorderedAccessView(
		D3D12LinkedDevice*						Device,
		const D3D12_UNORDERED_ACCESS_VIEW_DESC& Desc,
		D3D12Resource*							Resource,
		D3D12Resource*							CounterResource /*= nullptr*/)
		: D3D12View(Device, Desc, Resource)
		, CounterResource(CounterResource)
		, ClearDescriptor(Device)
	{
		RecreateView();
	}

	D3D12UnorderedAccessView::D3D12UnorderedAccessView(
		D3D12LinkedDevice* Device,
		D3D12Buffer*	   Buffer,
		UINT			   NumElements,
		UINT64			   CounterOffsetInBytes)
		: D3D12UnorderedAccessView(
			  Device,
			  [&]
			  {
				  return D3D12_UNORDERED_ACCESS_VIEW_DESC{
					  .Format		 = DXGI_FORMAT_UNKNOWN,
					  .ViewDimension = D3D12_UAV_DIMENSION_BUFFER,
					  .Buffer		 = {
								 .FirstElement		   = 0,
								 .NumElements		   = NumElements,
								 .StructureByteStride  = Buffer->GetStride(),
								 .CounterOffsetInBytes = CounterOffsetInBytes,
								 .Flags				   = D3D12_BUFFER_UAV_FLAG_NONE,
						 },
				  };
			  }(),
			  Buffer,
			  Buffer)
	{
	}

	D3D12UnorderedAccessView::D3D12UnorderedAccessView(
		D3D12LinkedDevice*		  Device,
		D3D12Texture*			  Texture,
		const TextureSubresource& Subresource)
		: D3D12UnorderedAccessView(
			  Device,
			  [&]
			  {
				  D3D12_RESOURCE_DESC Desc = Texture->GetDesc();

				  D3D12_UNORDERED_ACCESS_VIEW_DESC ViewDesc = {};
				  ViewDesc.Format							= Desc.Format;
				  switch (Desc.Dimension)
				  {
				  case D3D12_RESOURCE_DIMENSION_UNKNOWN:
				  case D3D12_RESOURCE_DIMENSION_BUFFER:
				  case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
					  assert(false && "Not implemented");

				  case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
					  if (Desc.DepthOrArraySize > 1)
					  {
						  ViewDesc.ViewDimension				  = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
						  ViewDesc.Texture2DArray.MipSlice		  = Subresource.MipSlice;
						  ViewDesc.Texture2DArray.FirstArraySlice = Subresource.ArraySlice;
						  ViewDesc.Texture2DArray.ArraySize		  = Subresource.NumArraySlices == TextureSubresource::AllArraySlices ? Desc.DepthOrArraySize : Subresource.NumArraySlices;
						  ViewDesc.Texture2DArray.PlaneSlice	  = 0;
					  }
					  else
					  {
						  ViewDesc.ViewDimension		= D3D12_UAV_DIMENSION_TEXTURE2D;
						  ViewDesc.Texture2D.MipSlice	= Subresource.MipSlice;
						  ViewDesc.Texture2D.PlaneSlice = 0;
					  }
					  break;

				  case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
					  assert(false && "Not implemented");
				  }
				  return ViewDesc;
			  }(),
			  Texture,
			  nullptr)
	{
	}

	void D3D12UnorderedAccessView::RecreateView()
	{
		Descriptor.CreateView(Desc, Resource->GetResource(), CounterResource ? CounterResource->GetResource() : nullptr);
		ClearDescriptor.CreateView(Desc, Resource->GetResource(), CounterResource ? CounterResource->GetResource() : nullptr);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE D3D12UnorderedAccessView::GetClearCpuHandle() const noexcept
	{
		return ClearDescriptor.GetCpuHandle();
	}

	D3D12Sampler::D3D12Sampler(
		D3D12LinkedDevice*		  Device,
		const D3D12_SAMPLER_DESC& Desc)
		: D3D12View(Device, Desc, nullptr)
	{
		Descriptor.CreateSampler(Desc);
	}
} // namespace RHI
