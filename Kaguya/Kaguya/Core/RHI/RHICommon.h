#pragma once
#include "RefPtr.h"
#include "ShaderCompiler.h"
#include "D3D12/d3dx12.h"

class IRHITexture;
class IRHIRenderPass;
class IRHIRootSignature;
class IRHIResource;

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

enum class ESamplerFilter
{
	Point,
	Linear,
	Anisotropic
};

enum class ESamplerAddressMode
{
	Wrap,
	Mirror,
	Clamp,
	Border,
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

	void AddConstantBufferView(UINT Binding) { AddDescriptor(Binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER); }
	void AddShaderResourceView(UINT Binding) { AddDescriptor(Binding, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); }
	void AddUnorderedAccessView(UINT Binding) { AddDescriptor(Binding, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); }

	void AddDescriptor(UINT Binding, VkDescriptorType Type)
	{
		VkDescriptorSetLayoutBinding& DescriptorSetLayoutBinding = DescriptorSetLayoutBindings.emplace_back();
		DescriptorSetLayoutBinding.binding						 = Binding;
		DescriptorSetLayoutBinding.descriptorType				 = Type;
		DescriptorSetLayoutBinding.descriptorCount				 = 1;
		DescriptorSetLayoutBinding.stageFlags					 = VK_SHADER_STAGE_ALL;
	}

	std::vector<PushConstant>				  PushConstants;
	std::vector<VkDescriptorSetLayoutBinding> DescriptorSetLayoutBindings;
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
	ClearValue(ERHIFormat Format, FLOAT Color[4])
		: Format(Format)
	{
		std::memcpy(this->Color, Color, sizeof(FLOAT) * 4);
	}
	ClearValue(ERHIFormat Format, FLOAT Depth, UINT8 Stencil)
		: Format(Format)
		, DepthStencil{ Depth, Stencil }
	{
	}

	ERHIFormat Format = ERHIFormat::UNKNOWN;
	union
	{
		FLOAT			  Color[4];
		DepthStencilValue DepthStencil;
	};
};

struct RenderPassAttachment
{
	[[nodiscard]] bool IsValid() const noexcept { return Format != ERHIFormat::UNKNOWN; }

	ERHIFormat Format = ERHIFormat::UNKNOWN;
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

struct RenderTargetDesc
{
	void AddRenderTarget(IRHITexture* RenderTarget) { RenderTargets[NumRenderTargets++] = RenderTarget; }
	void SetDepthStencil(IRHITexture* DepthStencil) { this->DepthStencil = DepthStencil; }

	IRHIRenderPass* RenderPass = nullptr;
	UINT			Width;
	UINT			Height;

	UINT		 NumRenderTargets = 0;
	IRHITexture* RenderTargets[8] = {};
	IRHITexture* DepthStencil	  = nullptr;
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

	ERHIFormat						 Format				 = ERHIFormat::UNKNOWN;
	ERHITextureType					 Type				 = ERHITextureType::Unknown;
	UINT							 Width				 = 1;
	UINT							 Height				 = 1;
	UINT16							 DepthOrArraySize	 = 1;
	UINT16							 MipLevels			 = 1;
	ERHITextureFlags				 Flags				 = ERHITextureFlags::RHITextureFlag_None;
	std::optional<D3D12_CLEAR_VALUE> OptimizedClearValue = std::nullopt;
};

class BlendState
{
public:
	/// <summary>
	/// Defines how to combine the blend inputs
	/// </summary>
	enum class BlendOp
	{
		Add,			 // Add src1 and src2
		Subtract,		 // Subtract src1 from src2
		ReverseSubtract, // Subtract src2 from src1
		Min,			 // Find the minimum between the sources (per-channel)
		Max				 // Find the maximum between the sources (per-channel)
	};

	/// <summary>
	/// Defines how to modulate the pixel-shader and render-target pixel values
	/// </summary>
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
		Factor	SrcBlendRgb,
		Factor	DstBlendRgb,
		BlendOp BlendOpRgb,
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
	/// <summary>
	/// Defines triangle filling mode.
	/// </summary>
	enum class EFillMode
	{
		Wireframe, // Draw lines connecting the vertices.
		Solid	   // Fill the triangles formed by the vertices
	};

	/// <summary>
	/// Defines which face would be culled
	/// </summary>
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

	std::vector<Element> Elements;
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

class IPipelineParserCallbacks
{
public:
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
	ERHIFormat Format;
	ESRVType   Type;
	union
	{
		Texture2DSRV Texture2D;
	};
};

struct IndexPool
{
	IndexPool() = default;
	IndexPool(size_t NumIndices)
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
		size_t index = FreeStart;
		FreeStart	 = Elements[index];
		return index;
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
