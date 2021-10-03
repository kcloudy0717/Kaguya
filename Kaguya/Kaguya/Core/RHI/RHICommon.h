#pragma once
#include "ShaderCompiler.h"
#include "D3D12/d3dx12.h"

#include "RHICore.h"
#include "BlendState.h"
#include "RasterizerState.h"
#include "DepthStencilState.h"

class D3D12RootSignature;
class D3D12RenderPass;

struct DeviceOptions
{
	bool EnableDebugLayer;
	bool EnableGpuBasedValidation;
	bool EnableAutoDebugName;
};

struct RHIViewport
{
	RHIViewport() noexcept = default;
	RHIViewport(
		FLOAT TopLeftX,
		FLOAT TopLeftY,
		FLOAT Width,
		FLOAT Height,
		FLOAT MinDepth = 0.0f,
		FLOAT MaxDepth = 1.0f) noexcept
		: TopLeftX(TopLeftX)
		, TopLeftY(TopLeftY)
		, Width(Width)
		, Height(Height)
		, MinDepth(MinDepth)
		, MaxDepth(MaxDepth)
	{
	}
	RHIViewport(FLOAT Width, FLOAT Height) noexcept
		: RHIViewport(0.0f, 0.0f, Width, Height)
	{
	}

	FLOAT TopLeftX;
	FLOAT TopLeftY;
	FLOAT Width;
	FLOAT Height;
	FLOAT MinDepth;
	FLOAT MaxDepth;
};

struct RHIRect
{
	RHIRect() noexcept = default;
	RHIRect(LONG Left, LONG Top, LONG Right, LONG Bottom) noexcept
		: Left(Left)
		, Top(Top)
		, Right(Right)
		, Bottom(Bottom)
	{
	}
	RHIRect(LONG Width, LONG Height) noexcept
		: RHIRect(0, 0, Width, Height)
	{
	}

	LONG Left;
	LONG Top;
	LONG Right;
	LONG Bottom;
};

struct SamplerDesc
{
	SamplerDesc() noexcept = default;
	SamplerDesc(
		ESamplerFilter		Filter,
		ESamplerAddressMode AddressU,
		ESamplerAddressMode AddressV,
		ESamplerAddressMode AddressW,
		FLOAT				MipLODBias,
		UINT				MaxAnisotropy,
		ComparisonFunc		ComparisonFunc,
		FLOAT				BorderColor,
		FLOAT				MinLOD,
		FLOAT				MaxLOD) noexcept
		: Filter(Filter)
		, AddressU(AddressU)
		, AddressV(AddressV)
		, AddressW(AddressW)
		, MipLODBias(MipLODBias)
		, MaxAnisotropy(MaxAnisotropy)
		, ComparisonFunc(ComparisonFunc)
		, BorderColor(BorderColor)
		, MinLOD(MinLOD)
		, MaxLOD(MaxLOD)
	{
	}

	ESamplerFilter		Filter		  = ESamplerFilter::Point;
	ESamplerAddressMode AddressU	  = ESamplerAddressMode::Wrap;
	ESamplerAddressMode AddressV	  = ESamplerAddressMode::Wrap;
	ESamplerAddressMode AddressW	  = ESamplerAddressMode::Wrap;
	FLOAT				MipLODBias	  = 0.0f;
	UINT				MaxAnisotropy = 0;
	ComparisonFunc		ComparisonFunc;
	FLOAT				BorderColor = 0.0f;
	FLOAT				MinLOD		= 0.0f;
	FLOAT				MaxLOD		= FLT_MAX;
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
	[[nodiscard]] bool IsValid() const noexcept { return Format != DXGI_FORMAT_UNKNOWN; }

	DXGI_FORMAT		  Format = DXGI_FORMAT_UNKNOWN;
	ELoadOp			  LoadOp;
	EStoreOp		  StoreOp;
	D3D12_CLEAR_VALUE ClearValue;
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
	RHIBufferDesc() noexcept = default;
	RHIBufferDesc(UINT64 SizeInBytes, UINT Stride, ERHIHeapType HeapType, ERHIBufferFlags Flags)
		: SizeInBytes(SizeInBytes)
		, Stride(Stride)
		, HeapType(HeapType)
		, Flags(Flags)
	{
	}

	template<typename T>
	static RHIBufferDesc StructuredBuffer(UINT NumElements, ERHIBufferFlags Flags = RHIBufferFlag_None)
	{
		return RHIBufferDesc(static_cast<UINT64>(NumElements) * sizeof(T), sizeof(T), ERHIHeapType::DeviceLocal, Flags);
	}

	template<typename T>
	static RHIBufferDesc RWStructuredBuffer(UINT NumElements, ERHIBufferFlags Flags = RHIBufferFlag_None)
	{
		return RHIBufferDesc(
			static_cast<UINT64>(NumElements) * sizeof(T),
			sizeof(T),
			ERHIHeapType::DeviceLocal,
			Flags | ERHIBufferFlags::RHIBufferFlag_AllowUnorderedAccess);
	}

	UINT64			SizeInBytes = 0;
	UINT			Stride		= 0;
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

	[[nodiscard]] bool AllowRenderTarget() const noexcept
	{
		return Flags & ERHITextureFlags::RHITextureFlag_AllowRenderTarget;
	}
	[[nodiscard]] bool AllowDepthStencil() const noexcept
	{
		return Flags & ERHITextureFlags::RHITextureFlag_AllowDepthStencil;
	}
	[[nodiscard]] bool AllowUnorderedAccess() const noexcept
	{
		return Flags & ERHITextureFlags::RHITextureFlag_AllowUnorderedAccess;
	}

	DXGI_FORMAT						 Format				 = DXGI_FORMAT_UNKNOWN;
	ERHITextureType					 Type				 = ERHITextureType::Unknown;
	UINT							 Width				 = 1;
	UINT							 Height				 = 1;
	UINT16							 DepthOrArraySize	 = 1;
	UINT16							 MipLevels			 = 1;
	ERHITextureFlags				 Flags				 = ERHITextureFlags::RHITextureFlag_None;
	std::optional<D3D12_CLEAR_VALUE> OptimizedClearValue = std::nullopt;
};

class D3D12InputLayout
{
public:
	[[nodiscard]] explicit operator D3D12_INPUT_LAYOUT_DESC() const noexcept;

	D3D12InputLayout() noexcept = default;
	D3D12InputLayout(size_t NumElements) { InputElements.reserve(NumElements); }

	void AddVertexLayoutElement(std::string_view SemanticName, UINT SemanticIndex, DXGI_FORMAT Format, UINT InputSlot);

private:
	std::vector<std::string>					  SemanticNames;
	mutable std::vector<D3D12_INPUT_ELEMENT_DESC> InputElements;
};

enum class PipelineStateSubobjectType
{
	RootSignature,
	VS,
	PS,
	DS,
	HS,
	GS,
	CS,
	BlendState,
	RasterizerState,
	DepthStencilState,
	InputLayout,
	PrimitiveTopology,
	RenderPass,

	NumTypes
};

// PSO desc is inspired by D3D12' PSO stream

template<typename TDesc, PipelineStateSubobjectType TType>
class alignas(void*) PipelineStateStreamSubobject
{
public:
	PipelineStateStreamSubobject() noexcept
		: Type(TType)
		, Desc(TDesc())
	{
	}
	PipelineStateStreamSubobject(const TDesc& Desc) noexcept
		: Type(TType)
		, Desc(Desc)
	{
	}
	PipelineStateStreamSubobject& operator=(const TDesc& Desc) noexcept
	{
		this->Type = TType;
		this->Desc = Desc;
		return *this;
	}
				 operator const TDesc&() const noexcept { return Desc; }
				 operator TDesc&() noexcept { return Desc; }
	TDesc*		 operator&() noexcept { return &Desc; }
	const TDesc* operator&() const noexcept { return &Desc; }

	TDesc& operator->() noexcept { return Desc; }

private:
	PipelineStateSubobjectType Type;
	TDesc					   Desc;
};

// clang-format off
using PipelineStateStreamRootSignature		= PipelineStateStreamSubobject<D3D12RootSignature*, PipelineStateSubobjectType::RootSignature>;
using PipelineStateStreamVS					= PipelineStateStreamSubobject<Shader*, PipelineStateSubobjectType::VS>;
using PipelineStateStreamPS					= PipelineStateStreamSubobject<Shader*, PipelineStateSubobjectType::PS>;
using PipelineStateStreamDS					= PipelineStateStreamSubobject<Shader*, PipelineStateSubobjectType::DS>;
using PipelineStateStreamHS					= PipelineStateStreamSubobject<Shader*, PipelineStateSubobjectType::HS>;
using PipelineStateStreamGS					= PipelineStateStreamSubobject<Shader*, PipelineStateSubobjectType::GS>;
using PipelineStateStreamCS					= PipelineStateStreamSubobject<Shader*, PipelineStateSubobjectType::CS>;
using PipelineStateStreamBlendState			= PipelineStateStreamSubobject<BlendState, PipelineStateSubobjectType::BlendState>;
using PipelineStateStreamRasterizerState	= PipelineStateStreamSubobject<RasterizerState, PipelineStateSubobjectType::RasterizerState>;
using PipelineStateStreamDepthStencilState	= PipelineStateStreamSubobject<DepthStencilState, PipelineStateSubobjectType::DepthStencilState>;
using PipelineStateStreamInputLayout		= PipelineStateStreamSubobject<D3D12InputLayout, PipelineStateSubobjectType::InputLayout>;
using PipelineStateStreamPrimitiveTopology	= PipelineStateStreamSubobject<PrimitiveTopology, PipelineStateSubobjectType::PrimitiveTopology>;
using PipelineStateStreamRenderPass			= PipelineStateStreamSubobject<D3D12RenderPass*, PipelineStateSubobjectType::RenderPass>;
// clang-format on

class IPipelineParserCallbacks
{
public:
	virtual ~IPipelineParserCallbacks() = default;

	// Subobject Callbacks
	virtual void RootSignatureCb(D3D12RootSignature*) {}
	virtual void VSCb(Shader*) {}
	virtual void PSCb(Shader*) {}
	virtual void DSCb(Shader*) {}
	virtual void HSCb(Shader*) {}
	virtual void GSCb(Shader*) {}
	virtual void CSCb(Shader*) {}
	virtual void BlendStateCb(const BlendState&) {}
	virtual void RasterizerStateCb(const RasterizerState&) {}
	virtual void DepthStencilStateCb(const DepthStencilState&) {}
	virtual void InputLayoutCb(const D3D12InputLayout&) {}
	virtual void PrimitiveTopologyTypeCb(PrimitiveTopology) {}
	virtual void RenderPassCb(D3D12RenderPass*) {}

	// Error Callbacks
	virtual void ErrorBadInputParameter(UINT /*ParameterIndex*/) {}
	virtual void ErrorDuplicateSubobject(PipelineStateSubobjectType /*DuplicateType*/) {}
	virtual void ErrorUnknownSubobject(UINT /*UnknownTypeValue*/) {}
};

struct PipelineStateStreamDesc
{
	SIZE_T SizeInBytes;
	void*  pPipelineStateSubobjectStream;
};

void RHIParsePipelineStream(const PipelineStateStreamDesc& Desc, IPipelineParserCallbacks* Callbacks);

enum class ESRVType
{
	Unknown,
	Texture2D,
};

struct Texture2DSRV
{
	std::optional<UINT>	 MostDetailedMip;
	std::optional<UINT>	 MipLevels;
	std::optional<FLOAT> ResourceMinLODClamp;
};

struct ShaderResourceViewDesc
{
	DXGI_FORMAT Format;
	ESRVType	Type;
	union
	{
		Texture2DSRV Texture2D;
	};
};

struct DescriptorIndexPool
{
	DescriptorIndexPool() = default;
	explicit DescriptorIndexPool(size_t NumIndices)
	{
		Elements.resize(NumIndices);
		Reset();
	}

	auto& operator[](size_t Index) { return Elements[Index]; }

	const auto& operator[](size_t Index) const { return Elements[Index]; }

	void Reset()
	{
		FreeStart		  = 0;
		NumActiveElements = 0;
		for (size_t i = 0; i < Elements.size(); ++i)
		{
			Elements[i] = i + 1;
		}
	}

	// Removes the first element from the free list and returns its index
	size_t Allocate()
	{
		assert(NumActiveElements < Elements.size() && "Consider increasing the size of the pool");
		NumActiveElements++;
		size_t Index = FreeStart;
		FreeStart	 = Elements[Index];
		return Index;
	}

	void Release(size_t Index)
	{
		NumActiveElements--;
		Elements[Index] = FreeStart;
		FreeStart		= Index;
	}

	std::vector<size_t> Elements;
	size_t				FreeStart		  = 0;
	size_t				NumActiveElements = 0;
};

constexpr D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE ToD3D12BeginAccessType(ELoadOp Op)
{
	// clang-format off
	switch (Op)
	{
	case ELoadOp::Load:		return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
	case ELoadOp::Clear:	return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
	case ELoadOp::Noop:		return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
	default:				return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_NO_ACCESS;
	}
	// clang-format on
}

constexpr D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE ToD3D12EndAccessType(EStoreOp Op)
{
	// clang-format off
	switch (Op)
	{
	case EStoreOp::Store:	return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
	case EStoreOp::Noop:	return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
	default:				return D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_NO_ACCESS;
	}
	// clang-format on
}
