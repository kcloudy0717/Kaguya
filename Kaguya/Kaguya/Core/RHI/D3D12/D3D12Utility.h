#pragma once

struct ShaderIdentifier
{
	ShaderIdentifier() noexcept = default;

	ShaderIdentifier(void* Data) { std::memcpy(this->Data, Data, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES); }

	BYTE Data[D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES];
};

constexpr DXGI_FORMAT GetValidDepthStencilViewFormat(DXGI_FORMAT Format)
{
	// TODO: Add more
	switch (Format)
	{
	case DXGI_FORMAT_R32_TYPELESS:
		return DXGI_FORMAT_D32_FLOAT;
	default:
		return Format;
	};
}

constexpr DXGI_FORMAT GetValidSRVFormat(DXGI_FORMAT Format)
{
	// TODO: Add more
	switch (Format)
	{
	case DXGI_FORMAT_D32_FLOAT:
		return DXGI_FORMAT_R32_FLOAT;
	default:
		return Format;
	}
};
