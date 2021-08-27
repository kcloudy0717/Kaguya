#include "D3D12Utility.h"

PIXCapture::PIXCapture()
{
	if (SUCCEEDED(::DXGIGetDebugInterface1(0, IID_PPV_ARGS(GraphicsAnalysis.ReleaseAndGetAddressOf()))))
	{
		GraphicsAnalysis->BeginCapture();
	}
}

PIXCapture::~PIXCapture()
{
	if (GraphicsAnalysis)
	{
		GraphicsAnalysis->EndCapture();
	}
}

void D3D12InputLayout::AddVertexLayoutElement(
	std::string_view SemanticName,
	UINT			 SemanticIndex,
	DXGI_FORMAT		 Format,
	UINT			 InputSlot,
	UINT			 AlignedByteOffset)
{
	SemanticNames.emplace_back(SemanticName);

	D3D12_INPUT_ELEMENT_DESC& Desc = InputElements.emplace_back();
	Desc.SemanticName			   = nullptr; // Will be resolved later
	Desc.SemanticIndex			   = SemanticIndex;
	Desc.Format					   = Format;
	Desc.InputSlot				   = InputSlot;
	Desc.AlignedByteOffset		   = AlignedByteOffset;
	Desc.InputSlotClass			   = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	Desc.InstanceDataStepRate	   = 0;
}

D3D12InputLayout::operator D3D12_INPUT_LAYOUT_DESC() const noexcept
{
	for (size_t i = 0; i < InputElements.size(); ++i)
	{
		InputElements[i].SemanticName = SemanticNames[i].data();
	}

	return { .pInputElementDescs = InputElements.data(), .NumElements = static_cast<UINT>(InputElements.size()) };
}
