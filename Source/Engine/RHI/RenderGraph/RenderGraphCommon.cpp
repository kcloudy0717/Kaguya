#include "RenderGraphCommon.h"

namespace RHI
{
	RgTextureDesc RgTextureDesc::Texture2D(std::string_view Name, DXGI_FORMAT Format, u32 Width, u32 Height, u16 MipLevels, RgClearValue ClearValue, bool AllowRenderTarget, bool AllowDepthStencil, bool AllowUnorderedAccess)
	{
		RgTextureDesc Desc(Name);
		Desc.Type				  = RgTextureType::Texture2D;
		Desc.Format				  = Format;
		Desc.Width				  = Width;
		Desc.Height				  = Height;
		Desc.DepthOrArraySize	  = 1;
		Desc.MipLevels			  = MipLevels;
		Desc.ClearValue			  = ClearValue;
		Desc.AllowRenderTarget	  = AllowRenderTarget;
		Desc.AllowDepthStencil	  = AllowDepthStencil;
		Desc.AllowUnorderedAccess = AllowUnorderedAccess;
		return Desc;
	}

	RgTextureDesc RgTextureDesc::Texture2DArray(std::string_view Name, DXGI_FORMAT Format, u32 Width, u32 Height, u32 ArraySize, u16 MipLevels, RgClearValue ClearValue, bool AllowRenderTarget, bool AllowDepthStencil, bool AllowUnorderedAccess)
	{
		RgTextureDesc Desc(Name);
		Desc.Type				  = RgTextureType::Texture2DArray;
		Desc.Format				  = Format;
		Desc.Width				  = Width;
		Desc.Height				  = Height;
		Desc.DepthOrArraySize	  = ArraySize;
		Desc.MipLevels			  = MipLevels;
		Desc.ClearValue			  = ClearValue;
		Desc.AllowRenderTarget	  = AllowRenderTarget;
		Desc.AllowDepthStencil	  = AllowDepthStencil;
		Desc.AllowUnorderedAccess = AllowUnorderedAccess;
		return Desc;
	}
} // namespace RHI