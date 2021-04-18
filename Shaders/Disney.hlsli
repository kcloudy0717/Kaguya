#ifndef DISNEY_HLSLI
#define DISNEY_HLSLI

#include <BxDF.hlsli>

// Sampling functions for disney brdf
// Refer to B in paper for sampling functions and distribution functions
float3 SampleGTR1(float2 Xi, float alpha)
{
	float phi = 2.0f * g_PI * Xi[0];
	float theta = 0.0f;
	if (alpha < 1.0f)
	{
		theta = acos(sqrt((1 - pow(pow(alpha, 2), 1 - Xi[1])) / (1 - pow(alpha, 2))));
	}
	return float3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
}

float3 SampleGTR2(float2 Xi, float alpha)
{
	float phi = 2.0f * g_PI * Xi[0];
	float theta = acos(sqrt((1 - Xi[1]) / (1 + (pow(alpha, 2) - 1) * Xi[1])));
	return float3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
}

inline float D_GTR1(float cosTheta, float alpha)
{
	// Eq 4
	float a2 = alpha * alpha;
	return (a2 - 1.0f) / (g_PI * log(a2) * (1.0f + (a2 - 1.0f) * cosTheta * cosTheta));
}

inline float D_GTR2(float cosTheta, float alpha)
{
	// Eq 8
	float a2 = alpha * alpha;
	float t = 1.0f + (a2 - 1.0f) * cosTheta * cosTheta;
	return a2 / (g_PI * t * t);
}

// Smith masking/shadowing term.
inline float smithG_GGX(float cosTheta, float alpha)
{
	float alpha2 = alpha * alpha;
	float cosTheta2 = cosTheta * cosTheta;
	return 1.0f / (cosTheta + sqrt(alpha2 + cosTheta2 - alpha2 * cosTheta2));
}

// https://seblagarde.wordpress.com/2013/04/29/memo-on-fresnel-equations/
//
// The Schlick Fresnel approximation is:
//
// R = R(0) + (1 - R(0)) (1 - cos theta)^5,
//
// where R(0) is the reflectance at normal indicence.
inline float SchlickWeight(float cosTheta)
{
	float m = clamp(1.0f - cosTheta, 0.0f, 1.0f);
	return m * m * m * m * m;
}

inline float FrSchlick(float R0, float cosTheta)
{
	return lerp(R0, 1.0f, SchlickWeight(cosTheta));
}

inline float3 FrSchlick(float3 R0, float cosTheta)
{
	return lerp(R0, float3(1.0f, 1.0f, 1.0f), SchlickWeight(cosTheta));
}

// For a dielectric, R(0) = (eta - 1)^2 / (eta + 1)^2, assuming we're
// coming from air..
inline float SchlickR0FromEta(float eta)
{
	return Sqr(eta - 1) / Sqr(eta + 1);
}

struct DisneySheen
{
	float3 f(float3 wo, float3 wi)
	{
		float3 wh = normalize(wi + wo);
		float cosThetaD = dot(wi, wh);

		return R * SchlickWeight(cosThetaD);
	}

	float3 R;
};

struct DisneyClearcoat
{
	float3 f(float3 wo, float3 wi)
	{
		float3 wh = normalize(wi + wo);

		// Clearcoat has ior = 1.5 hardcoded -> F0 = 0.04. It then uses the
		// GTR1 distribution, which has even fatter tails than Trowbridge-Reitz
		// (which is GTR2).
		float Dr = D_GTR1(AbsCosTheta(wh), gloss);
		float Fr = FrSchlick(0.04f, dot(wo, wh));
		// The geometric term always based on alpha = 0.25.
		float Gr = smithG_GGX(AbsCosTheta(wo), 0.25f) * smithG_GGX(AbsCosTheta(wi), 0.25f);

		return weight * Gr * Fr * Dr / 4.0f;
	}

	float weight, gloss;
};

struct Disney
{
	float3 f(float3 wo, float3 wi)
	{
		float3 wh = normalize(wi + wo);
		float cosThetaD = dot(wi, wh);

		float luminance = dot(baseColor, float3(0.212671f, 0.715160f, 0.072169f));
		float3 Ctint = luminance > 0.0f ? baseColor / luminance : float3(1.0f, 1.0f, 1.0f);
		float3 Cspec0 = lerp(specular * 0.08f * lerp(float3(1.0f, 1.0f, 1.0f), Ctint, specularTint), baseColor, metallic);
		float3 Csheen = lerp(float3(1.0f, 1.0f, 1.0f), Ctint, sheenTint);

		// Diffuse fresnel - go from 1 at normal incidence to .5 at grazing
		// and mix in diffuse retro-reflection based on roughness
		float Fo = SchlickWeight(AbsCosTheta(wo));
		float Fi = SchlickWeight(AbsCosTheta(wi));
		float Fd90 = 0.5f + 2.0f * cosThetaD * cosThetaD * roughness;
		float Fd = lerp(1.0f, Fd90, Fo) * lerp(1.0f, Fd90, Fi);

		// Based on Hanrahan-Krueger brdf approximation of isotropic bssrdf
		// Fss90 used to "flatten" retroreflection based on roughness
		float Fss90 = cosThetaD * cosThetaD * roughness;
		float Fss = lerp(1.0f, Fss90, Fo) * lerp(1.0f, Fss90, Fi);
		// 1.25 scale is used to (roughly) preserve albedo
		float ss = 1.25f * (Fss * (1.0f / (AbsCosTheta(wo) + AbsCosTheta(wi)) - 0.5f) + 0.5f);

		// Sheen
		DisneySheen DisneySheen;
		DisneySheen.R = sheen * Csheen;
		
		float a = max(0.001f, roughness);
		float Ds = D_GTR2(AbsCosTheta(wh), a);
		float FH = SchlickWeight(cosThetaD);
		float3 Fs = lerp(Cspec0, float3(1.0f, 1.0f, 1.0f), FH);
		float roughg = Sqr(roughness * 0.5f + 0.5f);
		float Gs = smithG_GGX(AbsCosTheta(wo), roughg) * smithG_GGX(AbsCosTheta(wi), roughg);

		// Clearcoat
		DisneyClearcoat DisneyClearcoat;
		DisneyClearcoat.weight = clearcoat;
		DisneyClearcoat.gloss = lerp(0.1f, 0.001f, clearcoatGloss);

		return ((1.0f / g_PI) * lerp(Fd, ss, subsurface) * baseColor + DisneySheen.f(wo, wi)) * (1.0f - metallic)
		+ Gs * Fs * Ds
		+ DisneyClearcoat.f(wo, wi);
	}
	
	float Pdf(float3 wo, float3 wi)
	{
		float3 wh = normalize(wo + wi);
		float cosTheta = AbsCosTheta(wh);

		float specularAlpha = max(0.001f, roughness);
		float clearcoatAlpha = lerp(0.1f, 0.001f, clearcoatGloss);

		float diffuseRatio = 0.5f * (1.0f - metallic);
		float specularRatio = 1.0f - diffuseRatio;

		float pdfGTR2 = D_GTR2(cosTheta, specularAlpha) * cosTheta;
		float pdfGTR1 = D_GTR1(cosTheta, clearcoatAlpha) * cosTheta;

		// calculate diffuse and specular pdfs and mix ratio
		float ratio = 1.0f / (1.0f + clearcoat);
		float pdfSpec = lerp(pdfGTR1, pdfGTR2, ratio) / (4.0 * abs(dot(wi, wh)));
		float pdfDiff = AbsCosTheta(wi) * g_1DIVPI;

		// weight pdfs according to ratios
		return diffuseRatio * pdfDiff + specularRatio * pdfSpec;
	}
	
	bool Samplef(float3 wo, float2 Xi, out BSDFSample bsdfSample)
	{
		// http://simon-kallweit.me/rendercompo2015/report/#disneybrdf, refer to this link
		// for how to importance sample the disney brdf

		float3 wi;

		float diffuse_ratio = 0.5f * (1.0f - metallic);

		// Sample diffuse
		if (Xi[0] < diffuse_ratio)
		{
			float2 _Xi = float2(Xi[0] / diffuse_ratio, Xi[1]);

			wi = SampleCosineHemisphere(_Xi);

			bsdfSample = InitBSDFSample(f(wo, wi), wi, Pdf(wo, wi), Flags());
		}
		// Sample specular
		else
		{
			float2 _Xi = float2((Xi[0] - diffuse_ratio) / (1.0f - diffuse_ratio), Xi[1]);

			float gtr2_ratio = 1.0f / (1.0f + clearcoat);

			if (_Xi[0] < gtr2_ratio)
			{
				_Xi[0] /= gtr2_ratio;

				float alpha = max(0.01f, pow(roughness, 2.0f));
				float3 wh = SampleGTR2(_Xi, alpha);

				wi = normalize(Reflect(wo, wh));
			}
			else
			{
				_Xi[0] = (_Xi[0] - gtr2_ratio) / (1 - gtr2_ratio);

				float alpha = lerp(0.1f, 0.001f, clearcoatGloss);
				float3 wh = SampleGTR1(_Xi, alpha);

				wi = normalize(Reflect(wo, wh));
			}

			bsdfSample = InitBSDFSample(f(wo, wi), wi, Pdf(wo, wi), Flags());
		}
		
		return true;
	}
	
	BxDFFlags Flags()
	{
		return BxDFFlags::Reflection | BxDFFlags::Diffuse | BxDFFlags::Glossy;
	}
	
	float3 baseColor;
	float metallic;
	float subsurface;
	float specular;
	float roughness;
	float specularTint;
	float anisotropic;
	float sheen;
	float sheenTint;
	float clearcoat;
	float clearcoatGloss;
};

#endif // DISNEY_HLSLI