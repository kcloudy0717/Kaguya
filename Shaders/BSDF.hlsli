#ifndef BSDF_HLSLI
#define BSDF_HLSLI

#include <BxDF.hlsli>
#include <Disney.hlsli>
#include <SharedTypes.hlsli>

#define BSDFType_Lambertian (0)
#define BSDFType_Mirror		(1)
#define BSDFType_Glass		(2)
#define BSDFType_Disney		(3)

struct BSDF
{
	float3 WorldToLocal(float3 v) { return ShadingFrame.ToLocal(v); }

	float3 LocalToWorld(float3 v) { return ShadingFrame.ToWorld(v); }

	bool IsNonSpecular() { return (Flags & (BxDFFlags::Diffuse | BxDFFlags::Glossy)); }
	bool IsDiffuse() { return (Flags & BxDFFlags::Diffuse); }
	bool IsGlossy() { return (Flags & BxDFFlags::Glossy); }
	bool IsSpecular() { return (Flags & BxDFFlags::Specular); }
	bool HasReflection() { return (Flags & BxDFFlags::Reflection); }
	bool HasTransmission() { return (Flags & BxDFFlags::Transmission); }

	// Evaluate the BSDF for a pair of directions
	float3 f(float3 woW, float3 wiW)
	{
		float3 wo = WorldToLocal(woW), wi = WorldToLocal(wiW);
		if (wo.z == 0.0f)
		{
			return float3(0.0f, 0.0f, 0.0f);
		}

		if (Material.BSDFType == BSDFType_Lambertian)
		{
			LambertianReflection BxDF;
			BxDF.R = Material.baseColor;

			return BxDF.f(wo, wi);
		}

		/*
		 * The commented checks are perfectly specular BxDFs, they return 0 for their evaluation
		 */
		// if (Material.BSDFType == BSDFType_Mirror)
		//{
		//	Mirror BxDF;
		//	BxDF.R = Material.baseColor;

		//	return BxDF.f(wo, wi);
		//}

		// if (Material.BSDFType == BSDFType_Glass)
		//{
		//	Glass BxDF;
		//	BxDF.R = Material.baseColor;
		//	BxDF.T = Material.T;
		//	BxDF.etaA = Material.etaA;
		//	BxDF.etaB = Material.etaB;

		//	return BxDF.f(wo, wi);
		//}

		if (Material.BSDFType == BSDFType_Disney)
		{
			Disney BxDF;
			BxDF.baseColor		= Material.baseColor;
			BxDF.metallic		= Material.metallic;
			BxDF.subsurface		= Material.subsurface;
			BxDF.specular		= Material.specular;
			BxDF.roughness		= Material.roughness;
			BxDF.specularTint	= Material.specularTint;
			BxDF.anisotropic	= Material.anisotropic;
			BxDF.sheen			= Material.sheen;
			BxDF.sheenTint		= Material.sheenTint;
			BxDF.clearcoat		= Material.clearcoat;
			BxDF.clearcoatGloss = Material.clearcoatGloss;

			return BxDF.f(wo, wi);
		}

		return float3(0.0f, 0.0f, 0.0f);
	}

	// Compute the pdf of sampling
	float Pdf(float3 woW, float3 wiW)
	{
		float3 wo = WorldToLocal(woW), wi = WorldToLocal(wiW);
		if (wo.z == 0.0f)
		{
			return 0.0f;
		}

		if (Material.BSDFType == BSDFType_Lambertian)
		{
			LambertianReflection BxDF;
			BxDF.R = Material.baseColor;

			return BxDF.Pdf(wo, wi);
		}

		/*
		 * The commented checks are perfectly specular BxDFs, they return 0 for their evaluation
		 */
		// if (Material.BSDFType == BSDFType_Mirror)
		//{
		//	Mirror BxDF;
		//	BxDF.R = Material.baseColor;

		//	return BxDF.Pdf(wo, wi);
		//}

		// if (Material.BSDFType == BSDFType_Glass)
		//{
		//	Glass BxDF;
		//	BxDF.R = Material.baseColor;
		//	BxDF.T = Material.T;
		//	BxDF.etaA = Material.etaA;
		//	BxDF.etaB = Material.etaB;

		//	return BxDF.Pdf(wo, wi);
		//}

		if (Material.BSDFType == BSDFType_Disney)
		{
			Disney BxDF;
			BxDF.baseColor		= Material.baseColor;
			BxDF.metallic		= Material.metallic;
			BxDF.subsurface		= Material.subsurface;
			BxDF.specular		= Material.specular;
			BxDF.roughness		= Material.roughness;
			BxDF.specularTint	= Material.specularTint;
			BxDF.anisotropic	= Material.anisotropic;
			BxDF.sheen			= Material.sheen;
			BxDF.sheenTint		= Material.sheenTint;
			BxDF.clearcoat		= Material.clearcoat;
			BxDF.clearcoatGloss = Material.clearcoatGloss;

			return BxDF.Pdf(wo, wi);
		}

		return 0.0f;
	}

	// Samples the BSDF
	bool Samplef(float3 woW, float2 Xi, inout BSDFSample bsdfSample)
	{
		float3 wo = WorldToLocal(woW);
		if (wo.z == 0.0f)
		{
			return false;
		}

		bool success = false;
		if (Material.BSDFType == BSDFType_Lambertian)
		{
			LambertianReflection BxDF;
			BxDF.R = Material.baseColor;

			success = BxDF.Samplef(wo, Xi, bsdfSample);
		}

		if (Material.BSDFType == BSDFType_Mirror)
		{
			Mirror BxDF;
			BxDF.R = Material.baseColor;

			success = BxDF.Samplef(wo, Xi, bsdfSample);
		}

		if (Material.BSDFType == BSDFType_Glass)
		{
			Glass BxDF;
			BxDF.R	  = Material.baseColor;
			BxDF.T	  = Material.T;
			BxDF.etaA = Material.etaA;
			BxDF.etaB = Material.etaB;
			
			success = BxDF.Samplef(wo, Xi, bsdfSample);
		}

		if (Material.BSDFType == BSDFType_Disney)
		{
			Disney BxDF;
			BxDF.baseColor		= Material.baseColor;
			BxDF.metallic		= Material.metallic;
			BxDF.subsurface		= Material.subsurface;
			BxDF.specular		= Material.specular;
			BxDF.roughness		= Material.roughness;
			BxDF.specularTint	= Material.specularTint;
			BxDF.anisotropic	= Material.anisotropic;
			BxDF.sheen			= Material.sheen;
			BxDF.sheenTint		= Material.sheenTint;
			BxDF.clearcoat		= Material.clearcoat;
			BxDF.clearcoatGloss = Material.clearcoatGloss;

			success = BxDF.Samplef(wo, Xi, bsdfSample);
		}

		if (!success || !any(bsdfSample.f) || bsdfSample.pdf == 0.0f || bsdfSample.wi.z == 0.0f)
		{
			return false;
		}

		bsdfSample.wi = LocalToWorld(bsdfSample.wi);
		return true;
	}

	float3 Ng;
	Frame  ShadingFrame;

	Material  Material;
	BxDFFlags Flags;
};

BSDF InitBSDF(float3 Ng, Frame ShadingFrame, Material Material)
{
	BSDF bsdf;
	bsdf.Ng			  = Ng;
	bsdf.ShadingFrame = ShadingFrame;
	bsdf.Material	  = Material;
	if (Material.BSDFType == BSDFType_Lambertian)
	{
		LambertianReflection BxDF;
		bsdf.Flags = BxDF.Flags();
	}

	if (Material.BSDFType == BSDFType_Mirror)
	{
		Mirror BxDF;
		bsdf.Flags = BxDF.Flags();
	}

	if (Material.BSDFType == BSDFType_Glass)
	{
		Glass BxDF;
		bsdf.Flags = BxDF.Flags();
	}

	if (Material.BSDFType == BSDFType_Disney)
	{
		Disney BxDF;
		bsdf.Flags = BxDF.Flags();
	}

	return bsdf;
}

struct Interaction
{
	RayDesc SpawnRayTo(Interaction Interaction)
	{
		float3 d = Interaction.p - p;

		RayDesc ray	  = (RayDesc)0;
		ray.Origin	  = OffsetRay(p, n);
		ray.TMin	  = 0.01f;
		ray.Direction = normalize(d);
		ray.TMax	  = length(d);
		return ray;
	}

	float3 p; // Hit point
	float3 wo;
	float3 n; // Normal
};

struct SurfaceInteraction
{
	float3 p; // Hit point
	float3 wo;
	float3 n; // Normal
	float2 uv;
	Frame  GeometryFrame;
	Frame  ShadingFrame;
	BSDF   BSDF;

	RayDesc SpawnRay(float3 d)
	{
		RayDesc ray	  = (RayDesc)0;
		ray.Origin	  = OffsetRay(p, n);
		ray.TMin	  = 0.01f;
		ray.Direction = normalize(d);
		ray.TMax	  = 10000.0f;
		return ray;
	}
};

struct VisibilityTester
{
	Interaction I0;
	Interaction I1;
};

float3 SampleLi(
	Light				 light,
	SurfaceInteraction	 si,
	float2				 Xi,
	out float3			 pWi,
	out float			 pPdf,
	out VisibilityTester pVisibilityTester)
{
	if (light.Type == LightType_Point)
	{
		pWi	 = normalize(light.Position - si.p);
		pPdf = 1.0f;

		pVisibilityTester.I0.p	= si.p;
		pVisibilityTester.I0.wo = si.wo;
		pVisibilityTester.I0.n	= si.n;

		pVisibilityTester.I1.p	= light.Position;
		pVisibilityTester.I1.wo = float3(0.0f, 0.0f, 0.0f);
		pVisibilityTester.I1.n	= float3(0.0f, 0.0f, 0.0f);

		float d	 = length(light.Position - si.p);
		float d2 = d * d;
		return light.I / d2;
	}

	if (light.Type == LightType_Quad)
	{
		float3			   ex	 = light.Points[1] - light.Points[0];
		float3			   ey	 = light.Points[3] - light.Points[0];
		SphericalRectangle squad = InitSphericalRectangle(light.Points[0], ex, ey, si.p);

		// Pick a random point on the light
		float3 Y = SampleSphericalRectangle(squad, Xi);

		pWi	 = normalize(Y - si.p);
		pPdf = 1.0f / squad.SolidAngle;

		pVisibilityTester.I0.p	= si.p;
		pVisibilityTester.I0.wo = si.wo;
		pVisibilityTester.I0.n	= si.n;

		pVisibilityTester.I1.p	= Y;
		pVisibilityTester.I1.wo = float3(0.0f, 0.0f, 0.0f);
		pVisibilityTester.I1.n	= float3(0.0f, 0.0f, 0.0f);

		return light.I;
	}

	return float3(0.0f, 0.0f, 0.0f);
}

#endif // BSDF_HLSLI
