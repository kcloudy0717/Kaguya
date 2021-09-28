#include "RenderGraphCommon.h"

auto RGTextureDesc::RWTexture2D(
	DXGI_FORMAT	  Format,
	UINT		  Width,
	UINT		  Height,
	UINT16		  MipLevels /*= 1*/,
	ETextureFlags Flags /*= TextureFlag_None*/) -> RGTextureDesc
{
	return {
		.Resolution		  = ETextureResolution::Static,
		.Format			  = Format,
		.Type			  = ETextureType::Texture2D,
		.Width			  = Width,
		.Height			  = Height,
		.DepthOrArraySize = 1,
		.MipLevels		  = MipLevels,
		.Flags			  = Flags | TextureFlag_AllowUnorderedAccess,
	};
}

auto RGTextureDesc::RWTexture2D(
	ETextureResolution Resolution,
	DXGI_FORMAT		   Format,
	UINT16			   MipLevels /*= 1*/,
	ETextureFlags	   Flags /*= TextureFlag_None*/) -> RGTextureDesc
{
	return {
		.Resolution		  = Resolution,
		.Format			  = Format,
		.Type			  = ETextureType::Texture2D,
		.Width			  = 0,
		.Height			  = 0,
		.DepthOrArraySize = 1,
		.MipLevels		  = MipLevels,
		.Flags			  = Flags | TextureFlag_AllowUnorderedAccess,
	};
}

auto RGTextureDesc::Texture2D(
	DXGI_FORMAT						 Format,
	UINT							 Width,
	UINT							 Height,
	UINT16							 MipLevels /*= 1*/,
	ETextureFlags					 Flags /*= TextureFlag_None*/,
	std::optional<D3D12_CLEAR_VALUE> OptimizedClearValue /*= std::nullopt*/) -> RGTextureDesc
{
	return { .Resolution		  = ETextureResolution::Static,
			 .Format			  = Format,
			 .Type				  = ETextureType::Texture2D,
			 .Width				  = Width,
			 .Height			  = Height,
			 .DepthOrArraySize	  = 1,
			 .MipLevels			  = MipLevels,
			 .Flags				  = Flags,
			 .OptimizedClearValue = OptimizedClearValue };
}

auto RGTextureDesc::Texture2D(
	ETextureResolution				 Resolution,
	DXGI_FORMAT						 Format,
	UINT16							 MipLevels /*= 1*/,
	ETextureFlags					 Flags /*= TextureFlag_None*/,
	std::optional<D3D12_CLEAR_VALUE> OptimizedClearValue /*= std::nullopt*/) -> RGTextureDesc
{
	return { .Resolution		  = Resolution,
			 .Format			  = Format,
			 .Type				  = ETextureType::Texture2D,
			 .Width				  = 0,
			 .Height			  = 0,
			 .DepthOrArraySize	  = 1,
			 .MipLevels			  = MipLevels,
			 .Flags				  = Flags,
			 .OptimizedClearValue = OptimizedClearValue };
}
