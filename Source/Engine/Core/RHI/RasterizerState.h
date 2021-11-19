#pragma once

class RasterizerState
{
public:
	[[nodiscard]] explicit operator D3D12_RASTERIZER_DESC() const noexcept;

	/// <summary>
	/// Defines triangle filling mode.
	/// </summary>
	enum class FILL_MODE
	{
		Wireframe, // Draw lines connecting the vertices.
		Solid	   // Fill the triangles formed by the vertices
	};

	/// <summary>
	/// Defines which face would be culled
	/// </summary>
	enum class CULL_MODE
	{
		None,  // Always draw all triangles
		Front, // Do not draw triangles that are front-facing
		Back   // Do not draw triangles that are back-facing
	};

	RasterizerState();

	void SetFillMode(FILL_MODE FillMode);

	void SetCullMode(CULL_MODE CullMode);

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

	FILL_MODE	 FillMode;
	CULL_MODE	 CullMode;
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

constexpr D3D12_FILL_MODE ToD3D12FillMode(RasterizerState::FILL_MODE FillMode)
{
	// clang-format off
	switch (FillMode)
	{
	case RasterizerState::FILL_MODE::Wireframe:	return D3D12_FILL_MODE_WIREFRAME;
	case RasterizerState::FILL_MODE::Solid:		return D3D12_FILL_MODE_SOLID;
	}
	return D3D12_FILL_MODE();
	// clang-format on
}

constexpr D3D12_CULL_MODE ToD3D12CullMode(RasterizerState::CULL_MODE CullMode)
{
	// clang-format off
	switch (CullMode)
	{
	case RasterizerState::CULL_MODE::None:	return D3D12_CULL_MODE_NONE;
	case RasterizerState::CULL_MODE::Front:	return D3D12_CULL_MODE_FRONT;
	case RasterizerState::CULL_MODE::Back:	return D3D12_CULL_MODE_BACK;
	}
	return D3D12_CULL_MODE();
	// clang-format on
}
