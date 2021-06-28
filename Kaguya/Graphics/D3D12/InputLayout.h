#pragma once

class InputLayout
{
public:
	void AddVertexLayoutElement(
		std::string_view SemanticName,
		UINT			 SemanticIndex,
		DXGI_FORMAT		 Format,
		UINT			 InputSlot,
		UINT			 AlignedByteOffset);

	void AddInstanceLayoutElement(
		std::string_view SemanticName,
		UINT			 SemanticIndex,
		DXGI_FORMAT		 Format,
		UINT			 InputSlot,
		UINT			 AlignedByteOffset,
		UINT			 InstanceDataStepRate);

	operator D3D12_INPUT_LAYOUT_DESC() const noexcept;

private:
	std::vector<std::string>					  SemanticNames;
	mutable std::vector<D3D12_INPUT_ELEMENT_DESC> InputElements; // must be mutable, as operator() resolves SemanticName
};
