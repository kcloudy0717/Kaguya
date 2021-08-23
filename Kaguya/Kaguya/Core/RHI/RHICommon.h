#pragma once

struct DeviceOptions
{
	bool EnableDebugLayer;
	bool EnableGpuBasedValidation;
	bool EnableAutoDebugName;
};

enum ERHIBufferFlags
{
	RHIBufferFlag_None				   = 0,
	RHIBufferFlag_AllowUnorderedAccess = 1 << 0
};

struct RHIBufferDesc
{
	template<typename T>
	static RHIBufferDesc StructuredBuffer(UINT NumElements, ERHIBufferFlags Flags = RHIBufferFlag_None)
	{
		return { .SizeInBytes = static_cast<UINT64>(NumElements) * sizeof(T), .Flags = Flags };
	}

	template<typename T>
	static RHIBufferDesc RWStructuredBuffer(UINT NumElements, ERHIBufferFlags Flags = RHIBufferFlag_None)
	{
		return { .SizeInBytes = static_cast<UINT64>(NumElements) * sizeof(T),
				 .Flags		  = Flags | EBufferFlags::RHIBufferFlag_AllowUnorderedAccess };
	}

	UINT64			SizeInBytes = 0;
	ERHIBufferFlags Flags		= ERHIBufferFlags::RHIBufferFlag_None;
};

enum class ERHITextureType
{
	Unknown,
	Texture2D,
	Texture2DArray,
	Texture3D,
	TextureCube
};

enum ERHITextureFlags
{
	RHITextureFlag_None					= 0,
	RHITextureFlag_AllowRenderTarget	= 1 << 0,
	RHITextureFlag_AllowDepthStencil	= 1 << 1,
	RHITextureFlag_AllowUnorderedAccess = 1 << 2
};
DEFINE_ENUM_FLAG_OPERATORS(ERHITextureFlags);

struct RHITextureDesc
{
	[[nodiscard]] static auto RWTexture2D(
		DXGI_FORMAT		 Format,
		UINT			 Width,
		UINT			 Height,
		UINT16			 MipLevels = 1,
		ERHITextureFlags Flags	   = RHITextureFlag_None) -> RHITextureDesc
	{
		return {
			.Format			  = Format,
			.Type			  = ERHITextureType::Texture2D,
			.Width			  = Width,
			.Height			  = Height,
			.DepthOrArraySize = 1,
			.MipLevels		  = MipLevels,
			.Flags			  = Flags | RHITextureFlag_AllowUnorderedAccess,
		};
	}

	[[nodiscard]] static auto RWTexture2DArray(
		DXGI_FORMAT		 Format,
		UINT			 Width,
		UINT			 Height,
		UINT16			 ArraySize,
		UINT16			 MipLevels = 1,
		ERHITextureFlags Flags	   = RHITextureFlag_None) -> RHITextureDesc
	{
		return {
			.Format			  = Format,
			.Type			  = ERHITextureType::Texture2DArray,
			.Width			  = Width,
			.Height			  = Height,
			.DepthOrArraySize = ArraySize,
			.MipLevels		  = MipLevels,
			.Flags			  = Flags | RHITextureFlag_AllowUnorderedAccess,
		};
	}

	[[nodiscard]] static auto RWTexture3D(
		DXGI_FORMAT		 Format,
		UINT			 Width,
		UINT			 Height,
		UINT16			 Depth,
		UINT16			 MipLevels = 1,
		ERHITextureFlags Flags	   = RHITextureFlag_None) -> RHITextureDesc
	{
		return {
			.Format			  = Format,
			.Type			  = ERHITextureType::Texture3D,
			.Width			  = Width,
			.Height			  = Height,
			.DepthOrArraySize = Depth,
			.MipLevels		  = MipLevels,
			.Flags			  = Flags | RHITextureFlag_AllowUnorderedAccess,
		};
	}

	[[nodiscard]] static auto Texture2D(
		DXGI_FORMAT						 Format,
		UINT							 Width,
		UINT							 Height,
		UINT16							 MipLevels			 = 1,
		ERHITextureFlags				 Flags				 = RHITextureFlag_None,
		std::optional<D3D12_CLEAR_VALUE> OptimizedClearValue = std::nullopt) -> RHITextureDesc
	{
		return { .Format			  = Format,
				 .Type				  = ERHITextureType::Texture2D,
				 .Width				  = Width,
				 .Height			  = Height,
				 .DepthOrArraySize	  = 1,
				 .MipLevels			  = MipLevels,
				 .Flags				  = Flags,
				 .OptimizedClearValue = OptimizedClearValue };
	}

	[[nodiscard]] static auto Texture2DArray(
		DXGI_FORMAT		 Format,
		UINT			 Width,
		UINT			 Height,
		UINT16			 ArraySize,
		UINT16			 MipLevels = 1,
		ERHITextureFlags Flags	   = RHITextureFlag_None) -> RHITextureDesc
	{
		return {
			.Format			  = Format,
			.Type			  = ERHITextureType::Texture2DArray,
			.Width			  = Width,
			.Height			  = Height,
			.DepthOrArraySize = ArraySize,
			.MipLevels		  = MipLevels,
			.Flags			  = Flags,
		};
	}

	[[nodiscard]] static auto Texture3D(
		DXGI_FORMAT		 Format,
		UINT			 Width,
		UINT			 Height,
		UINT16			 Depth,
		UINT16			 MipLevels = 1,
		ERHITextureFlags Flags	   = RHITextureFlag_None) -> RHITextureDesc
	{
		return {
			.Format			  = Format,
			.Type			  = ERHITextureType::Texture3D,
			.Width			  = Width,
			.Height			  = Height,
			.DepthOrArraySize = Depth,
			.MipLevels		  = MipLevels,
			.Flags			  = Flags,
		};
	}

	[[nodiscard]] static auto TextureCube(
		DXGI_FORMAT		 Format,
		UINT			 Dimension,
		UINT16			 MipLevels = 1,
		ERHITextureFlags Flags	   = RHITextureFlag_None) -> RHITextureDesc
	{
		return {
			.Format			  = Format,
			.Type			  = ERHITextureType::TextureCube,
			.Width			  = Dimension,
			.Height			  = Dimension,
			.DepthOrArraySize = 6,
			.MipLevels		  = MipLevels,
			.Flags			  = Flags,
		};
	}

	bool AllowRenderTarget() const noexcept { return Flags & ERHITextureFlags::RHITextureFlag_AllowRenderTarget; }
	bool AllowDepthStencil() const noexcept { return Flags & ERHITextureFlags::RHITextureFlag_AllowDepthStencil; }
	bool AllowUnorderedAccess() const noexcept { return Flags & ERHITextureFlags::RHITextureFlag_AllowUnorderedAccess; }

	DXGI_FORMAT						 Format				 = DXGI_FORMAT_UNKNOWN;
	ERHITextureType					 Type				 = ERHITextureType::Unknown;
	UINT							 Width				 = 1;
	UINT							 Height				 = 1;
	UINT16							 DepthOrArraySize	 = 1;
	UINT16							 MipLevels			 = 1;
	ERHITextureFlags				 Flags				 = ERHITextureFlags::RHITextureFlag_None;
	std::optional<D3D12_CLEAR_VALUE> OptimizedClearValue = std::nullopt;
};
