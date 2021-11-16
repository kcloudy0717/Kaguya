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
	MS,
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
using PipelineStateStreamMS					= PipelineStateStreamSubobject<Shader*, PipelineStateSubobjectType::MS>;
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
	virtual void MSCb(Shader*) {}
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
