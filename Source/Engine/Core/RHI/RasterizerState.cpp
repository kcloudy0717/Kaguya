#include "RasterizerState.h"

RasterizerState::operator D3D12_RASTERIZER_DESC() const noexcept
{
	D3D12_RASTERIZER_DESC Desc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	Desc.FillMode			   = ToD3D12FillMode(FillMode);
	Desc.CullMode			   = ToD3D12CullMode(CullMode);
	Desc.FrontCounterClockwise = FrontCounterClockwise ? TRUE : FALSE;
	Desc.DepthBias			   = DepthBias;
	Desc.DepthBiasClamp		   = DepthBiasClamp;
	Desc.SlopeScaledDepthBias  = SlopeScaledDepthBias;
	Desc.DepthClipEnable	   = DepthClipEnable ? TRUE : FALSE;
	Desc.MultisampleEnable	   = MultisampleEnable ? TRUE : FALSE;
	Desc.AntialiasedLineEnable = AntialiasedLineEnable ? TRUE : FALSE;
	Desc.ForcedSampleCount	   = ForcedSampleCount;
	Desc.ConservativeRaster =
		ConservativeRaster ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	return Desc;
}

RasterizerState::RasterizerState()
{
	SetFillMode(EFillMode::Solid);
	SetCullMode(ECullMode::Back);
	SetFrontCounterClockwise(false);
	SetDepthBias(0);
	SetDepthBiasClamp(0.0f);
	SetSlopeScaledDepthBias(0.0f);
	SetDepthClipEnable(true);
	SetMultisampleEnable(false);
	SetAntialiasedLineEnable(false);
	SetForcedSampleCount(0);
	SetConservativeRaster(false);
}

void RasterizerState::SetFillMode(EFillMode FillMode)
{
	this->FillMode = FillMode;
}

void RasterizerState::SetCullMode(ECullMode CullMode)
{
	this->CullMode = CullMode;
}

void RasterizerState::SetFrontCounterClockwise(bool FrontCounterClockwise)
{
	this->FrontCounterClockwise = FrontCounterClockwise;
}

void RasterizerState::SetDepthBias(int DepthBias)
{
	this->DepthBias = DepthBias;
}

void RasterizerState::SetDepthBiasClamp(float DepthBiasClamp)
{
	this->DepthBiasClamp = DepthBiasClamp;
}

void RasterizerState::SetSlopeScaledDepthBias(float SlopeScaledDepthBias)
{
	this->SlopeScaledDepthBias = SlopeScaledDepthBias;
}

void RasterizerState::SetDepthClipEnable(bool DepthClipEnable)
{
	this->DepthClipEnable = DepthClipEnable;
}

void RasterizerState::SetMultisampleEnable(bool MultisampleEnable)
{
	this->MultisampleEnable = MultisampleEnable;
}

void RasterizerState::SetAntialiasedLineEnable(bool AntialiasedLineEnable)
{
	this->AntialiasedLineEnable = AntialiasedLineEnable;
}

void RasterizerState::SetForcedSampleCount(unsigned int ForcedSampleCount)
{
	this->ForcedSampleCount = ForcedSampleCount;
}

void RasterizerState::SetConservativeRaster(bool ConservativeRaster)
{
	this->ConservativeRaster = ConservativeRaster;
}
