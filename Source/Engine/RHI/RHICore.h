#pragma once
#include <cstddef>
#include <cassert>

#include <string>
#include <string_view>
#include <span>

#include <memory>
#include <mutex>
#include <optional>
#include <filesystem>

#include <array>
#include <vector>
#include <map>
#include <unordered_map>
#include <queue>
#include <set>
#include <unordered_map>

#include <dxgiformat.h>

#include "System/System.h"

DECLARE_LOG_CATEGORY(RHI);

enum class RHI_VENDOR
{
	NVIDIA = 0x10DE,
	AMD	   = 0x1002,
	Intel  = 0x8086
};
inline const char* GetRHIVendorString(RHI_VENDOR Vendor)
{
	switch (Vendor)
	{
		using enum RHI_VENDOR;
	case NVIDIA:
		return "NVIDIA";
	case AMD:
		return "AMD";
	case Intel:
		return "Intel";
	}
	return "<unknown>";
}

enum class RHI_SHADER_MODEL
{
	ShaderModel_6_5,
	ShaderModel_6_6
};

enum class RHI_SHADER_TYPE
{
	Vertex,
	Hull,
	Domain,
	Geometry,
	Pixel,
	Compute,
	Amplification,
	Mesh
};

enum class RHI_PIPELINE_STATE_TYPE
{
	Graphics,
	Compute,
};

enum class RHI_PIPELINE_STATE_SUBOBJECT_TYPE
{
	RootSignature,
	InputLayout,
	VS,
	PS,
	DS,
	HS,
	GS,
	CS,
	AS,
	MS,
	BlendState,
	RasterizerState,
	DepthStencilState,
	RenderTargetState,
	PrimitiveTopology,

	NumTypes
};
inline const char* GetRHIPipelineStateSubobjectTypeString(RHI_PIPELINE_STATE_SUBOBJECT_TYPE Type)
{
	switch (Type)
	{
	case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::RootSignature:
		return "Root Signature";
	case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::InputLayout:
		return "Input Layout";
	case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::VS:
		return "Vertex Shader";
	case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::PS:
		return "Pixel Shader";
	case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::DS:
		return "Domain Shader";
	case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::HS:
		return "Hull Shader";
	case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::GS:
		return "Geometry Shader";
	case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::CS:
		return "Compute Shader";
	case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::AS:
		return "Amplification Shader";
	case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::MS:
		return "Mesh Shader";
	case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::BlendState:
		return "Blend State";
	case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::RasterizerState:
		return "Rasterizer State";
	case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::DepthStencilState:
		return "Depth Stencil State";
	case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::RenderTargetState:
		return "Render Target State";
	case RHI_PIPELINE_STATE_SUBOBJECT_TYPE::PrimitiveTopology:
		return "Primitive Topology";
	}
	return "<unknown>";
}

enum class RHI_PRIMITIVE_TOPOLOGY
{
	Undefined,
	Point,
	Line,
	Triangle,
	Patch
};

enum class RHI_COMPARISON_FUNC
{
	Never,		  // Comparison always fails
	Less,		  // Passes if source is less than the destination
	Equal,		  // Passes if source is equal to the destination
	LessEqual,	  // Passes if source is less than or equal to the destination
	Greater,	  // Passes if source is greater than to the destination
	NotEqual,	  // Passes if source is not equal to the destination
	GreaterEqual, // Passes if source is greater than or equal to the destination
	Always		  // Comparison always succeeds
};

enum class RHI_SAMPLER_FILTER
{
	Point,
	Linear,
	Anisotropic
};

enum class RHI_SAMPLER_ADDRESS_MODE
{
	Wrap,
	Mirror,
	Clamp,
	Border,
};

enum class RHI_BLEND_OP
{
	Add,
	Subtract,
	ReverseSubtract,
	Min,
	Max
};

enum class RHI_FACTOR
{
	Zero,				 // (0, 0, 0, 0)
	One,				 // (1, 1, 1, 1)
	SrcColor,			 // The pixel-shader output color
	OneMinusSrcColor,	 // One minus the pixel-shader output color
	DstColor,			 // The render-target color
	OneMinusDstColor,	 // One minus the render-target color
	SrcAlpha,			 // The pixel-shader output alpha value
	OneMinusSrcAlpha,	 // One minus the pixel-shader output alpha value
	DstAlpha,			 // The render-target alpha value
	OneMinusDstAlpha,	 // One minus the render-target alpha value
	BlendFactor,		 // Constant color, set using Desc#SetBlendFactor()
	OneMinusBlendFactor, // One minus constant color, set using Desc#SetBlendFactor()
	SrcAlphaSaturate,	 // (f, f, f, 1), where f = min(pixel shader output alpha, 1 - render-target pixel alpha)
	Src1Color,			 // Fragment-shader output color 1
	OneMinusSrc1Color,	 // One minus pixel-shader output color 1
	Src1Alpha,			 // Fragment-shader output alpha 1
	OneMinusSrc1Alpha	 // One minus pixel-shader output alpha 1
};

struct RHI_RENDER_TARGET_BLEND_DESC
{
	bool		 BlendEnable   = false;
	RHI_FACTOR	 SrcBlendRgb   = RHI_FACTOR::One;
	RHI_FACTOR	 DstBlendRgb   = RHI_FACTOR::Zero;
	RHI_BLEND_OP BlendOpRgb	   = RHI_BLEND_OP::Add;
	RHI_FACTOR	 SrcBlendAlpha = RHI_FACTOR::One;
	RHI_FACTOR	 DstBlendAlpha = RHI_FACTOR::Zero;
	RHI_BLEND_OP BlendOpAlpha  = RHI_BLEND_OP::Add;

	struct WriteMask
	{
		bool R = true;
		bool G = true;
		bool B = true;
		bool A = true;
	} WriteMask;
};

enum class RHI_FILL_MODE
{
	Wireframe, // Draw lines connecting the vertices.
	Solid	   // Fill the triangles formed by the vertices
};

enum class RHI_CULL_MODE
{
	None,  // Always draw all triangles
	Front, // Do not draw triangles that are front-facing
	Back   // Do not draw triangles that are back-facing
};

enum class RHI_STENCIL_OP
{
	Keep,			  // Keep the stencil value
	Zero,			  // Set the stencil value to zero
	Replace,		  // Replace the stencil value with the reference value
	IncreaseSaturate, // Increase the stencil value by one, clamp if necessary
	DecreaseSaturate, // Decrease the stencil value by one, clamp if necessary
	Invert,			  // Invert the stencil data (bitwise not)
	Increase,		  // Increase the stencil value by one, wrap if necessary
	Decrease		  // Decrease the stencil value by one, wrap if necessary
};

struct RHI_DEPTH_STENCILOP_DESC
{
	RHI_STENCIL_OP		StencilFailOp	   = RHI_STENCIL_OP::Keep;
	RHI_STENCIL_OP		StencilDepthFailOp = RHI_STENCIL_OP::Keep;
	RHI_STENCIL_OP		StencilPassOp	   = RHI_STENCIL_OP::Keep;
	RHI_COMPARISON_FUNC StencilFunc		   = RHI_COMPARISON_FUNC::Always;
};

struct BlendState
{
	bool						 AlphaToCoverageEnable	= false;
	bool						 IndependentBlendEnable = false;
	RHI_RENDER_TARGET_BLEND_DESC RenderTargets[8];
};

struct RasterizerState
{
	RHI_FILL_MODE FillMode				= RHI_FILL_MODE::Solid;
	RHI_CULL_MODE CullMode				= RHI_CULL_MODE::Back;
	bool		  FrontCounterClockwise = false;
	int			  DepthBias				= 0;
	float		  DepthBiasClamp		= 0.0f;
	float		  SlopeScaledDepthBias	= 0.0f;
	bool		  DepthClipEnable		= true;
	bool		  MultisampleEnable		= false;
	bool		  AntialiasedLineEnable = false;
	unsigned int  ForcedSampleCount		= 0;
	bool		  ConservativeRaster	= false;
};

struct DepthStencilState
{
	// Default states
	// https://docs.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_depth_stencil_desc#remarks
	bool					 DepthEnable	  = true;
	bool					 DepthWrite		  = true;
	RHI_COMPARISON_FUNC		 DepthFunc		  = RHI_COMPARISON_FUNC::Less;
	bool					 StencilEnable	  = false;
	std::byte				 StencilReadMask  = std::byte{ 0xff };
	std::byte				 StencilWriteMask = std::byte{ 0xff };
	RHI_DEPTH_STENCILOP_DESC FrontFace;
	RHI_DEPTH_STENCILOP_DESC BackFace;
};

struct RenderTargetState
{
	DXGI_FORMAT	  RTFormats[8]	   = { DXGI_FORMAT_UNKNOWN };
	std::uint32_t NumRenderTargets = 0;
	DXGI_FORMAT	  DSFormat		   = DXGI_FORMAT_UNKNOWN;
};

struct RHIViewport
{
	float TopLeftX;
	float TopLeftY;
	float Width;
	float Height;
	float MinDepth;
	float MaxDepth;
};

struct RHIRect
{
	long Left;
	long Top;
	long Right;
	long Bottom;
};

template<typename TDesc, RHI_PIPELINE_STATE_SUBOBJECT_TYPE TType>
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
	RHI_PIPELINE_STATE_SUBOBJECT_TYPE Type;
	TDesc							  Desc;
};

namespace RHI
{
	class D3D12RootSignature;
	class D3D12InputLayout;
} // namespace RHI
class Shader;

using PipelineStateStreamRootSignature	   = PipelineStateStreamSubobject<RHI::D3D12RootSignature*, RHI_PIPELINE_STATE_SUBOBJECT_TYPE::RootSignature>;
using PipelineStateStreamInputLayout	   = PipelineStateStreamSubobject<RHI::D3D12InputLayout*, RHI_PIPELINE_STATE_SUBOBJECT_TYPE::InputLayout>;
using PipelineStateStreamVS				   = PipelineStateStreamSubobject<Shader*, RHI_PIPELINE_STATE_SUBOBJECT_TYPE::VS>;
using PipelineStateStreamPS				   = PipelineStateStreamSubobject<Shader*, RHI_PIPELINE_STATE_SUBOBJECT_TYPE::PS>;
using PipelineStateStreamDS				   = PipelineStateStreamSubobject<Shader*, RHI_PIPELINE_STATE_SUBOBJECT_TYPE::DS>;
using PipelineStateStreamHS				   = PipelineStateStreamSubobject<Shader*, RHI_PIPELINE_STATE_SUBOBJECT_TYPE::HS>;
using PipelineStateStreamGS				   = PipelineStateStreamSubobject<Shader*, RHI_PIPELINE_STATE_SUBOBJECT_TYPE::GS>;
using PipelineStateStreamCS				   = PipelineStateStreamSubobject<Shader*, RHI_PIPELINE_STATE_SUBOBJECT_TYPE::CS>;
using PipelineStateStreamAS				   = PipelineStateStreamSubobject<Shader*, RHI_PIPELINE_STATE_SUBOBJECT_TYPE::AS>;
using PipelineStateStreamMS				   = PipelineStateStreamSubobject<Shader*, RHI_PIPELINE_STATE_SUBOBJECT_TYPE::MS>;
using PipelineStateStreamBlendState		   = PipelineStateStreamSubobject<BlendState, RHI_PIPELINE_STATE_SUBOBJECT_TYPE::BlendState>;
using PipelineStateStreamRasterizerState   = PipelineStateStreamSubobject<RasterizerState, RHI_PIPELINE_STATE_SUBOBJECT_TYPE::RasterizerState>;
using PipelineStateStreamDepthStencilState = PipelineStateStreamSubobject<DepthStencilState, RHI_PIPELINE_STATE_SUBOBJECT_TYPE::DepthStencilState>;
using PipelineStateStreamRenderTargetState = PipelineStateStreamSubobject<RenderTargetState, RHI_PIPELINE_STATE_SUBOBJECT_TYPE::RenderTargetState>;
using PipelineStateStreamPrimitiveTopology = PipelineStateStreamSubobject<RHI_PRIMITIVE_TOPOLOGY, RHI_PIPELINE_STATE_SUBOBJECT_TYPE::PrimitiveTopology>;

class IPipelineParserCallbacks
{
public:
	virtual ~IPipelineParserCallbacks() = default;

	// Subobject Callbacks
	virtual void RootSignatureCb(RHI::D3D12RootSignature*) {}
	virtual void InputLayoutCb(RHI::D3D12InputLayout*) {}
	virtual void VSCb(Shader*) {}
	virtual void PSCb(Shader*) {}
	virtual void DSCb(Shader*) {}
	virtual void HSCb(Shader*) {}
	virtual void GSCb(Shader*) {}
	virtual void CSCb(Shader*) {}
	virtual void ASCb(Shader*) {}
	virtual void MSCb(Shader*) {}
	virtual void BlendStateCb(const BlendState&) {}
	virtual void RasterizerStateCb(const RasterizerState&) {}
	virtual void DepthStencilStateCb(const DepthStencilState&) {}
	virtual void RenderTargetStateCb(const RenderTargetState&) {}
	virtual void PrimitiveTopologyTypeCb(RHI_PRIMITIVE_TOPOLOGY) {}

	// Error Callbacks
	virtual void ErrorBadInputParameter(size_t /*ParameterIndex*/) {}
	virtual void ErrorDuplicateSubobject(RHI_PIPELINE_STATE_SUBOBJECT_TYPE /*DuplicateType*/) {}
	virtual void ErrorUnknownSubobject(size_t /*UnknownTypeValue*/) {}
};

struct PipelineStateStreamDesc
{
	size_t SizeInBytes;
	void*  pPipelineStateSubobjectStream;
};

void RHIParsePipelineStream(const PipelineStateStreamDesc& Desc, IPipelineParserCallbacks* Callbacks);
