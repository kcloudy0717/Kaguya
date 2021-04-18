#pragma once
#include <d3d12.h>
#include "AbstractionLayer.h"

class InputLayout
{
public:
	void AddVertexLayoutElement(LPCSTR SemanticName, UINT SemanticIndex, DXGI_FORMAT Format, UINT InputSlot, UINT AlignedByteOffset);

	void AddInstanceLayoutElement(LPCSTR SemanticName, UINT SemanticIndex, DXGI_FORMAT Format, UINT InputSlot, UINT AlignedByteOffset, UINT InstanceDataStepRate);

	operator D3D12_INPUT_LAYOUT_DESC() const noexcept;
private:
	std::vector<std::string>						m_InputElementSemanticNames;
	mutable std::vector<D3D12_INPUT_ELEMENT_DESC>	m_InputElements; // must be mutable, as operator() resolves SemanticName
};