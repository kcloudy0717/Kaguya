#pragma once
#include "IAsset.h"
#include "Math/Math.h"
#include "RHI/RHI.h"
#include <DirectXTex.h>

namespace Asset
{
	struct TextureImportOptions
	{
		std::filesystem::path Path;

		bool sRGB		  = false;
		bool GenerateMips = true;
	};

	class Texture : public IAsset
	{
	public:
		void Release()
		{
			TexImage.Release();
		}

		TextureImportOptions Options;

		Vec2i Resolution;
		bool  IsCubemap = false;

		std::string			  Name;
		DirectX::ScratchImage TexImage;

		RHI::D3D12Texture			 DxTexture;
		RHI::D3D12ShaderResourceView SRV;
	};

	template<>
	struct AssetTypeTraits<Texture>
	{
		static constexpr AssetType Enum = AssetType::Texture;
		using ApiType					= Texture;
	};
} // namespace Asset
