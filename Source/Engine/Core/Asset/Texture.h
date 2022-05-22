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
		static void SaveToDisk(RHI::D3D12Texture& Texture, const std::filesystem::path& Path);

		void SaveToDisk(const std::filesystem::path& Path);

		void Release();

		TextureImportOptions Options;

		Math::Vec2i Extent;
		bool		IsCubemap = false;

		std::string			  Name;
		DirectX::ScratchImage TexImage;

		RHI::D3D12Texture			 DxTexture;
		RHI::D3D12ShaderResourceView Srv;

	private:
		static void SaveToDDS(RHI::D3D12Texture& Texture, const std::filesystem::path& Path);
	};

	template<>
	struct AssetTypeTraits<Texture>
	{
		static constexpr AssetType Enum = AssetType::Texture;
		using ApiType					= Texture;
	};
} // namespace Asset
