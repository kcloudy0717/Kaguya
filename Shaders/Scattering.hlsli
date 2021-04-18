#ifndef SCATTERING_HLSLI
#define SCATTERING_HLSLI

#include <Math.hlsli>

float PhaseHenyeyGreenstein(float cosTheta, float g)
{
	float g2 = Sqr(g);
	float denom = 1 + g2 + 2 * g * cosTheta;
	return g_1DIV4PI * (1 - g2) / (denom * sqrt(denom));
}

struct HenyeyGreenstein
{
	float p(float3 wo, float3 wi)
	{
		float cosTheta = dot(wo, wi);
		return PhaseHenyeyGreenstein(cosTheta, g);
	}
	
	float Samplep(float3 wo, float2 Xi, out float3 wi)
	{
		
	}
	
	float g;
};

#endif // SCATTERING_HLSLI