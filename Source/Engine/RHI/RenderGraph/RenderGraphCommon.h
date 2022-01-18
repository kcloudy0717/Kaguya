#pragma once
#include <compare>
#include "System/System.h"
#include "RHI/RHI.h"

enum class RgResourceType : u64
{
	Unknown,
	Buffer,
	Texture,
	RenderTarget,
	ShaderResourceView,
	UnorderedAccessView,
	RootSignature,
	PipelineState,
	RaytracingPipelineState,
};

// A virtual resource handle, the underlying realization of the
// resource type is done in RenderGraphRegistry, refer to RenderGraph/FrameGraph in Halcyon/Frostbite
struct RgResourceHandle
{
	auto operator<=>(const RgResourceHandle&) const = default;

	[[nodiscard]] bool IsValid() const noexcept { return Type != RgResourceType::Unknown && Id != UINT_MAX; }

	void Invalidate()
	{
		Type	= RgResourceType::Unknown;
		State	= 0;
		Version = 0;
		Id		= UINT_MAX;
	}

	RgResourceType Type	   : 16; // 16 bit to represent type, might remove some bits from this and give it to version
	u64			   State   : 1;	 // 1 bit to represent state of the handle, true = ready to use, vice versa
	u64			   Version : 15; // 15 bits to represent version should be more than enough, we can always just increase bit used if is not enough
	u64			   Id	   : 32; // 32 bit unsigned int
};

static_assert(sizeof(RgResourceHandle) == sizeof(u64));

template<>
struct std::hash<RgResourceHandle>
{
	size_t operator()(const RgResourceHandle& RenderResourceHandle) const noexcept
	{
		return Hash::Hash64(&RenderResourceHandle, sizeof(RenderResourceHandle));
	}
};

struct RgBufferDesc
{
	RgBufferDesc& SetSize(UINT64 SizeInBytes)
	{
		this->SizeInBytes = SizeInBytes;
		return *this;
	}

	RgBufferDesc& AllowUnorderedAccess()
	{
		UnorderedAccess = true;
		return *this;
	}

	UINT64 SizeInBytes	   = 0;
	bool   UnorderedAccess = false;
};

enum class RgTextureType
{
	Texture2D,
	Texture2DArray,
	Texture3D,
	TextureCube
};

struct RgTextureDesc
{
	RgTextureDesc& SetFormat(DXGI_FORMAT Format)
	{
		this->Format = Format;
		return *this;
	}

	RgTextureDesc& SetType(RgTextureType Type)
	{
		this->Type = Type;
		return *this;
	}

	RgTextureDesc& SetExtent(u32 Width, u32 Height, u32 DepthOrArraySize)
	{
		this->Width			   = Width;
		this->Height		   = Height;
		this->DepthOrArraySize = DepthOrArraySize;
		return *this;
	}

	RgTextureDesc& SetMipLevels(u16 MipLevels)
	{
		this->MipLevels = MipLevels;
		return *this;
	}

	RgTextureDesc& AllowRenderTarget()
	{
		RenderTarget = true;
		return *this;
	}

	RgTextureDesc& AllowDepthStencil()
	{
		DepthStencil = true;
		return *this;
	}

	RgTextureDesc& AllowUnorderedAccess()
	{
		UnorderedAccess = true;
		return *this;
	}

	RgTextureDesc& SetClearValue(D3D12_CLEAR_VALUE ClearValue)
	{
		this->OptimizedClearValue = ClearValue;
		return *this;
	}

	DXGI_FORMAT						 Format				 = DXGI_FORMAT_UNKNOWN;
	RgTextureType					 Type				 = RgTextureType::Texture2D;
	u32								 Width				 = 1;
	u32								 Height				 = 1;
	u32								 DepthOrArraySize	 = 1;
	u16								 MipLevels			 = 1;
	bool							 RenderTarget		 = false;
	bool							 DepthStencil		 = false;
	bool							 UnorderedAccess	 = false;
	std::optional<D3D12_CLEAR_VALUE> OptimizedClearValue = std::nullopt;
};

struct RgRenderTargetDesc
{
	RgRenderTargetDesc& AddRenderTarget(RgResourceHandle RenderTarget, bool sRGB)
	{
		RenderTargets[NumRenderTargets] = RenderTarget;
		this->sRGB[NumRenderTargets]	= sRGB;
		NumRenderTargets++;
		return *this;
	}
	RgRenderTargetDesc& SetDepthStencil(RgResourceHandle DepthStencil)
	{
		this->DepthStencil = DepthStencil;
		return *this;
	}

	u32				 NumRenderTargets = 0;
	RgResourceHandle RenderTargets[8] = {};
	bool			 sRGB[8]		  = {};
	RgResourceHandle DepthStencil;
};

enum class RgViewType
{
	BufferSrv,
	BufferUav,
	TextureSrv,
	TextureUav
};

struct RgBufferSrv
{
	bool Raw;
	u32	 FirstElement;
	u32	 NumElements;
};

struct RgBufferUav
{
	u32 NumElements;
	u64 CounterOffsetInBytes;
};

struct RgTextureSrv
{
	bool sRGB;
	u32	 MostDetailedMip;
	u32	 MipLevels;
};

struct RgTextureUav
{
	u32 ArraySlice;
	u32 MipSlice;
};

struct RgViewDesc
{
	RgViewDesc& SetResource(RgResourceHandle Resource)
	{
		this->Resource = Resource;
		return *this;
	}

	RgViewDesc& AsBufferSrv(
		bool Raw,
		u32	 FirstElement,
		u32	 NumElements)
	{
		Type				   = RgViewType::BufferSrv;
		BufferSrv.Raw		   = Raw;
		BufferSrv.FirstElement = FirstElement;
		BufferSrv.NumElements  = NumElements;
		return *this;
	}

	RgViewDesc& AsBufferUav(
		u32 NumElements,
		u64 CounterOffsetInBytes)
	{
		Type						   = RgViewType::BufferUav;
		BufferUav.NumElements		   = NumElements;
		BufferUav.CounterOffsetInBytes = CounterOffsetInBytes;
		return *this;
	}

	RgViewDesc& AsTextureSrv(
		bool			   sRGB			   = false,
		std::optional<u32> MostDetailedMip = std::nullopt,
		std::optional<u32> MipLevels	   = std::nullopt)
	{
		Type					   = RgViewType::TextureSrv;
		TextureSrv.sRGB			   = sRGB;
		TextureSrv.MostDetailedMip = MostDetailedMip.value_or(-1);
		TextureSrv.MipLevels	   = MipLevels.value_or(-1);
		return *this;
	}

	RgViewDesc& AsTextureUav(
		std::optional<u32> ArraySlice = std::nullopt,
		std::optional<u32> MipSlice	  = std::nullopt)
	{
		Type				  = RgViewType::TextureUav;
		TextureUav.ArraySlice = ArraySlice.value_or(-1);
		TextureUav.MipSlice	  = MipSlice.value_or(-1);
		return *this;
	}

	RgResourceHandle Resource;
	RgViewType		 Type;
	union
	{
		RgBufferSrv	 BufferSrv;
		RgBufferUav	 BufferUav;
		RgTextureSrv TextureSrv;
		RgTextureUav TextureUav;
	};
};

struct RgResource
{
	std::string_view Name;
	RgResourceHandle Handle;
};

struct RgBuffer : RgResource
{
	RgBufferDesc Desc;
};

struct RgTexture : RgResource
{
	RgTextureDesc Desc;
};

struct RgRenderTarget : RgResource
{
	RgRenderTargetDesc Desc;
};

struct RgView : RgResource
{
	RgViewDesc Desc;
};

template<typename T>
struct RgResourceTraits
{
	static constexpr auto Enum = RgResourceType::Unknown;
	using Desc				   = void;
	using Type				   = void;
};

template<>
struct RgResourceTraits<D3D12Buffer>
{
	static constexpr auto Enum = RgResourceType::Buffer;
	using ApiType			   = D3D12Buffer;
	using Desc				   = RgBufferDesc;
	using Type				   = RgBuffer;
};

template<>
struct RgResourceTraits<D3D12Texture>
{
	static constexpr auto Enum = RgResourceType::Texture;
	using ApiType			   = D3D12Texture;
	using Desc				   = RgTextureDesc;
	using Type				   = RgTexture;
};

template<>
struct RgResourceTraits<D3D12RenderTarget>
{
	static constexpr auto Enum = RgResourceType::RenderTarget;
	using ApiType			   = D3D12RenderTarget;
	using Desc				   = RgRenderTargetDesc;
	using Type				   = RgRenderTarget;
};

template<>
struct RgResourceTraits<D3D12ShaderResourceView>
{
	static constexpr auto Enum = RgResourceType::ShaderResourceView;
	using ApiType			   = D3D12ShaderResourceView;
	using Desc				   = RgViewDesc;
	using Type				   = RgView;
};

template<>
struct RgResourceTraits<D3D12UnorderedAccessView>
{
	static constexpr auto Enum = RgResourceType::UnorderedAccessView;
	using ApiType			   = D3D12UnorderedAccessView;
	using Desc				   = RgViewDesc;
	using Type				   = RgView;
};