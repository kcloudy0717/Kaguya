#ifndef SAMPLING_HLSLI
#define SAMPLING_HLSLI

#include "Math.hlsli"

float2 SampleUniformDisk(float2 Xi)
{
	float radius = sqrt(Xi[0]);
	float theta	 = g_2PI * Xi[1];

	return float2(radius * cos(theta), radius * sin(theta));
}

float2 SampleConcentricDisk(float2 Xi)
{
	// Map Xi to $[-1,1]^2$
	float2 XiOffset = 2.0f * Xi - 1.0f;

	// Handle degeneracy at the origin
	if (XiOffset.x == 0.0f && XiOffset.y == 0.0f)
	{
		return float2(0.0f, 0.0f);
	}

	// Apply concentric mapping to point
	float radius, theta;
	if (abs(XiOffset.x) > abs(XiOffset.y))
	{
		radius = XiOffset.x;
		theta  = g_PIDIV4 * (XiOffset.y / XiOffset.x);
	}
	else
	{
		radius = XiOffset.y;
		theta  = g_PIDIV2 - g_PIDIV4 * (XiOffset.x / XiOffset.y);
	}

	return float2(radius * cos(theta), radius * sin(theta));
}

float3 SampleUniformHemisphere(float2 Xi)
{
	float z	  = Xi[0];
	float r	  = sqrt(max(0.0f, 1.0f - z * z));
	float phi = 2.0f * g_PI * Xi[1];
	return float3(r * cos(phi), r * sin(phi), z);
}

float UniformHemispherePdf()
{
	return g_1DIV2PI;
}

float3 SampleCosineHemisphere(float2 Xi)
{
	float2 p = SampleConcentricDisk(Xi);
	float  z = sqrt(max(0.0f, 1.0f - p.x * p.x - p.y * p.y));

	return float3(p.x, p.y, z);
}

float CosineHemispherePdf(float cosTheta)
{
	return cosTheta * g_1DIVPI;
}

float BalanceHeuristic(int nf, float fPdf, int ng, float gPdf)
{
	return (nf * fPdf) / (nf * fPdf + ng * gPdf);
}

float PowerHeuristic(int nf, float fPdf, int ng, float gPdf)
{
	float f = nf * fPdf, g = ng * gPdf;
	return (f * f) / (f * f + g * g);
}

// Sampling the Solid Angle of Area Light Sources
// https://schuttejoe.github.io/post/arealightsampling/

// Solid-angle rectangular light sampling yields lower variance and faster convergence.
// An Area-Preserving Parametrization for Spherical Rectangles:
// https://www.arnoldrenderer.com/research/egsr2013_spherical_rectangle.pdf
struct SphericalRectangle
{
	float3 o, x, y, z;
	float  z0, z0sq;
	float  x0, y0, y0sq;
	float  x1, y1, y1sq;
	float  b0, b1, b0sq, k;
	float  SolidAngle;
};

SphericalRectangle InitSphericalRectangle(float3 s, float3 ex, float3 ey, float3 o)
{
	SphericalRectangle squad;

	squad.o	  = o;
	float exl = length(ex);
	float eyl = length(ey);

	// compute local reference system 'R'
	squad.x = ex / exl;
	squad.y = ey / eyl;
	squad.z = cross(squad.x, squad.y);

	// compute rectangle coords in local reference system
	float3 d = s - o;
	squad.z0 = dot(d, squad.z);

	// flip 'z' to make it point against 'Q'
	if (squad.z0 > 0.0f)
	{
		squad.z	 = -squad.z;
		squad.z0 = -squad.z0;
	}

	squad.z0sq = squad.z0 * squad.z0;
	squad.x0   = dot(d, squad.x);
	squad.y0   = dot(d, squad.y);
	squad.x1   = squad.x0 + exl;
	squad.y1   = squad.y0 + eyl;
	squad.y0sq = squad.y0 * squad.y0;
	squad.y1sq = squad.y1 * squad.y1;

	// create vectors to four vertices
	float3 v00 = float3(squad.x0, squad.y0, squad.z0);
	float3 v01 = float3(squad.x0, squad.y1, squad.z0);
	float3 v10 = float3(squad.x1, squad.y0, squad.z0);
	float3 v11 = float3(squad.x1, squad.y1, squad.z0);

	// compute normals to edges
	float3 n0 = normalize(cross(v00, v10));
	float3 n1 = normalize(cross(v10, v11));
	float3 n2 = normalize(cross(v11, v01));
	float3 n3 = normalize(cross(v01, v00));

	// compute internal angles (gamma_i)
	float g0 = acos(-dot(n0, n1));
	float g1 = acos(-dot(n1, n2));
	float g2 = acos(-dot(n2, n3));
	float g3 = acos(-dot(n3, n0));

	// compute predefined constants
	squad.b0   = n0.z;
	squad.b1   = n2.z;
	squad.b0sq = squad.b0 * squad.b0;
	squad.k	   = 2.0f * g_PI - g2 - g3;

	// compute solid angle from internal angles
	squad.SolidAngle = g0 + g1 - squad.k;

	return squad;
}

float3 SampleSphericalRectangle(SphericalRectangle squad, float2 Xi)
{
	// 1. compute 'cu'
	float au = Xi[0] * squad.SolidAngle + squad.k;
	float fu = (cos(au) * squad.b0 - squad.b1) / sin(au);
	float cu = 1.0f / sqrt(fu * fu + squad.b0sq) * (fu > 0.0f ? 1.0f : -1.0f);
	cu		 = clamp(cu, -1.0f, 1.0f); // avoid NaNs

	// 2. compute 'xu'
	float xu = -(cu * squad.z0) / sqrt(1.0f - cu * cu);
	xu		 = clamp(xu, squad.x0, squad.x1); // avoid Infs

	// 3. compute 'yv'
	float d	 = sqrt(xu * xu + squad.z0sq);
	float h0 = squad.y0 / sqrt(d * d + squad.y0sq);
	float h1 = squad.y1 / sqrt(d * d + squad.y1sq);
	float hv = h0 + Xi[1] * (h1 - h0), hv2 = hv * hv;
	float yv = (hv2 < 1.0f - 1e-6f) ? (hv * d) / sqrt(1.0f - hv2) : squad.y1;

	// 4. transform (xu, yv, z0) to world coords
	return squad.o + xu * squad.x + yv * squad.y + squad.z0 * squad.z;
}

#endif // SAMPLING_HLSLI
