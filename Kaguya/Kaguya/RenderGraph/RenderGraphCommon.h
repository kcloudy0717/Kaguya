#pragma once
#include <compare>
#include <typeindex>
#include <unordered_map>

class RenderGraph;

struct RenderGraphViewData
{
	UINT RenderWidth, RenderHeight;
	UINT ViewportWidth, ViewportHeight;
};

class RenderScope
{
public:
	// Every RenderScope will have RenderGraphViewData
	RenderScope() { Get<RenderGraphViewData>(); }

	template<typename T>
	T& Get()
	{
		if (auto iter = DataTable.find(typeid(T)); iter != DataTable.end())
		{
			return *reinterpret_cast<T*>(iter->second.get());
		}
		else
		{
			DataTable[typeid(T)] = std::make_unique<BYTE[]>(sizeof(T));
			T& Data				 = *reinterpret_cast<T*>(DataTable[typeid(T)].get());
			Data				 = T(); // Explicit initialization
			return Data;
		}
	}

private:
	std::unordered_map<std::type_index, std::unique_ptr<BYTE[]>> DataTable;
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
	Buffer,
	Texture,

	PipelineState,
	RaytracingPipelineState
};

enum class ERGHandleState : UINT64
{
	Dirty,
	Ready
};

struct RenderResourceHandle
{
	auto operator<=>(const RenderResourceHandle&) const = default;

	ERGResourceType Type  : 16;
	ERGHandleState	State : 16;
	UINT64			Id	  : 32;
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
	BufferFlag_None = 0,
};

struct RGBufferDesc
{
	template<typename T>
	static RGBufferDesc Create(UINT NumElements)
	{
		return {};
	}

	EBufferFlags Flags		 = EBufferFlags::BufferFlag_None;
	UINT64		 SizeInBytes = 0;
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
	static RGTextureDesc RWTexture2D(
		DXGI_FORMAT	  Format,
		UINT		  Width,
		UINT		  Height,
		UINT16		  MipLevels = 1,
		ETextureFlags Flags		= TextureFlag_None)
	{
		return {
			.Format			  = Format,
			.TextureType	  = ETextureType::Texture2D,
			.Width			  = Width,
			.Height			  = Height,
			.DepthOrArraySize = 1,
			.MipLevels		  = MipLevels,
			.Flags			  = Flags | TextureFlag_AllowUnorderedAccess,
		};
	}

	static RGTextureDesc RWTexture2DArray(
		DXGI_FORMAT	  Format,
		UINT		  Width,
		UINT		  Height,
		UINT16		  ArraySize,
		UINT16		  MipLevels = 1,
		ETextureFlags Flags		= TextureFlag_None)
	{
		return {
			.Format			  = Format,
			.TextureType	  = ETextureType::Texture2DArray,
			.Width			  = Width,
			.Height			  = Height,
			.DepthOrArraySize = ArraySize,
			.MipLevels		  = MipLevels,
			.Flags			  = Flags | TextureFlag_AllowUnorderedAccess,
		};
	}

	static RGTextureDesc RWTexture3D(
		DXGI_FORMAT	  Format,
		UINT		  Width,
		UINT		  Height,
		UINT16		  Depth,
		UINT16		  MipLevels = 1,
		ETextureFlags Flags		= TextureFlag_None)
	{
		return {
			.Format			  = Format,
			.TextureType	  = ETextureType::Texture3D,
			.Width			  = Width,
			.Height			  = Height,
			.DepthOrArraySize = Depth,
			.MipLevels		  = MipLevels,
			.Flags			  = Flags | TextureFlag_AllowUnorderedAccess,
		};
	}

	static RGTextureDesc Texture2D(
		DXGI_FORMAT						 Format,
		UINT							 Width,
		UINT							 Height,
		UINT16							 MipLevels			 = 1,
		ETextureFlags					 Flags				 = TextureFlag_None,
		std::optional<D3D12_CLEAR_VALUE> OptimizedClearValue = std::nullopt)
	{
		return { .Format			  = Format,
				 .TextureType		  = ETextureType::Texture2D,
				 .Width				  = Width,
				 .Height			  = Height,
				 .DepthOrArraySize	  = 1,
				 .MipLevels			  = MipLevels,
				 .Flags				  = Flags,
				 .OptimizedClearValue = OptimizedClearValue };
	}

	static RGTextureDesc Texture2DArray(
		DXGI_FORMAT	  Format,
		UINT		  Width,
		UINT		  Height,
		UINT16		  ArraySize,
		UINT16		  MipLevels = 1,
		ETextureFlags Flags		= TextureFlag_None)
	{
		return {
			.Format			  = Format,
			.TextureType	  = ETextureType::Texture2DArray,
			.Width			  = Width,
			.Height			  = Height,
			.DepthOrArraySize = ArraySize,
			.MipLevels		  = MipLevels,
			.Flags			  = Flags,
		};
	}

	static RGTextureDesc Texture3D(
		DXGI_FORMAT	  Format,
		UINT		  Width,
		UINT		  Height,
		UINT16		  Depth,
		UINT16		  MipLevels = 1,
		ETextureFlags Flags		= TextureFlag_None)
	{
		return {
			.Format			  = Format,
			.TextureType	  = ETextureType::Texture3D,
			.Width			  = Width,
			.Height			  = Height,
			.DepthOrArraySize = Depth,
			.MipLevels		  = MipLevels,
			.Flags			  = Flags,
		};
	}

	static RGTextureDesc TextureCube(
		DXGI_FORMAT	  Format,
		UINT		  Dimension,
		UINT16		  MipLevels = 1,
		ETextureFlags Flags		= TextureFlag_None)
	{
		return {
			.Format			  = Format,
			.TextureType	  = ETextureType::TextureCube,
			.Width			  = Dimension,
			.Height			  = Dimension,
			.DepthOrArraySize = 6,
			.MipLevels		  = MipLevels,
			.Flags			  = Flags,
		};
	}

	bool AllowRenderTarget() const noexcept { return Flags & ETextureFlags::TextureFlag_AllowRenderTarget; }
	bool AllowDepthStencil() const noexcept { return Flags & ETextureFlags::TextureFlag_AllowDepthStencil; }
	bool AllowUnorderedAccess() const noexcept { return Flags & ETextureFlags::TextureFlag_AllowUnorderedAccess; }

	ETextureResolution TextureResolution = ETextureResolution::Static;

	DXGI_FORMAT						 Format			  = DXGI_FORMAT_UNKNOWN;
	ETextureType					 TextureType	  = ETextureType::Texture2D;
	UINT							 Width			  = 1;
	UINT							 Height			  = 1;
	UINT16							 DepthOrArraySize = 1;
	UINT16							 MipLevels		  = 1;
	ETextureFlags					 Flags			  = ETextureFlags::TextureFlag_None;
	std::optional<D3D12_CLEAR_VALUE> OptimizedClearValue;
};
