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

#include <dxgiformat.h>

#include "System/System.h"

DECLARE_LOG_CATEGORY(RHI);

enum class RHI_VENDOR
{
	NVIDIA = 0x10DE,
	AMD	   = 0x1002,
	Intel  = 0x8086
};

enum class RHI_SHADER_MODEL
{
	ShaderModel_6_6
};

enum class RHI_SHADER_TYPE
{
	Vertex,
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

enum class RHI_PRIMITIVE_TOPOLOGY
{
	Undefined,
	Point,
	Line,
	Triangle
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
	BlendFactor,		 // Constant color, set using CommandList
	OneMinusBlendFactor, // One minus constant color, set using CommandList
	SrcAlphaSaturate,	 // (f, f, f, 1), where f = min(pixel shader output alpha, 1 - render-target pixel alpha)
	Src1Color,			 // Fragment-shader output color 1
	OneMinusSrc1Color,	 // One minus pixel-shader output color 1
	Src1Alpha,			 // Fragment-shader output alpha 1
	OneMinusSrc1Alpha	 // One minus pixel-shader output alpha 1
};

struct RHIRenderTargetBlendDesc
{
	bool		 EnableBlend   = false;
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

struct RHIDepthStencilOpDesc
{
	RHI_STENCIL_OP		StencilFailOp	   = RHI_STENCIL_OP::Keep;
	RHI_STENCIL_OP		StencilDepthFailOp = RHI_STENCIL_OP::Keep;
	RHI_STENCIL_OP		StencilPassOp	   = RHI_STENCIL_OP::Keep;
	RHI_COMPARISON_FUNC StencilFunc		   = RHI_COMPARISON_FUNC::Always;
};

struct RHIBlendState
{
	bool					 AlphaToCoverageEnable	= false;
	bool					 IndependentBlendEnable = false;
	RHIRenderTargetBlendDesc RenderTargets[8];
};

struct RHIRasterizerState
{
	RHI_FILL_MODE FillMode				= RHI_FILL_MODE::Solid;
	RHI_CULL_MODE CullMode				= RHI_CULL_MODE::Back;
	i32			  DepthBias				= 0;
	f32			  DepthBiasClamp		= 0.0f;
	f32			  SlopeScaledDepthBias	= 0.0f;
	bool		  FrontCounterClockwise = false;
	bool		  DepthClipEnable		= true;
};
static constexpr size_t SizeOfRHIRasterizerState = sizeof(RHIRasterizerState);

struct RHIDepthStencilState
{
	bool				  EnableDepthTest	= true;
	bool				  EnableStencilTest = false;
	bool				  DepthWrite		= true;
	RHI_COMPARISON_FUNC	  DepthFunc			= RHI_COMPARISON_FUNC::Less;
	u8					  StencilReadMask	= 0xff;
	u8					  StencilWriteMask	= 0xff;
	RHIDepthStencilOpDesc FrontFace;
	RHIDepthStencilOpDesc BackFace;
};

struct RHIRenderTargetState
{
	DXGI_FORMAT RTFormats[8]	 = { DXGI_FORMAT_UNKNOWN };
	u32			NumRenderTargets = 0;
	DXGI_FORMAT DSFormat		 = DXGI_FORMAT_UNKNOWN;
};

struct RHIViewport
{
	f32 TopLeftX;
	f32 TopLeftY;
	f32 Width;
	f32 Height;
	f32 MinDepth;
	f32 MaxDepth;
};

struct RHIRect
{
	i32 Left;
	i32 Top;
	i32 Right;
	i32 Bottom;
};

template<typename TDesc, RHI_PIPELINE_STATE_SUBOBJECT_TYPE TType>
class alignas(void*) PipelineStateStreamSubobject
{
public:
	PipelineStateStreamSubobject() noexcept = default;
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
	RHI_PIPELINE_STATE_SUBOBJECT_TYPE Type = TType;
	TDesc							  Desc = {};
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
using PipelineStateStreamCS				   = PipelineStateStreamSubobject<Shader*, RHI_PIPELINE_STATE_SUBOBJECT_TYPE::CS>;
using PipelineStateStreamAS				   = PipelineStateStreamSubobject<Shader*, RHI_PIPELINE_STATE_SUBOBJECT_TYPE::AS>;
using PipelineStateStreamMS				   = PipelineStateStreamSubobject<Shader*, RHI_PIPELINE_STATE_SUBOBJECT_TYPE::MS>;
using PipelineStateStreamBlendState		   = PipelineStateStreamSubobject<RHIBlendState, RHI_PIPELINE_STATE_SUBOBJECT_TYPE::BlendState>;
using PipelineStateStreamRasterizerState   = PipelineStateStreamSubobject<RHIRasterizerState, RHI_PIPELINE_STATE_SUBOBJECT_TYPE::RasterizerState>;
using PipelineStateStreamDepthStencilState = PipelineStateStreamSubobject<RHIDepthStencilState, RHI_PIPELINE_STATE_SUBOBJECT_TYPE::DepthStencilState>;
using PipelineStateStreamRenderTargetState = PipelineStateStreamSubobject<RHIRenderTargetState, RHI_PIPELINE_STATE_SUBOBJECT_TYPE::RenderTargetState>;
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
	virtual void CSCb(Shader*) {}
	virtual void ASCb(Shader*) {}
	virtual void MSCb(Shader*) {}
	virtual void BlendStateCb(const RHIBlendState&) {}
	virtual void RasterizerStateCb(const RHIRasterizerState&) {}
	virtual void DepthStencilStateCb(const RHIDepthStencilState&) {}
	virtual void RenderTargetStateCb(const RHIRenderTargetState&) {}
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

const char* GetRHIVendorString(RHI_VENDOR Vendor);
const char* GetRHIPipelineStateSubobjectTypeString(RHI_PIPELINE_STATE_SUBOBJECT_TYPE Type);
