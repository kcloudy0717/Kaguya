#pragma once
#include <compare>
#include <typeindex>
#include <unordered_map>

class RenderGraph;

struct RenderGraphViewData
{
	UINT RenderWidth, RenderHeight;
	UINT ViewportWidth, ViewportHeight;

	template<typename T>
	T GetRenderWidth() const noexcept
	{
		return static_cast<T>(RenderWidth);
	}
	template<typename T>
	T GetRenderHeight() const noexcept
	{
		return static_cast<T>(RenderHeight);
	}
	template<typename T>
	T GetViewportWidth() const noexcept
	{
		return static_cast<T>(ViewportWidth);
	}
	template<typename T>
	T GetViewportHeight() const noexcept
	{
		return static_cast<T>(ViewportHeight);
	}
};

class RenderGraphChild
{
public:
	RenderGraphChild() noexcept
		: Parent(nullptr)
	{
	}

	RenderGraphChild(RenderGraph* Parent) noexcept
		: Parent(Parent)
	{
	}

	RenderGraph* GetParentRenderGraph() const { return Parent; }

	void SetParentRenderGraph(RenderGraph* Parent)
	{
		assert(this->Parent == nullptr);
		this->Parent = Parent;
	}

protected:
	RenderGraph* Parent;
};

enum class ERGResourceType : UINT64
{
	Unknown,

	Buffer,
	Texture,

	RenderPass,
	RenderTarget,

	RootSignature,
	PipelineState,
	RaytracingPipelineState
};

enum class ERGHandleState : UINT64
{
	Dirty,
	Ready
};

// A virtual resource handle, the underlying realization of the
// resource type is done in RenderGraphRegistry, refer to RenderGraph/FrameGraph in Halcyon/Frostbite
struct RenderResourceHandle
{
	auto operator<=>(const RenderResourceHandle&) const = default;

	[[nodiscard]] bool IsValid() const noexcept { return Type != ERGResourceType::Unknown && Id != UINT_MAX; }

	void Invalidate()
	{
		Type	= ERGResourceType::Unknown;
		State	= ERGHandleState::Dirty;
		Version = 0;
		Id		= UINT_MAX;
	}

	ERGResourceType Type	: 16;
	ERGHandleState	State	: 2;
	UINT64			Version : 14;
	UINT64			Id		: 32;
};

static_assert(sizeof(RenderResourceHandle) == sizeof(UINT64));

namespace std
{
template<>
struct hash<RenderResourceHandle>
{
	size_t operator()(const RenderResourceHandle& RenderResourceHandle) const
	{
		return CityHash64((char*)&RenderResourceHandle, sizeof(RenderResourceHandle));
	}
};
} // namespace std

enum EBufferFlags
{
	BufferFlag_None					= 0,
	BufferFlag_AllowUnorderedAccess = 1 << 0
};

struct RGBufferDesc
{
	template<typename T>
	static RGBufferDesc StructuredBuffer(UINT NumElements, EBufferFlags Flags = BufferFlag_None)
	{
		return { .SizeInBytes = static_cast<UINT64>(NumElements) * sizeof(T), .Flags = Flags };
	}

	template<typename T>
	static RGBufferDesc RWStructuredBuffer(UINT NumElements, EBufferFlags Flags = BufferFlag_None)
	{
		return { .SizeInBytes = static_cast<UINT64>(NumElements) * sizeof(T),
				 .Flags		  = Flags | EBufferFlags::BufferFlag_AllowUnorderedAccess };
	}

	UINT64		 SizeInBytes = 0;
	EBufferFlags Flags		 = EBufferFlags::BufferFlag_None;
};

enum class ETextureResolution
{
	Static,	 // Texture resolution is fixed
	Render,	 // Texture resolution is based on render resolution
	Viewport // Texture resolution is based on viewport resolution
};

enum class ETextureType
{
	Texture2D,
	Texture2DArray,
	Texture3D,
	TextureCube
};

enum ETextureFlags
{
	TextureFlag_None				 = 0,
	TextureFlag_AllowRenderTarget	 = 1 << 0,
	TextureFlag_AllowDepthStencil	 = 1 << 1,
	TextureFlag_AllowUnorderedAccess = 1 << 2
};
DEFINE_ENUM_FLAG_OPERATORS(ETextureFlags);

struct RGTextureDesc
{
	[[nodiscard]] static auto RWTexture2D(
		DXGI_FORMAT	  Format,
		UINT		  Width,
		UINT		  Height,
		UINT16		  MipLevels = 1,
		ETextureFlags Flags		= TextureFlag_None) -> RGTextureDesc;
	[[nodiscard]] static auto RWTexture2D(
		ETextureResolution Resolution,
		DXGI_FORMAT		   Format,
		UINT16			   MipLevels = 1,
		ETextureFlags	   Flags	 = TextureFlag_None) -> RGTextureDesc;

	[[nodiscard]] static auto Texture2D(
		DXGI_FORMAT						 Format,
		UINT							 Width,
		UINT							 Height,
		UINT16							 MipLevels			 = 1,
		ETextureFlags					 Flags				 = TextureFlag_None,
		std::optional<D3D12_CLEAR_VALUE> OptimizedClearValue = std::nullopt) -> RGTextureDesc;
	[[nodiscard]] static auto Texture2D(
		ETextureResolution				 Resolution,
		DXGI_FORMAT						 Format,
		UINT16							 MipLevels			 = 1,
		ETextureFlags					 Flags				 = TextureFlag_None,
		std::optional<D3D12_CLEAR_VALUE> OptimizedClearValue = std::nullopt) -> RGTextureDesc;

	[[nodiscard]] bool AllowRenderTarget() const noexcept
	{
		return Flags & ETextureFlags::TextureFlag_AllowRenderTarget;
	}
	[[nodiscard]] bool AllowDepthStencil() const noexcept
	{
		return Flags & ETextureFlags::TextureFlag_AllowDepthStencil;
	}
	[[nodiscard]] bool AllowUnorderedAccess() const noexcept
	{
		return Flags & ETextureFlags::TextureFlag_AllowUnorderedAccess;
	}

	ETextureResolution				 Resolution			 = ETextureResolution::Static;
	DXGI_FORMAT						 Format				 = DXGI_FORMAT_UNKNOWN;
	ETextureType					 Type				 = ETextureType::Texture2D;
	UINT							 Width				 = 1;
	UINT							 Height				 = 1;
	UINT16							 DepthOrArraySize	 = 1;
	UINT16							 MipLevels			 = 1;
	ETextureFlags					 Flags				 = ETextureFlags::TextureFlag_None;
	std::optional<D3D12_CLEAR_VALUE> OptimizedClearValue = std::nullopt;
};
