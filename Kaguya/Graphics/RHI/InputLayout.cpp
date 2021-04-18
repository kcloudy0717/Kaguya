#include "pch.h"
#include "InputLayout.h"
#include "D3D12/d3dx12.h"

void InputLayout::AddVertexLayoutElement(LPCSTR SemanticName, UINT SemanticIndex, DXGI_FORMAT Format, UINT InputSlot, UINT AlignedByteOffset)
{
	m_InputElementSemanticNames.push_back(SemanticName);

	D3D12_INPUT_ELEMENT_DESC Desc	= {};
	{
		Desc.SemanticName			= nullptr; // Will be resolved later
		Desc.SemanticIndex			= SemanticIndex;
		Desc.Format					= Format;
		Desc.InputSlot				= InputSlot;
		Desc.AlignedByteOffset		= AlignedByteOffset;
		Desc.InputSlotClass			= D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		Desc.InstanceDataStepRate	= 0;
	}
	m_InputElements.push_back(Desc);
}

void InputLayout::AddInstanceLayoutElement(LPCSTR SemanticName, UINT SemanticIndex, DXGI_FORMAT Format, UINT InputSlot, UINT AlignedByteOffset, UINT InstanceDataStepRate)
{
	m_InputElementSemanticNames.push_back(SemanticName);

	D3D12_INPUT_ELEMENT_DESC Desc	= {};
	{
		Desc.SemanticName			= nullptr; // Will be resolved later
		Desc.SemanticIndex			= SemanticIndex;
		Desc.Format					= Format;
		Desc.InputSlot				= InputSlot;
		Desc.AlignedByteOffset		= AlignedByteOffset;
		Desc.InputSlotClass			= D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
		Desc.InstanceDataStepRate	= InstanceDataStepRate;
	}
	m_InputElements.push_back(Desc);
}

InputLayout::operator D3D12_INPUT_LAYOUT_DESC() const noexcept
{
	for (size_t i = 0; i < m_InputElements.size(); ++i)
	{
		m_InputElements[i].SemanticName = m_InputElementSemanticNames[i].data();
	}

	D3D12_INPUT_LAYOUT_DESC Desc = {};
	{
		Desc.pInputElementDescs = m_InputElements.data();
		Desc.NumElements		= static_cast<UINT>(m_InputElements.size());
	}
	return Desc;
}