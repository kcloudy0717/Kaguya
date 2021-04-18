struct VSOutput
{
	float4 Position : SV_Position;
	float2 Texture	: TEXCOORD0;
};

/*
	This VS produces a big triangle that fits the entire NDC screen
*/
VSOutput VSMain(uint VertexID : SV_VertexID)
{
	VSOutput OUT;
	OUT.Texture = float2((VertexID << 1) & 2, VertexID & 2);
	OUT.Position = float4(OUT.Texture.x * 2.0f - 1.0f, -OUT.Texture.y * 2.0f + 1.0f, 0.0f, 1.0f);
	return OUT;
}