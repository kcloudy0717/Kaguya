struct VertexAttributes
{
	float4 Position : SV_Position;
	float2 Texture : TEXCOORD0;
};

VertexAttributes VSMain(uint VertexID : SV_VertexID)
{
	VertexAttributes va;
	va.Texture	= float2((VertexID << 1) & 2, VertexID & 2);
	va.Position = float4(va.Texture.x * 2.0f - 1.0f, -va.Texture.y * 2.0f + 1.0f, 0.0f, 1.0f);
	return va;
}
