struct VSOutput
{
	float4					   Pos : SV_POSITION;
	[[vk::location(0)]] float3 Color : COLOR;
};

VSOutput main(uint VertexID : SV_VertexID)
{
	// const array of positions for the triangle
    const float3 positions[3] = { float3(0.5f, 0.5f, 0.0f), float3(-0.5f, 0.5f, 0.0f), float3(0.f, -0.5f, 0.0f) };
	// const array of colors for the triangle
	const float3 colors[3] = { float3(1.0f, 0.0f, 0.0f),   // red
							   float3(0.0f, 1.0f, 0.0f),   // green
							   float3(0.0f, 0.0f, 1.0f) }; // blue

	VSOutput output = (VSOutput)0;
	output.Pos		= float4(positions[VertexID], 1.0f);
	output.Color	= colors[VertexID];
	return output;
}
