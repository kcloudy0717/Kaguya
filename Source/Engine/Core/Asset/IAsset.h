#pragma once
#include "System.h"

namespace Asset
{
	enum class AssetType : u64
	{
		Unknown,
		Mesh,
		Texture
	};

	template<typename T>
	struct AssetTypeTraits
	{
		static constexpr AssetType Enum = AssetType::Unknown;
		using ApiType					= void;
	};

	struct AssetHandle
	{
		AssetHandle()
		{
			Invalidate();
		}

		auto operator<=>(const AssetHandle&) const = default;

		[[nodiscard]] bool IsValid() const noexcept { return Type != AssetType::Unknown && Id != UINT_MAX; }

		void Invalidate()
		{
			Type	= AssetType::Unknown;
			State	= false;
			Version = 0;
			Id		= UINT_MAX;
		}

		AssetType Type	  : 16;
		u64		  State	  : 1;
		u64		  Version : 15;
		u64		  Id	  : 32;
	};

	static_assert(sizeof(AssetHandle) == sizeof(u64));

	class IAsset
	{
	public:
		AssetHandle Handle;
	};
} // namespace Asset
