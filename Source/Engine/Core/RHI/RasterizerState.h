#pragma once

class RasterizerState
{
public:
	[[nodiscard]] explicit operator D3D12_RASTERIZER_DESC() const noexcept;

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

constexpr D3D12_FILL_MODE ToD3D12FillMode(RasterizerState::EFillMode FillMode)
{
	// clang-format off
	switch (FillMode)
	{
	case RasterizerState::EFillMode::Wireframe:	return D3D12_FILL_MODE_WIREFRAME;
	case RasterizerState::EFillMode::Solid:		return D3D12_FILL_MODE_SOLID;
	}
	return D3D12_FILL_MODE();
	// clang-format on
}

constexpr D3D12_CULL_MODE ToD3D12CullMode(RasterizerState::ECullMode CullMode)
{
	// clang-format off
	switch (CullMode)
	{
	case RasterizerState::ECullMode::None:	return D3D12_CULL_MODE_NONE;
	case RasterizerState::ECullMode::Front:	return D3D12_CULL_MODE_FRONT;
	case RasterizerState::ECullMode::Back:	return D3D12_CULL_MODE_BACK;
	}
	return D3D12_CULL_MODE();
	// clang-format on
}
