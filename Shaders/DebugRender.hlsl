struct VertexAttributes
{
	float4 Position : SV_POSITION;
	float3 Color : COLOR;
};

cbuffer RootConstants : register(b0)
{
	float4x4 MVP;
};

VertexAttributes VSMain(float3 Position : POSITION, float3 Color : COLOR)
{
	VertexAttributes va;
	va.Position = mul(float4(Position, 1.0f), MVP);
	va.Color	= Color;
	return va;
}

struct MRT
{
	float4 Albedo : SV_Target0;
	float4 Normal : SV_Target1;
	float2 Motion : SV_Target2;
};

MRT PSMain(VertexAttributes va)
{
	MRT mrt;
	mrt.Albedo = float4(va.Color, 1.0f);
	mrt.Normal = float4(va.Color, 1.0f);
	mrt.Motion = va.Color.xy;
	return mrt;
}
