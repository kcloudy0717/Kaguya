#pragma once

class IRHIResource;
class IRHIDescriptorTable;

struct DeviceOptions
{
	bool EnableDebugLayer;
	bool EnableGpuBasedValidation;
	bool EnableAutoDebugName;
};

enum class ERHIFormat
{
	UNKNOWN,

	R8_UINT,
	R8_SINT,
	R8_UNORM,
	R8_SNORM,
	RG8_UINT,
	RG8_SINT,
	RG8_UNORM,
	RG8_SNORM,
	R16_UINT,
	R16_SINT,
	R16_UNORM,
	R16_SNORM,
	R16_FLOAT,
	BGRA4_UNORM,
	B5G6R5_UNORM,
	B5G5R5A1_UNORM,
	RGBA8_UINT,
	RGBA8_SINT,
	RGBA8_UNORM,
	RGBA8_SNORM,
	BGRA8_UNORM,
	SRGBA8_UNORM,
	SBGRA8_UNORM,
	R10G10B10A2_UNORM,
	R11G11B10_FLOAT,
	RG16_UINT,
	RG16_SINT,
	RG16_UNORM,
	RG16_SNORM,
	RG16_FLOAT,
	R32_UINT,
	R32_SINT,
	R32_FLOAT,
	RGBA16_UINT,
	RGBA16_SINT,
	RGBA16_FLOAT,
	RGBA16_UNORM,
	RGBA16_SNORM,
	RG32_UINT,
	RG32_SINT,
	RG32_FLOAT,
	RGB32_UINT,
	RGB32_SINT,
	RGB32_FLOAT,
	RGBA32_UINT,
	RGBA32_SINT,
	RGBA32_FLOAT,

	D16,
	D24S8,
	X24G8_UINT,
	D32,
	D32S8,
	X32G8_UINT,

	BC1_UNORM,
	BC1_UNORM_SRGB,
	BC2_UNORM,
	BC2_UNORM_SRGB,
	BC3_UNORM,
	BC3_UNORM_SRGB,
	BC4_UNORM,
	BC4_SNORM,
	BC5_UNORM,
	BC5_SNORM,
	BC6H_UFLOAT,
	BC6H_SFLOAT,
	BC7_UNORM,
	BC7_UNORM_SRGB,

	COUNT,
};

enum class EDescriptorType
{
	ConstantBuffer,
	Texture,
	RWTexture,
	Sampler,

	NumDescriptorTypes
};

struct DescriptorHandle
{
	[[nodiscard]] bool IsValid() const noexcept { return Index != UINT_MAX; }

	IRHIResource* Resource = nullptr;

	EDescriptorType Type;
	UINT			Index = UINT_MAX;
};

struct DescriptorRange
{
	EDescriptorType Type;
	UINT			NumDescriptors;
	UINT			Binding;
};

struct DescriptorTableDesc
{
	void AddDescriptorRange(const DescriptorRange& Range) { Ranges.emplace_back(Range); }

	std::vector<DescriptorRange> Ranges;
};

struct PushConstant
{
	UINT Size;
};

struct RootSignatureDesc
{
	template<typename T>
	void AddPushConstant()
	{
		PushConstant& PushConstant = PushConstants.emplace_back();
		PushConstant.Size		   = sizeof(T);
	}

	void AddDescriptorTable(IRHIDescriptorTable* DescriptorTable)
	{
		DescriptorTables[NumDescriptorTables++] = DescriptorTable;
	}

	std::vector<PushConstant> PushConstants;
	UINT					  NumDescriptorTables = 0;
	IRHIDescriptorTable*	  DescriptorTables[32];
};

struct DescriptorPoolDesc
{
	UINT NumConstantBufferDescriptors;
	UINT NumShaderResourceDescriptors;
	UINT NumUnorderedAccessDescriptors;
	UINT NumSamplers;

	IRHIDescriptorTable* DescriptorTable;
};

enum class EResourceStates
{
	Common,
	VertexBuffer,
	ConstantBuffer,
	IndexBuffer,
	RenderTarget,
	UnorderedAccess,
	DepthWrite,
	DepthRead,
	NonPixelShaderResource,
	PixelShaderResource,
	StreamOut,
	IndirectArgument,
	CopyDest,
	CopySource,
	ResolveDest,
	ResolveSource,
	RaytracingAccelerationStructure,
	GenericRead,
	Present,
	Predication,
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

struct DepthStencilValue
{
	FLOAT Depth;
	UINT8 Stencil;
};

struct ClearValue
{
	ClearValue() noexcept = default;
	ClearValue(VkFormat Format, FLOAT Color[4])
		: Format(Format)
	{
		std::memcpy(this->Color, Color, sizeof(Color));
	}
	ClearValue(VkFormat Format, FLOAT Depth, UINT8 Stencil)
		: Format(Format)
		, DepthStencil{ Depth, Stencil }
	{
	}

	VkFormat Format = VK_FORMAT_UNDEFINED;
	union
	{
		FLOAT			  Color[4];
		DepthStencilValue DepthStencil;
	};
};

struct RenderPassAttachment
{
	[[nodiscard]] bool IsValid() const noexcept { return Format != VK_FORMAT_UNDEFINED; }

	VkFormat   Format = VK_FORMAT_UNDEFINED;
	ELoadOp	   LoadOp;
	EStoreOp   StoreOp;
	ClearValue ClearValue;
};

struct RenderPassDesc
{
	void AddRenderTarget(RenderPassAttachment RenderTarget) { RenderTargets[NumRenderTargets++] = RenderTarget; }
	void SetDepthStencil(RenderPassAttachment DepthStencil) { this->DepthStencil = DepthStencil; }

	UINT				 NumRenderTargets = 0;
	RenderPassAttachment RenderTargets[8];
	RenderPassAttachment DepthStencil;
};

enum class ERHIHeapType
{
	DeviceLocal,
	Upload,
	Readback
};

enum ERHIBufferFlags
{
	RHIBufferFlag_None				   = 0,
	RHIBufferFlag_ConstantBuffer	   = 1 << 0,
	RHIBufferFlag_VertexBuffer		   = 1 << 1,
	RHIBufferFlag_IndexBuffer		   = 1 << 2,
	RHIBufferFlag_AllowUnorderedAccess = 1 << 3
};
DEFINE_ENUM_FLAG_OPERATORS(ERHIBufferFlags);

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
	ERHIHeapType	HeapType	= ERHIHeapType::DeviceLocal;
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
		ERHIFormat		 Format,
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
		ERHIFormat		 Format,
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
		ERHIFormat		 Format,
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
		ERHIFormat						 Format,
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
		ERHIFormat		 Format,
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
		ERHIFormat		 Format,
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
		ERHIFormat		 Format,
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

	ERHIFormat						 Format				 = ERHIFormat::UNKNOWN;
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
		, ActiveElements(0)
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
		, ActiveElements(std::exchange(TPool.ActiveElements, {}))
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

		ActiveElements++;
		return reinterpret_cast<pointer>(&pElement->Storage);
	}

	void Deallocate(pointer p) noexcept
	{
		const auto pElement = reinterpret_cast<Element*>(p);
		pElement->pNext		= FreeStart;
		FreeStart			= pElement;
		--ActiveElements;
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
	std::size_t				   ActiveElements;
};

enum class ESRVType
{
	Unknown,
	Buffer,
	Texture1D,
	Texture1DArray,
	Texture2D,
	Texture2DArray,
	Texture2DMS,
	Texture2DMSArray,
	Texture3D,
	TextureCube,
	TextureCubeArray,
	RaytracingAccelerationStructure
};
