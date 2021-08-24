#pragma once

struct DeviceOptions
{
	bool EnableDebugLayer;
	bool EnableGpuBasedValidation;
	bool EnableAutoDebugName;
};

enum class ELoadOp
{
	Load,
	Clear,
	Noop
};

enum class EStoreOp
{
	Store,
	Noop
};

struct RenderPassAttachment
{
	bool IsValid() const noexcept { return Format != VK_FORMAT_UNDEFINED; }

	VkFormat Format = VK_FORMAT_UNDEFINED;
	ELoadOp	 LoadOp;
	EStoreOp StoreOp;
};

struct RenderPassDesc
{
	void AddRenderTarget(RenderPassAttachment RenderTarget) { RenderTargets[NumRenderTargets++] = RenderTarget; }
	void SetDepthStencil(RenderPassAttachment DepthStencil) { this->DepthStencil = DepthStencil; }

	UINT				 NumRenderTargets = 0;
	RenderPassAttachment RenderTargets[8];
	RenderPassAttachment DepthStencil;
};

enum ERHIBufferFlags
{
	RHIBufferFlag_None				   = 0,
	RHIBufferFlag_VertexBuffer		   = 1 << 0,
	RHIBufferFlag_IndexBuffer		   = 1 << 1,
	RHIBufferFlag_AllowUnorderedAccess = 1 << 2
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
				 .Flags		  = Flags | ERHIBufferFlags::RHIBufferFlag_AllowUnorderedAccess };
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

template<typename T>
class TPool
{
public:
	using value_type = T;
	using pointer	 = T*;

	TPool(std::size_t Size)
		: Size(Size)
	{
		Pool = std::make_unique<Element[]>(Size);

		for (std::size_t i = 1; i < Size; ++i)
		{
			Pool[i - 1].pNext = &Pool[i];
		}
		FreeStart = &Pool[0];
	}

	TPool(TPool&& TPool) noexcept
		: Pool(std::exchange(TPool.Pool, {}))
		, FreeStart(std::exchange(TPool.FreeStart, {}))
		, Size(std::exchange(TPool.Size, {}))
	{
	}
	TPool& operator=(TPool&& TPool) noexcept
	{
		if (this != &TPool)
		{
			Pool	  = std::exchange(TPool.Pool, {});
			FreeStart = std::exchange(TPool.FreeStart, {});
			Size	  = std::exchange(TPool.Size, {});
		}
		return *this;
	}

	TPool(const TPool&) = delete;
	TPool& operator=(const TPool&) = delete;

	[[nodiscard]] pointer Allocate()
	{
		if (!FreeStart)
		{
			throw std::bad_alloc();
		}

		const auto pElement = FreeStart;
		FreeStart			= pElement->pNext;

		return reinterpret_cast<pointer>(&pElement->Storage);
	}

	void Deallocate(pointer p) noexcept
	{
		const auto pElement = reinterpret_cast<Element*>(p);
		pElement->pNext		= FreeStart;
		FreeStart			= pElement;
	}

	template<typename... TArgs>
	[[nodiscard]] pointer Construct(TArgs&&... Args)
	{
		return new (Allocate()) value_type(std::forward<TArgs>(Args)...);
	}

	void Destruct(pointer p) noexcept
	{
		if (p)
		{
			p->~T();
			Deallocate(p);
		}
	}

	void Destroy()
	{
		Pool.reset();
		FreeStart = nullptr;
		Size	  = 0;
	}

private:
	union Element
	{
		std::aligned_storage_t<sizeof(value_type), alignof(value_type)> Storage;
		Element*														pNext;
	};

	std::unique_ptr<Element[]> Pool;
	Element*				   FreeStart;
	std::size_t				   Size;
};
