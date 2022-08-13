#include "D3D12PipelineState.h"
#include "D3D12Device.h"
#include "D3D12RootSignature.h"
#include "ShaderCompiler.h"
#include <unordered_set>

namespace RHI
{
	void D3D12PipelineParserCallbacks::RootSignatureCb(D3D12RootSignature* RootSignature)
	{
		this->RootSignature = RootSignature->GetApiHandle();
	}

	void D3D12PipelineParserCallbacks::InputLayoutCb(D3D12InputLayout* InputLayout)
	{
		InputElements = InputLayout->InputElements;
	}

	void D3D12PipelineParserCallbacks::VSCb(Shader* VS)
	{
		Type	 = RHI_PIPELINE_STATE_TYPE::Graphics;
		this->VS = { .pShaderBytecode = VS->GetPointer(), .BytecodeLength = VS->GetSize() };
	}

	void D3D12PipelineParserCallbacks::PSCb(Shader* PS)
	{
		Type	 = RHI_PIPELINE_STATE_TYPE::Graphics;
		this->PS = { .pShaderBytecode = PS->GetPointer(), .BytecodeLength = PS->GetSize() };
	}

	void D3D12PipelineParserCallbacks::CSCb(Shader* CS)
	{
		Type	 = RHI_PIPELINE_STATE_TYPE::Compute;
		this->CS = { .pShaderBytecode = CS->GetPointer(), .BytecodeLength = CS->GetSize() };
	}

	void D3D12PipelineParserCallbacks::ASCb(Shader* AS)
	{
		Type			   = RHI_PIPELINE_STATE_TYPE::Graphics;
		ContainsMeshShader = true;
		this->AS		   = { .pShaderBytecode = AS->GetPointer(), .BytecodeLength = AS->GetSize() };
	}

	void D3D12PipelineParserCallbacks::MSCb(Shader* MS)
	{
		Type			   = RHI_PIPELINE_STATE_TYPE::Graphics;
		ContainsMeshShader = true;
		this->MS		   = { .pShaderBytecode = MS->GetPointer(), .BytecodeLength = MS->GetSize() };
	}

	void D3D12PipelineParserCallbacks::BlendStateCb(const RHIBlendState& BlendState)
	{
		static constexpr D3D12_BLEND BlendFactors[] = {
			D3D12_BLEND_ZERO,			  // Zero
			D3D12_BLEND_ONE,			  // One
			D3D12_BLEND_SRC_COLOR,		  // SrcColor
			D3D12_BLEND_INV_SRC_COLOR,	  // OneMinusSrcColor
			D3D12_BLEND_DEST_COLOR,		  // DstColor
			D3D12_BLEND_INV_DEST_COLOR,	  // OneMinusDstColor
			D3D12_BLEND_SRC_ALPHA,		  // SrcAlpha
			D3D12_BLEND_INV_SRC_ALPHA,	  // OneMinusSrcAlpha
			D3D12_BLEND_DEST_ALPHA,		  // DstAlpha
			D3D12_BLEND_INV_DEST_ALPHA,	  // OneMinusDstAlpha
			D3D12_BLEND_BLEND_FACTOR,	  // BlendFactor
			D3D12_BLEND_INV_BLEND_FACTOR, // OneMinusBlendFactor
			D3D12_BLEND_SRC_ALPHA_SAT,	  // SrcAlphaSaturate
			D3D12_BLEND_SRC1_COLOR,		  // Src1Color
			D3D12_BLEND_INV_SRC1_COLOR,	  // OneMinusSrc1Color
			D3D12_BLEND_SRC1_ALPHA,		  // Src1Alpha
			D3D12_BLEND_INV_SRC1_ALPHA,	  // OneMinusSrc1Alpha
		};
		static constexpr D3D12_BLEND_OP BlendOps[] = {
			D3D12_BLEND_OP_ADD,			 // Add
			D3D12_BLEND_OP_SUBTRACT,	 // Subtract
			D3D12_BLEND_OP_REV_SUBTRACT, // ReverseSubtract
			D3D12_BLEND_OP_MIN,			 // Min
			D3D12_BLEND_OP_MAX,			 // Max
		};

		Type						= RHI_PIPELINE_STATE_TYPE::Graphics;
		D3D12_BLEND_DESC& Desc		= this->BlendState;
		Desc.AlphaToCoverageEnable	= BlendState.AlphaToCoverageEnable;
		Desc.IndependentBlendEnable = BlendState.IndependentBlendEnable;
		for (size_t i = 0; i < 8; ++i)
		{
			const auto& RHIRenderTarget = BlendState.RenderTargets[i];
			auto&		RenderTarget	= Desc.RenderTarget[i];
			RenderTarget.BlendEnable	= RHIRenderTarget.EnableBlend ? TRUE : FALSE;
			RenderTarget.LogicOpEnable	= FALSE; // Default
			RenderTarget.SrcBlend		= BlendFactors[size_t(RHIRenderTarget.SrcBlendRgb)];
			RenderTarget.DestBlend		= BlendFactors[size_t(RHIRenderTarget.DstBlendRgb)];
			RenderTarget.BlendOp		= BlendOps[size_t(RHIRenderTarget.BlendOpRgb)];
			RenderTarget.SrcBlendAlpha	= BlendFactors[size_t(RHIRenderTarget.SrcBlendAlpha)];
			RenderTarget.DestBlendAlpha = BlendFactors[size_t(RHIRenderTarget.DstBlendAlpha)];
			RenderTarget.BlendOpAlpha	= BlendOps[size_t(RHIRenderTarget.BlendOpAlpha)];
			RenderTarget.LogicOp		= D3D12_LOGIC_OP_CLEAR;
			RenderTarget.RenderTargetWriteMask |= RHIRenderTarget.WriteMask.R ? D3D12_COLOR_WRITE_ENABLE_RED : 0;
			RenderTarget.RenderTargetWriteMask |= RHIRenderTarget.WriteMask.G ? D3D12_COLOR_WRITE_ENABLE_GREEN : 0;
			RenderTarget.RenderTargetWriteMask |= RHIRenderTarget.WriteMask.B ? D3D12_COLOR_WRITE_ENABLE_BLUE : 0;
			RenderTarget.RenderTargetWriteMask |= RHIRenderTarget.WriteMask.A ? D3D12_COLOR_WRITE_ENABLE_ALPHA : 0;
		}
	}

	void D3D12PipelineParserCallbacks::RasterizerStateCb(const RHIRasterizerState& RasterizerState)
	{
		static constexpr D3D12_FILL_MODE FillModes[] = {
			D3D12_FILL_MODE_WIREFRAME, // Wireframe
			D3D12_FILL_MODE_SOLID,	   // Solid
		};
		static constexpr D3D12_CULL_MODE CullModes[] = {
			D3D12_CULL_MODE_NONE,  // None
			D3D12_CULL_MODE_FRONT, // Front
			D3D12_CULL_MODE_BACK,  // Back
		};

		Type						= RHI_PIPELINE_STATE_TYPE::Graphics;
		D3D12_RASTERIZER_DESC& Desc = this->RasterizerState;
		Desc.FillMode				= FillModes[size_t(RasterizerState.FillMode)];
		Desc.CullMode				= CullModes[size_t(RasterizerState.CullMode)];
		Desc.FrontCounterClockwise	= RasterizerState.FrontCounterClockwise;
		Desc.DepthBias				= RasterizerState.DepthBias;
		Desc.DepthBiasClamp			= RasterizerState.DepthBiasClamp;
		Desc.SlopeScaledDepthBias	= RasterizerState.SlopeScaledDepthBias;
		Desc.DepthClipEnable		= RasterizerState.DepthClipEnable;
		Desc.MultisampleEnable		= FALSE;									 // Default
		Desc.AntialiasedLineEnable	= FALSE;									 // Default
		Desc.ForcedSampleCount		= 0;										 // Default
		Desc.ConservativeRaster		= D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF; // Default
	}

	void D3D12PipelineParserCallbacks::DepthStencilStateCb(const RHIDepthStencilState& DepthStencilState)
	{
		static constexpr D3D12_COMPARISON_FUNC ComparisonFuncs[] = {
			D3D12_COMPARISON_FUNC_NEVER,		 // Never
			D3D12_COMPARISON_FUNC_LESS,			 // Less
			D3D12_COMPARISON_FUNC_EQUAL,		 // Equal
			D3D12_COMPARISON_FUNC_LESS_EQUAL,	 // LessEqual
			D3D12_COMPARISON_FUNC_GREATER,		 // Greater
			D3D12_COMPARISON_FUNC_NOT_EQUAL,	 // NotEqual
			D3D12_COMPARISON_FUNC_GREATER_EQUAL, // GreaterEqual
			D3D12_COMPARISON_FUNC_ALWAYS,		 // Always
		};

		static constexpr D3D12_STENCIL_OP StencilOps[] = {
			D3D12_STENCIL_OP_KEEP,	   // Keep
			D3D12_STENCIL_OP_ZERO,	   // Zero
			D3D12_STENCIL_OP_REPLACE,  // Replace
			D3D12_STENCIL_OP_INCR_SAT, // IncreaseSaturate
			D3D12_STENCIL_OP_DECR_SAT, // DecreaseSaturate
			D3D12_STENCIL_OP_INVERT,   // Invert
			D3D12_STENCIL_OP_INCR,	   // Increase
			D3D12_STENCIL_OP_DECR,	   // Decrease
		};

		Type							  = RHI_PIPELINE_STATE_TYPE::Graphics;
		D3D12_DEPTH_STENCIL_DESC& Desc	  = this->DepthStencilState;
		Desc.DepthEnable				  = DepthStencilState.EnableDepthTest;
		Desc.DepthWriteMask				  = DepthStencilState.DepthWrite ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
		Desc.DepthFunc					  = ComparisonFuncs[size_t(DepthStencilState.DepthFunc)];
		Desc.StencilEnable				  = DepthStencilState.EnableStencilTest;
		Desc.StencilReadMask			  = DepthStencilState.StencilReadMask;
		Desc.StencilWriteMask			  = DepthStencilState.StencilWriteMask;
		Desc.FrontFace.StencilFailOp	  = StencilOps[size_t(DepthStencilState.FrontFace.StencilFailOp)];
		Desc.FrontFace.StencilDepthFailOp = StencilOps[size_t(DepthStencilState.FrontFace.StencilDepthFailOp)];
		Desc.FrontFace.StencilPassOp	  = StencilOps[size_t(DepthStencilState.FrontFace.StencilPassOp)];
		Desc.FrontFace.StencilFunc		  = ComparisonFuncs[size_t(DepthStencilState.FrontFace.StencilFunc)];
		Desc.BackFace.StencilFailOp		  = StencilOps[size_t(DepthStencilState.BackFace.StencilFailOp)];
		Desc.BackFace.StencilDepthFailOp  = StencilOps[size_t(DepthStencilState.BackFace.StencilDepthFailOp)];
		Desc.BackFace.StencilPassOp		  = StencilOps[size_t(DepthStencilState.BackFace.StencilPassOp)];
		Desc.BackFace.StencilFunc		  = ComparisonFuncs[size_t(DepthStencilState.BackFace.StencilFunc)];
	}

	void D3D12PipelineParserCallbacks::RenderTargetStateCb(const RHIRenderTargetState& RenderTargetState)
	{
		Type			 = RHI_PIPELINE_STATE_TYPE::Graphics;
		RTFormats[0]	 = RenderTargetState.RTFormats[0];
		RTFormats[1]	 = RenderTargetState.RTFormats[1];
		RTFormats[2]	 = RenderTargetState.RTFormats[2];
		RTFormats[3]	 = RenderTargetState.RTFormats[3];
		RTFormats[4]	 = RenderTargetState.RTFormats[4];
		RTFormats[5]	 = RenderTargetState.RTFormats[5];
		RTFormats[6]	 = RenderTargetState.RTFormats[6];
		RTFormats[7]	 = RenderTargetState.RTFormats[7];
		NumRenderTargets = RenderTargetState.NumRenderTargets;
		DSFormat		 = RenderTargetState.DSFormat;
	}

	void D3D12PipelineParserCallbacks::PrimitiveTopologyTypeCb(RHI_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopology)
	{
		static constexpr D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyTypes[] = {
			D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED, // Undefined
			D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT,	 // Point
			D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE,		 // Line
			D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,	 // Triangle
		};

		Type					= RHI_PIPELINE_STATE_TYPE::Graphics;
		this->PrimitiveTopology = PrimitiveTopologyTypes[size_t(PrimitiveTopology)];
	}

	void D3D12PipelineParserCallbacks::ErrorBadInputParameter(size_t Index)
	{
		KAGUYA_LOG(D3D12RHI, Error, "Bad input parameter at {}", Index);
	}

	void D3D12PipelineParserCallbacks::ErrorDuplicateSubobject(RHI_PIPELINE_STATE_SUBOBJECT_TYPE Type)
	{
		KAGUYA_LOG(D3D12RHI, Error, "Duplicate subobject {}", GetRHIPipelineStateSubobjectTypeString(Type));
	}

	void D3D12PipelineParserCallbacks::ErrorUnknownSubobject(size_t Index)
	{
		KAGUYA_LOG(D3D12RHI, Error, "Unknown subobject at {}", Index);
	}

	struct awaitable_CompilePsoThread
	{
		constexpr bool await_ready() const noexcept { return false; }

		constexpr void await_resume() const noexcept {}

		void await_suspend(std::coroutine_handle<> handle) const
		{
			Process::GetThreadPool().QueueThreadpoolWork(
				[](void* Context)
				{
					std::coroutine_handle<>::from_address(Context)();
				},
				handle.address());
		}
	};

	D3D12PipelineState::D3D12PipelineState(
		D3D12Device*				   Parent,
		std::wstring				   Name,
		const PipelineStateStreamDesc& Desc)
		: D3D12DeviceChild(Parent)
	{
		D3D12PipelineParserCallbacks Parser;
		RHIParsePipelineStream(Desc, &Parser);
		CompilationWork = Create(Parent, Name, Parser.Type, Parser);
	}

	ID3D12PipelineState* D3D12PipelineState::GetApiHandle() const noexcept
	{
		if (CompilationWork)
		{
			CompilationWork.Wait();
			PipelineState	= CompilationWork.Get();
			CompilationWork = nullptr;
		}
		return PipelineState.Get();
	}

	AsyncTask<Arc<ID3D12PipelineState>> D3D12PipelineState::Create(
		D3D12Device*				 Device,
		std::wstring				 Name,
		RHI_PIPELINE_STATE_TYPE		 Type,
		D3D12PipelineParserCallbacks Parser)
	{
		if (Device->AllowAsyncPsoCompilation())
		{
			co_await awaitable_CompilePsoThread{};
		}
		else
		{
			co_await std::suspend_never{};
		}

		if (Type == RHI_PIPELINE_STATE_TYPE::Graphics)
		{
			if (Parser.ContainsMeshShader)
			{
				D3DX12_MESH_SHADER_PIPELINE_STATE_DESC Desc = {
					.pRootSignature		   = Parser.RootSignature,
					.MS					   = Parser.MS,
					.PS					   = Parser.PS,
					.BlendState			   = Parser.BlendState,
					.SampleMask			   = DefaultSampleMask(),
					.RasterizerState	   = Parser.RasterizerState,
					.DepthStencilState	   = Parser.DepthStencilState,
					.PrimitiveTopologyType = Parser.PrimitiveTopology,
					.NumRenderTargets	   = Parser.NumRenderTargets,
					.RTVFormats			   = {
								   Parser.RTFormats[0],
								   Parser.RTFormats[1],
								   Parser.RTFormats[2],
								   Parser.RTFormats[3],
								   Parser.RTFormats[4],
								   Parser.RTFormats[5],
								   Parser.RTFormats[6],
								   Parser.RTFormats[7],
					   },
					.DSVFormat	= Parser.DSFormat,
					.SampleDesc = DefaultSampleDesc(),
					.NodeMask	= Device->GetAllNodeMask(),
					.CachedPSO	= D3D12_CACHED_PIPELINE_STATE(),
					.Flags		= D3D12_PIPELINE_STATE_FLAG_NONE
				};

				CD3DX12_PIPELINE_STATE_STREAM2		   Stream(Desc);
				const D3D12_PIPELINE_STATE_STREAM_DESC StreamDesc = {
					.SizeInBytes				   = sizeof(Stream),
					.pPipelineStateSubobjectStream = &Stream
				};
				co_return Compile<D3D12_PIPELINE_STATE_STREAM_DESC>(Device, Name, StreamDesc);
			}
			else
			{
				const D3D12_GRAPHICS_PIPELINE_STATE_DESC Desc = {
					.pRootSignature		   = Parser.RootSignature,
					.VS					   = Parser.VS,
					.PS					   = Parser.PS,
					.DS					   = D3D12_SHADER_BYTECODE{},
					.HS					   = D3D12_SHADER_BYTECODE{},
					.GS					   = D3D12_SHADER_BYTECODE{},
					.StreamOutput		   = D3D12_STREAM_OUTPUT_DESC{},
					.BlendState			   = Parser.BlendState,
					.SampleMask			   = DefaultSampleMask{},
					.RasterizerState	   = Parser.RasterizerState,
					.DepthStencilState	   = Parser.DepthStencilState,
					.InputLayout		   = { Parser.InputElements.data(), static_cast<UINT>(Parser.InputElements.size()) },
					.IBStripCutValue	   = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,
					.PrimitiveTopologyType = Parser.PrimitiveTopology,
					.NumRenderTargets	   = Parser.NumRenderTargets,
					.RTVFormats			   = {
								   Parser.RTFormats[0],
								   Parser.RTFormats[1],
								   Parser.RTFormats[2],
								   Parser.RTFormats[3],
								   Parser.RTFormats[4],
								   Parser.RTFormats[5],
								   Parser.RTFormats[6],
								   Parser.RTFormats[7],
					   },
					.DSVFormat	= Parser.DSFormat,
					.SampleDesc = DefaultSampleDesc{},
					.NodeMask	= Device->GetAllNodeMask(),
					.CachedPSO	= D3D12_CACHED_PIPELINE_STATE{},
					.Flags		= D3D12_PIPELINE_STATE_FLAG_NONE
				};
				co_return Compile<D3D12_GRAPHICS_PIPELINE_STATE_DESC>(Device, Name, Desc);
			}
		}
		else if (Type == RHI_PIPELINE_STATE_TYPE::Compute)
		{
			const D3D12_COMPUTE_PIPELINE_STATE_DESC Desc = {
				.pRootSignature = Parser.RootSignature,
				.CS				= Parser.CS,
				.NodeMask		= Device->GetAllNodeMask(),
				.CachedPSO		= D3D12_CACHED_PIPELINE_STATE(),
				.Flags			= D3D12_PIPELINE_STATE_FLAG_NONE
			};
			co_return Compile<D3D12_COMPUTE_PIPELINE_STATE_DESC>(Device, Name, Desc);
		}
	}

	template<typename TDesc>
	Arc<ID3D12PipelineState> D3D12PipelineState::Compile(D3D12Device* Device, const std::wstring& Name, const TDesc& Desc)
	{
		ScopedTimer Timer(
			[&](i64 Milliseconds)
			{
				KAGUYA_LOG(D3D12RHI, Info, L"Thread: {} has finished compiling PSO: \"{}\" in {}ms", GetCurrentThreadId(), Name, Milliseconds);
			});

		ID3D12Device2*			 Device2 = Device->GetD3D12Device5();
		Arc<ID3D12PipelineState> PipelineState;
		VERIFY_D3D12_API((Device2->*D3D12PipelineStateTraits<TDesc>::Create())(&Desc, IID_PPV_ARGS(&PipelineState)));
		return PipelineState;
	}

	RaytracingPipelineStateDesc::RaytracingPipelineStateDesc() noexcept
		: Desc(D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE)
		, GlobalRootSignature(nullptr)
		, ShaderConfig()
		, PipelineConfig()
	{
	}

	RaytracingPipelineStateDesc& RaytracingPipelineStateDesc::AddLibrary(
		const D3D12_SHADER_BYTECODE&		  Library,
		const std::vector<std::wstring_view>& Symbols)
	{
		// Add a DXIL library to the pipeline. The exported symbols must correspond exactly to the
		// names of the shaders declared in the library, although unused ones can be omitted.
		Libraries.emplace_back(DxilLibrary(Library, Symbols));
		return *this;
	}

	RaytracingPipelineStateDesc& RaytracingPipelineStateDesc::AddHitGroup(
		std::wstring_view				 HitGroupName,
		std::optional<std::wstring_view> AnyHitSymbol,
		std::optional<std::wstring_view> ClosestHitSymbol,
		std::optional<std::wstring_view> IntersectionSymbol)
	{
		// In DXR the hit-related shaders are grouped into hit groups. Such shaders are:
		// - The intersection shader, which can be used to intersect custom geometry, and is called upon
		//   hitting the bounding box the the object. A default one exists to intersect triangles
		// - The any hit shader, called on each intersection, which can be used to perform early
		//   alpha-testing and allow the ray to continue if needed. Default is a pass-through.
		// - The closest hit shader, invoked on the hit point closest to the ray start.
		// The shaders in a hit group share the same root signature, and are only referred to by the
		// hit group name in other places of the program.
		HitGroups.emplace_back(HitGroup(HitGroupName, AnyHitSymbol, ClosestHitSymbol, IntersectionSymbol));

		// 3 different shaders can be invoked to obtain an intersection: an
		// intersection shader is called when hitting the bounding box of non-triangular geometry.
		// An any-hit shader is called on potential intersections.
		// This shader can, for example, perform alpha-testing and
		// discard some intersections. Finally, the closest-hit program is invoked on
		// the intersection point closest to the ray origin. Those 3 shaders are bound
		// together into a hit group.

		// Note that for triangular geometry the intersection shader is built-in. An
		// empty any-hit shader is also defined by default, so in our simple case each
		// hit group contains only the closest hit shader. Note that since the
		// exported symbols are defined above the shaders can be simply referred to by
		// name.
		return *this;
	}

	RaytracingPipelineStateDesc& RaytracingPipelineStateDesc::AddRootSignatureAssociation(
		ID3D12RootSignature*				  RootSignature,
		const std::vector<std::wstring_view>& Symbols)
	{
		RootSignatureAssociations.emplace_back(RootSignatureAssociation(RootSignature, Symbols));
		return *this;
	}

	RaytracingPipelineStateDesc& RaytracingPipelineStateDesc::SetGlobalRootSignature(
		ID3D12RootSignature* GlobalRootSignature)
	{
		this->GlobalRootSignature = GlobalRootSignature;
		return *this;
	}

	RaytracingPipelineStateDesc& RaytracingPipelineStateDesc::SetRaytracingShaderConfig(
		UINT MaxPayloadSizeInBytes,
		UINT MaxAttributeSizeInBytes)
	{
		ShaderConfig.MaxPayloadSizeInBytes	 = MaxPayloadSizeInBytes;
		ShaderConfig.MaxAttributeSizeInBytes = MaxAttributeSizeInBytes;
		return *this;
	}

	RaytracingPipelineStateDesc& RaytracingPipelineStateDesc::SetRaytracingPipelineConfig(
		UINT MaxTraceRecursionDepth)
	{
		PipelineConfig.MaxTraceRecursionDepth = MaxTraceRecursionDepth;
		return *this;
	}

	D3D12_STATE_OBJECT_DESC RaytracingPipelineStateDesc::Build()
	{
		// Add all the DXIL libraries
		for (const auto& [Library, Symbols] : Libraries)
		{
			auto DxilLibrarySubobject = Desc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
			DxilLibrarySubobject->SetDXILLibrary(&Library);
			for (const auto& Symbol : Symbols)
			{
				DxilLibrarySubobject->DefineExport(Symbol.data());
			}
		}

		// Add all the hit group declarations
		for (const auto& [HitGroupName, AnyHit, ClosestHit, Intersection] : HitGroups)
		{
			auto HitGroupSubobject = Desc.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
			HitGroupSubobject->SetHitGroupExport(HitGroupName.data());
			HitGroupSubobject->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);

			HitGroupSubobject->SetAnyHitShaderImport(!AnyHit.empty() ? AnyHit.data() : nullptr);
			HitGroupSubobject->SetClosestHitShaderImport(!ClosestHit.empty() ? ClosestHit.data() : nullptr);
			HitGroupSubobject->SetIntersectionShaderImport(!Intersection.empty() ? Intersection.data() : nullptr);
		}

		// Add 2 subobjects for every root signature association
		for (const auto& [RootSignature, Symbols] : RootSignatureAssociations)
		{
			// The root signature association requires two objects for each: one to declare the root
			// signature, and another to associate that root signature to a set of symbols

			// Add a subobject to declare the local root signature
			auto LocalRootSignatureSubobject = Desc.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
			LocalRootSignatureSubobject->SetRootSignature(RootSignature);

			// Add a subobject for the association between the exported shader symbols and the local root signature
			auto SubobjectToExportsAssociationSubobject = Desc.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
			for (const auto& Symbol : Symbols)
			{
				SubobjectToExportsAssociationSubobject->AddExport(Symbol.data());
			}
			SubobjectToExportsAssociationSubobject->SetSubobjectToAssociate(*LocalRootSignatureSubobject);
		}

		// Add a subobject for global root signature
		auto GlobalRootSignatureSubobject = Desc.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
		GlobalRootSignatureSubobject->SetRootSignature(GlobalRootSignature);

		// Add a subobject for the raytracing shader config
		auto RaytracingShaderConfigSubobject = Desc.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
		RaytracingShaderConfigSubobject->Config(ShaderConfig.MaxPayloadSizeInBytes, ShaderConfig.MaxAttributeSizeInBytes);

		// Add a subobject for the association between shaders and the payload
		std::vector<std::wstring_view> ExportedSymbols						  = BuildShaderExportList();
		auto						   SubobjectToExportsAssociationSubobject = Desc.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
		for (const auto& Symbol : ExportedSymbols)
		{
			SubobjectToExportsAssociationSubobject->AddExport(Symbol.data());
		}
		SubobjectToExportsAssociationSubobject->SetSubobjectToAssociate(*RaytracingShaderConfigSubobject);

		// Add a subobject for the raytracing pipeline configuration
		auto RaytracingPipelineConfigSubobject = Desc.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
		RaytracingPipelineConfigSubobject->Config(PipelineConfig.MaxTraceRecursionDepth);

		return Desc;
	}

	std::vector<std::wstring_view> RaytracingPipelineStateDesc::BuildShaderExportList() const
	{
		// Get all names from libraries
		// Get names associated to hit groups
		// Return list of libraries + hit group names - shaders in hit groups
		std::unordered_set<std::wstring_view> Exports;

		// Add all the symbols exported by the libraries
		for (const auto& Library : Libraries)
		{
			for (const auto& Symbol : Library.Symbols)
			{
#ifdef _DEBUG
				// Sanity check in debug mode: check that no name is exported more than once
				if (Exports.contains(Symbol))
				{
					throw std::logic_error("Multiple definition of a symbol in the imported DXIL libraries");
				}
#endif
				Exports.insert(Symbol);
			}
		}

#ifdef _DEBUG
		// Sanity check in debug mode: verify that the hit groups do not reference an unknown shader name
		{
			std::unordered_set<std::wstring_view> AllExports = Exports;

			for (const auto& HitGroup : HitGroups)
			{
				if (!HitGroup.AnyHitSymbol.empty() && !Exports.contains(HitGroup.AnyHitSymbol))
				{
					throw std::logic_error("Any hit symbol not found in the imported DXIL libraries");
				}

				if (!HitGroup.ClosestHitSymbol.empty() && !Exports.contains(HitGroup.ClosestHitSymbol))
				{
					throw std::logic_error("Closest hit symbol not found in the imported DXIL libraries");
				}

				if (!HitGroup.IntersectionSymbol.empty() && !Exports.contains(HitGroup.IntersectionSymbol))
				{
					throw std::logic_error("Intersection symbol not found in the imported DXIL libraries");
				}

				AllExports.insert(HitGroup.HitGroupName);
			}

			// Sanity check in debug mode: verify that the root signature associations do not reference an
			// unknown shader or hit group name
			for (const auto& RootSignatureAssociation : RootSignatureAssociations)
			{
				for (const auto& Symbol : RootSignatureAssociation.Symbols)
				{
					if (!Symbol.empty() && !AllExports.contains(Symbol))
					{
						throw std::logic_error("Root association symbol not found in the imported DXIL libraries and hit group names");
					}
				}
			}
		}
#endif

		// Go through all hit groups and remove the symbols corresponding to intersection, any hit and
		// closest hit shaders from the symbol set
		for (const auto& HitGroup : HitGroups)
		{
			if (!HitGroup.ClosestHitSymbol.empty())
			{
				Exports.erase(HitGroup.ClosestHitSymbol);
			}

			if (!HitGroup.AnyHitSymbol.empty())
			{
				Exports.erase(HitGroup.AnyHitSymbol);
			}

			if (!HitGroup.IntersectionSymbol.empty())
			{
				Exports.erase(HitGroup.IntersectionSymbol);
			}

			Exports.insert(HitGroup.HitGroupName);
		}

		return { Exports.begin(), Exports.end() };
	}

	D3D12RaytracingPipelineState::D3D12RaytracingPipelineState(
		D3D12Device*				 Parent,
		RaytracingPipelineStateDesc& Desc)
		: D3D12DeviceChild(Parent)
	{
		D3D12_STATE_OBJECT_DESC ApiDesc = Desc.Build();
		VERIFY_D3D12_API(Parent->GetD3D12Device5()->CreateStateObject(&ApiDesc, IID_PPV_ARGS(&StateObject)));
		VERIFY_D3D12_API(StateObject->QueryInterface(IID_PPV_ARGS(&StateObjectProperties)));
	}

	void* D3D12RaytracingPipelineState::GetShaderIdentifier(std::wstring_view ExportName) const
	{
		void* ShaderIdentifier = StateObjectProperties->GetShaderIdentifier(ExportName.data());
		assert(ShaderIdentifier && "Invalid ExportName");
		return ShaderIdentifier;
	}
} // namespace RHI
