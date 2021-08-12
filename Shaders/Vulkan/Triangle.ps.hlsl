
float4 main([[vk::location(0)]] float3 Color : COLOR) : SV_TARGET
{
    return float4(Color, 1);
}
