#pragma once
#include "RefPtr.h"
#include "ShaderCompiler.h"
#include "D3D12/d3dx12.h"

class IRHIRenderPass;
class IRHIRootSignature;
class IRHIResource;
class IRHIDescriptorTable;

struct DeviceOptions
{
	bool EnableDebugLayer;
	bool EnableGpuBasedValidation;
	bool EnableAutoDebugName;
};

struct RHIViewport
{
	RHIViewport() noexcept = default;
	RHIViewport(FLOAT TopLeftX, FLOAT TopLeftY, FLOAT Width, FLOAT Height, FLOAT MinDepth, FLOAT MaxDepth) noexcept
		: TopLeftX(TopLeftX)
		, TopLeftY(TopLeftY)
		, Width(Width)
		, Height(Height)
		, MinDepth(MinDepth)
		, MaxDepth(MaxDepth)
	{
	}
	RHIViewport(FLOAT Width, FLOAT Height)
		: RHIViewport(0.0f, 0.0f, Width, Height, 0.0f, 1.0f)
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
	LONG Left;
	LONG Top;
	LONG Right;
	LONG Bottom;
};

enum class PrimitiveTopology
{
	Undefined,
	Point,
	Line,
	Triangle,
	Patch
};

enum class ComparisonFunc
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

enum class DescriptorHeapType
{
	Resource,
	Sampler
};

struct DescriptorHeapDesc
{
	DescriptorHeapType Type;
	union
	{
		struct
		{
			UINT NumConstantBufferDescriptors;
			UINT NumTextureDescriptors;
			UINT NumRWTextureDescriptors;
		} Resource;
		struct
		{
			UINT NumDescriptors;
		} Sampler;
	};
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

class BlendState
{
public:
	/* Defines how to combine the blend inputs */
	enum class BlendOp
	{
		Add,			 // Add src1 and src2
		Subtract,		 // Subtract src1 from src2
		ReverseSubtract, // Subtract src2 from src1
		Min,			 // Find the minimum between the sources (per-channel)
		Max				 // Find the maximum between the sources (per-channel)
	};

	/* Defines how to modulate the pixel-shader and render-target pixel values */
	enum class Factor
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

	struct RenderTarget
	{
		bool	BlendEnable	  = false;
		Factor	SrcBlendRGB	  = Factor::One;
		Factor	DstBlendRGB	  = Factor::Zero;
		BlendOp BlendOpRGB	  = BlendOp::Add;
		Factor	SrcBlendAlpha = Factor::One;
		Factor	DstBlendAlpha = Factor::Zero;
		BlendOp BlendOpAlpha  = BlendOp::Add;

		struct WriteMask
		{
			bool R = true;
			bool G = true;
			bool B = true;
			bool A = true;
		} WriteMask;
	};

	BlendState();

	void SetAlphaToCoverageEnable(bool AlphaToCoverageEnable);
	void SetIndependentBlendEnable(bool IndependentBlendEnable);

	void SetRenderTargetBlendDesc(
		UINT	Index,
		Factor	SrcBlendRGB,
		Factor	DstBlendRGB,
		BlendOp BlendOpRGB,
		Factor	SrcBlendAlpha,
		Factor	DstBlendAlpha,
		BlendOp BlendOpAlpha);

	bool		 AlphaToCoverageEnable;
	bool		 IndependentBlendEnable;
	RenderTarget RenderTargets[8];
};

class RasterizerState
{
public:
	enum class EFillMode
	{
		Wireframe, // Draw lines connecting the vertices.
		Solid	   // Fill the triangles formed by the vertices
	};

	enum class ECullMode
	{
		None,  // Always draw all triangles
		Front, // Do not draw triangles that are front-facing
		Back   // Do not draw triangles that are back-facing
	};

	RasterizerState();

	void SetFillMode(EFillMode FillMode);

	void SetCullMode(ECullMode CullMode);

	// Determines how to interpret triangle direction
	void SetFrontCounterClockwise(bool FrontCounterClockwise);

	void SetDepthBias(int DepthBias);

	void SetDepthBiasClamp(float DepthBiasClamp);

	void SetSlopeScaledDepthBias(float SlopeScaledDepthBias);

	void SetDepthClipEnable(bool DepthClipEnable);

	void SetMultisampleEnable(bool MultisampleEnable);

	void SetAntialiasedLineEnable(bool AntialiasedLineEnable);

	void SetForcedSampleCount(unsigned int ForcedSampleCount);

	void SetConservativeRaster(bool ConservativeRaster);

	EFillMode	 FillMode;
	ECullMode	 CullMode;
	bool		 FrontCounterClockwise;
	int			 DepthBias;
	float		 DepthBiasClamp;
	float		 SlopeScaledDepthBias;
	bool		 DepthClipEnable;
	bool		 MultisampleEnable;
	bool		 AntialiasedLineEnable;
	unsigned int ForcedSampleCount;
	bool		 ConservativeRaster;
};

class DepthStencilState
{
public:
	enum class Face
	{
		Front,
		Back,
		FrontAndBack
	};

	enum class StencilOperation
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

	struct Stencil
	{
		StencilOperation StencilFailOp = StencilOperation::Keep; // Stencil operation in case stencil test fails
		StencilOperation StencilDepthFailOp =
			StencilOperation::Keep; // Stencil operation in case stencil test passes but depth test fails
		StencilOperation StencilPassOp =
			StencilOperation::Keep;							 // Stencil operation in case stencil and depth tests pass
		ComparisonFunc StencilFunc = ComparisonFunc::Always; // Stencil comparison function
	};

	DepthStencilState();

	void SetDepthEnable(bool DepthEnable);

	void SetDepthWrite(bool DepthWrite);

	void SetDepthFunc(ComparisonFunc DepthFunc);

	void SetStencilEnable(bool StencilEnable);

	void SetStencilReadMask(UINT8 StencilReadMask);

	void SetStencilWriteMask(UINT8 StencilWriteMask);

	void SetStencilOp(
		Face			 Face,
		StencilOperation StencilFailOp,
		StencilOperation StencilDepthFailOp,
		StencilOperation StencilPassOp);

	void SetStencilFunc(Face Face, ComparisonFunc StencilFunc);

	bool		   DepthEnable;
	bool		   DepthWrite;
	ComparisonFunc DepthFunc;
	bool		   StencilEnable;
	UINT8		   StencilReadMask;
	UINT8		   StencilWriteMask;
	Stencil		   FrontFace;
	Stencil		   BackFace;
};

class InputLayout
{
public:
	void AddVertexLayoutElement(
		std::string_view SemanticName,
		UINT			 SemanticIndex,
		UINT			 Location,
		ERHIFormat		 Format,
		UINT			 Stride);

	struct Element
	{
		std::string SemanticName;
		UINT		SemanticIndex;
		UINT		Location;
		ERHIFormat	Format;
		UINT		Stride;
	};

	std::vector<Element> m_InputElements;
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

template<typename T, PipelineStateSubobjectType Type>
class alignas(void*) PipelineStateStreamSubobject
{
public:
	PipelineStateStreamSubobject() noexcept
		: _Type(Type)
		, _Desc(T())
	{
	}
	PipelineStateStreamSubobject(const T& Desc) noexcept
		: _Type(Type)
		, _Desc(Desc)
	{
	}
	PipelineStateStreamSubobject& operator=(const T& Desc) noexcept
	{
		_Type = Type;
		_Desc = Desc;
		return *this;
	}
			 operator const T&() const noexcept { return _Desc; }
			 operator T&() noexcept { return _Desc; }
	T*		 operator&() noexcept { return &_Desc; }
	const T* operator&() const noexcept { return &_Desc; }

	T& operator->() noexcept { return _Desc; }

private:
	PipelineStateSubobjectType _Type;
	T						   _Desc;
};

// clang-format off
using PipelineStateStreamRootSignature		= PipelineStateStreamSubobject<IRHIRootSignature*, PipelineStateSubobjectType::RootSignature>;
using PipelineStateStreamVS					= PipelineStateStreamSubobject<Shader*, PipelineStateSubobjectType::VS>;
using PipelineStateStreamPS					= PipelineStateStreamSubobject<Shader*, PipelineStateSubobjectType::PS>;
using PipelineStateStreamDS					= PipelineStateStreamSubobject<Shader*, PipelineStateSubobjectType::DS>;
using PipelineStateStreamHS					= PipelineStateStreamSubobject<Shader*, PipelineStateSubobjectType::HS>;
using PipelineStateStreamGS					= PipelineStateStreamSubobject<Shader*, PipelineStateSubobjectType::GS>;
using PipelineStateStreamCS					= PipelineStateStreamSubobject<Shader*, PipelineStateSubobjectType::CS>;
using PipelineStateStreamBlendState			= PipelineStateStreamSubobject<BlendState, PipelineStateSubobjectType::BlendState>;
using PipelineStateStreamRasterizerState	= PipelineStateStreamSubobject<RasterizerState, PipelineStateSubobjectType::RasterizerState>;
using PipelineStateStreamDepthStencilState	= PipelineStateStreamSubobject<DepthStencilState, PipelineStateSubobjectType::DepthStencilState>;
using PipelineStateStreamInputLayout		= PipelineStateStreamSubobject<InputLayout, PipelineStateSubobjectType::InputLayout>;
using PipelineStateStreamPrimitiveTopology	= PipelineStateStreamSubobject<PrimitiveTopology, PipelineStateSubobjectType::PrimitiveTopology>;
using PipelineStateStreamRenderPass			= PipelineStateStreamSubobject<IRHIRenderPass*, PipelineStateSubobjectType::RenderPass>;
// clang-format on

struct IPipelineParserCallbacks
{
	virtual ~IPipelineParserCallbacks() = default;

	// Subobject Callbacks
	virtual void RootSignatureCb(IRHIRootSignature*) {}
	virtual void VSCb(Shader*) {}
	virtual void PSCb(Shader*) {}
	virtual void DSCb(Shader*) {}
	virtual void HSCb(Shader*) {}
	virtual void GSCb(Shader*) {}
	virtual void CSCb(Shader*) {}
	virtual void BlendStateCb(const BlendState&) {}
	virtual void RasterizerStateCb(const RasterizerState&) {}
	virtual void DepthStencilStateCb(const DepthStencilState&) {}
	virtual void InputLayoutCb(const InputLayout&) {}
	virtual void PrimitiveTopologyTypeCb(PrimitiveTopology) {}
	virtual void RenderPassCb(IRHIRenderPass*) {}

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

inline void RHIParsePipelineStream(const PipelineStateStreamDesc& Desc, IPipelineParserCallbacks* pCallbacks)
{
	if (Desc.SizeInBytes == 0 || Desc.pPipelineStateSubobjectStream == nullptr)
	{
		pCallbacks->ErrorBadInputParameter(1); // first parameter issue
		return;
	}

	bool SubobjectSeen[static_cast<size_t>(PipelineStateSubobjectType::NumTypes)] = {};
	for (SIZE_T CurOffset = 0, SizeOfSubobject = 0; CurOffset < Desc.SizeInBytes; CurOffset += SizeOfSubobject)
	{
		BYTE*  pStream		 = static_cast<BYTE*>(Desc.pPipelineStateSubobjectStream) + CurOffset;
		auto   SubobjectType = *reinterpret_cast<PipelineStateSubobjectType*>(pStream);
		size_t Index		 = static_cast<size_t>(SubobjectType);

		if (Index < 0 || Index >= static_cast<size_t>(PipelineStateSubobjectType::NumTypes))
		{
			pCallbacks->ErrorUnknownSubobject(Index);
			return;
		}
		if (SubobjectSeen[Index])
		{
			pCallbacks->ErrorDuplicateSubobject(SubobjectType);
			return; // disallow subobject duplicates in a stream
		}
		SubobjectSeen[Index] = true;

		switch (SubobjectType)
		{
		case PipelineStateSubobjectType::RootSignature:
			pCallbacks->RootSignatureCb(*reinterpret_cast<PipelineStateStreamRootSignature*>(pStream));
			SizeOfSubobject = sizeof(PipelineStateStreamRootSignature);
			break;
		case PipelineStateSubobjectType::VS:
			pCallbacks->VSCb(*reinterpret_cast<PipelineStateStreamVS*>(pStream));
			SizeOfSubobject = sizeof(PipelineStateStreamVS);
			break;
		case PipelineStateSubobjectType::PS:
			pCallbacks->PSCb(*reinterpret_cast<PipelineStateStreamPS*>(pStream));
			SizeOfSubobject = sizeof(PipelineStateStreamPS);
			break;
		case PipelineStateSubobjectType::DS:
			pCallbacks->DSCb(*reinterpret_cast<PipelineStateStreamDS*>(pStream));
			SizeOfSubobject = sizeof(PipelineStateStreamPS);
			break;
		case PipelineStateSubobjectType::HS:
			pCallbacks->HSCb(*reinterpret_cast<PipelineStateStreamHS*>(pStream));
			SizeOfSubobject = sizeof(PipelineStateStreamHS);
			break;
		case PipelineStateSubobjectType::GS:
			pCallbacks->GSCb(*reinterpret_cast<PipelineStateStreamGS*>(pStream));
			SizeOfSubobject = sizeof(PipelineStateStreamGS);
			break;
		case PipelineStateSubobjectType::CS:
			pCallbacks->CSCb(*reinterpret_cast<PipelineStateStreamCS*>(pStream));
			SizeOfSubobject = sizeof(PipelineStateStreamCS);
			break;
		case PipelineStateSubobjectType::BlendState:
			pCallbacks->BlendStateCb(*reinterpret_cast<PipelineStateStreamBlendState*>(pStream));
			SizeOfSubobject = sizeof(PipelineStateStreamBlendState);
			break;
		case PipelineStateSubobjectType::RasterizerState:
			pCallbacks->RasterizerStateCb(*reinterpret_cast<PipelineStateStreamRasterizerState*>(pStream));
			SizeOfSubobject = sizeof(PipelineStateStreamRasterizerState);
			break;
		case PipelineStateSubobjectType::DepthStencilState:
			pCallbacks->DepthStencilStateCb(*reinterpret_cast<PipelineStateStreamDepthStencilState*>(pStream));
			SizeOfSubobject = sizeof(PipelineStateStreamDepthStencilState);
			break;
		case PipelineStateSubobjectType::InputLayout:
			pCallbacks->InputLayoutCb(*reinterpret_cast<PipelineStateStreamInputLayout*>(pStream));
			SizeOfSubobject = sizeof(PipelineStateStreamInputLayout);
			break;
		case PipelineStateSubobjectType::PrimitiveTopology:
			pCallbacks->PrimitiveTopologyTypeCb(*reinterpret_cast<PipelineStateStreamPrimitiveTopology*>(pStream));
			SizeOfSubobject = sizeof(PipelineStateStreamPrimitiveTopology);
			break;
		case PipelineStateSubobjectType::RenderPass:
			pCallbacks->RenderPassCb(*reinterpret_cast<PipelineStateStreamRenderPass*>(pStream));
			SizeOfSubobject = sizeof(PipelineStateStreamRenderPass);
			break;
		default:
			pCallbacks->ErrorUnknownSubobject(Index);
			return;
		}
	}
}
