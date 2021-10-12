#pragma once
#include "D3D12Common.h"

enum class ED3D12PipelineStateType
{
	Graphics,
	Compute,
};

class D3D12PipelineParserCallbacks : public IPipelineParserCallbacks
{
public:
	void RootSignatureCb(D3D12RootSignature* RootSignature) override { this->RootSignature = RootSignature; }

	void VSCb(Shader* VS) override
	{
		Type	 = ED3D12PipelineStateType::Graphics;
		this->VS = VS;
	}
	void PSCb(Shader* PS) override
	{
		Type	 = ED3D12PipelineStateType::Graphics;
		this->PS = PS;
	}
	void DSCb(Shader* DS) override
	{
		Type	 = ED3D12PipelineStateType::Graphics;
		this->DS = DS;
	}
	void HSCb(Shader* HS) override
	{
		Type	 = ED3D12PipelineStateType::Graphics;
		this->HS = HS;
	}
	void GSCb(Shader* GS) override
	{
		Type	 = ED3D12PipelineStateType::Graphics;
		this->GS = GS;
	}
	void CSCb(Shader* CS) override
	{
		Type	 = ED3D12PipelineStateType::Compute;
		this->CS = CS;
	}
	void MSCb(Shader* MS) override
	{
		Type	 = ED3D12PipelineStateType::Graphics;
		this->MS = MS;
	}

	void BlendStateCb(const BlendState& BlendState) override
	{
		Type			 = ED3D12PipelineStateType::Graphics;
		this->BlendState = BlendState;
	}

	void RasterizerStateCb(const RasterizerState& RasterizerState) override
	{
		Type				  = ED3D12PipelineStateType::Graphics;
		this->RasterizerState = RasterizerState;
	}

	void DepthStencilStateCb(const DepthStencilState& DepthStencilState) override
	{
		Type					= ED3D12PipelineStateType::Graphics;
		this->DepthStencilState = DepthStencilState;
	}

	void InputLayoutCb(const D3D12InputLayout& InputLayout) override
	{
		Type			  = ED3D12PipelineStateType::Graphics;
		this->InputLayout = InputLayout;
	}

	void PrimitiveTopologyTypeCb(PrimitiveTopology PrimitiveTopology) override
	{
		Type					= ED3D12PipelineStateType::Graphics;
		this->PrimitiveTopology = PrimitiveTopology;
	}

	void RenderPassCb(D3D12RenderPass* RenderPass) override
	{
		Type			 = ED3D12PipelineStateType::Graphics;
		this->RenderPass = RenderPass;
	}

	void ErrorBadInputParameter(UINT) override
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void ErrorDuplicateSubobject(PipelineStateSubobjectType) override
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void ErrorUnknownSubobject(UINT) override { throw std::logic_error("The method or operation is not implemented."); }

	ED3D12PipelineStateType Type;
	D3D12RootSignature*		RootSignature = nullptr;
	Shader*					VS			  = nullptr;
	Shader*					PS			  = nullptr;
	Shader*					DS			  = nullptr;
	Shader*					HS			  = nullptr;
	Shader*					GS			  = nullptr;
	Shader*					CS			  = nullptr;
	Shader*					MS			  = nullptr;
	BlendState				BlendState;
	RasterizerState			RasterizerState;
	DepthStencilState		DepthStencilState;
	D3D12InputLayout		InputLayout;
	PrimitiveTopology		PrimitiveTopology;
	D3D12RenderPass*		RenderPass;
};

class D3D12PipelineState : public D3D12DeviceChild
{
public:
	D3D12PipelineState(D3D12Device* Parent, const PipelineStateStreamDesc& Desc);

	[[nodiscard]] ID3D12PipelineState*	  GetApiHandle() const noexcept;
	[[nodiscard]] ED3D12PipelineStateType GetType() const noexcept { return Type; }

private:
	template<ED3D12PipelineStateType Type>
	void Create(D3D12PipelineParserCallbacks Parser);

	template<ED3D12PipelineStateType Type>
	void InternalCreate(const D3D12PipelineParserCallbacks& Parser);

private:
	Microsoft::WRL::ComPtr<ID3D12PipelineState> PipelineState;
	ED3D12PipelineStateType						Type;
	mutable std::unique_ptr<ThreadPoolWork>		CompilationWork;
};
