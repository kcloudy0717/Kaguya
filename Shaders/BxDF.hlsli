#pragma once
#include "Math.hlsli"
#include "Sampling.hlsli"

void swap(inout float a, inout float b)
{
	float tmp = a;
	a		  = b;
	b		  = tmp;
}

float FrDielectric(float cosThetaI, float etaI, float etaT)
{
	if (etaI == etaT)
	{
		return 0.0f;
	}

	cosThetaI = clamp(cosThetaI, -1.0f, 1.0f);

	// Potentially swap indices of refraction
	bool entering = cosThetaI > 0.0f;
	if (!entering)
	{
		swap(etaI, etaT);
		cosThetaI = abs(cosThetaI);
	}

	// Compute cosThetaT using Snell's law
	float sinThetaI = sqrt(max(0.0f, 1.0f - cosThetaI * cosThetaI));
	float sinThetaT = etaI / etaT * sinThetaI;

	// Handle total internal reflection
	if (sinThetaT >= 1.0f)
	{
		return 1.0f;
	}

	float cosThetaT = sqrt(max(0.0f, 1.0f - sinThetaT * sinThetaT));

	float Rparl = ((etaT * cosThetaI) - (etaI * cosThetaT)) / ((etaT * cosThetaI) + (etaI * cosThetaT));
	float Rperp = ((etaI * cosThetaI) - (etaT * cosThetaT)) / ((etaI * cosThetaI) + (etaT * cosThetaT));
	return (Rparl * Rparl + Rperp * Rperp) * 0.5f;
}

inline float CosTheta(float3 w)
{
	return w.z;
}
inline float Cos2Theta(float3 w)
{
	return w.z * w.z;
}
inline float AbsCosTheta(float3 w)
{
	return abs(w.z);
}
inline float Sin2Theta(float3 w)
{
	return max(0.0f, 1.0f - Cos2Theta(w));
}
inline float SinTheta(float3 w)
{
	return sqrt(Sin2Theta(w));
}
inline float TanTheta(float3 w)
{
	return SinTheta(w) / CosTheta(w);
}
inline float Tan2Theta(float3 w)
{
	return Sin2Theta(w) / Cos2Theta(w);
}
inline float CosPhi(float3 w)
{
	float sinTheta = SinTheta(w);
	return (sinTheta == 0.0f) ? 1.0f : clamp(w.x / sinTheta, -1.0f, 1.0f);
}
inline float SinPhi(float3 w)
{
	float sinTheta = SinTheta(w);
	return (sinTheta == 0.0f) ? 0.0f : clamp(w.y / sinTheta, -1.0f, 1.0f);
}
inline float Cos2Phi(float3 w)
{
	return CosPhi(w) * CosPhi(w);
}
inline float Sin2Phi(float3 w)
{
	return SinPhi(w) * SinPhi(w);
}

inline bool SameHemisphere(float3 v0, float3 v1)
{
	return v0.z * v1.z > 0;
}

inline float3 Reflect(float3 wo, float3 n)
{
	return -wo + 2.0f * dot(wo, n) * n;
}

inline bool Refract(float3 wi, float3 n, float eta, out float3 wt)
{
	// Compute $\cos \theta_\roman{t}$ using Snell's law
	float cosThetaI	 = dot(n, wi);
	float sin2ThetaI = max(0.0f, float(1.0f - cosThetaI * cosThetaI));
	float sin2ThetaT = eta * eta * sin2ThetaI;

	// Handle total internal reflection for transmission
	if (sin2ThetaT >= 1)
		return false;
	float cosThetaT = sqrt(1 - sin2ThetaT);
	wt				= eta * -wi + (eta * cosThetaI - cosThetaT) * n;
	return true;
}

#define BxDFTypes_Unknown	   (0)
#define BxDFTypes_Reflection   (1 << 0)
#define BxDFTypes_Transmission (1 << 1)
#define BxDFTypes_All		   (BxDFTypes_Reflection | BxDFTypes_Transmission)

enum BxDFFlags
{
	Unknown		 = 0,
	Reflection	 = 1 << 0,
	Transmission = 1 << 1,

	Diffuse	 = 1 << 2,
	Glossy	 = 1 << 3,
	Specular = 1 << 4,
	// Composite flags definitions
	DiffuseReflection	 = Diffuse | Reflection,
	DiffuseTransmission	 = Diffuse | Transmission,
	GlossyReflection	 = Glossy | Reflection,
	GlossyTransmission	 = Glossy | Transmission,
	SpecularReflection	 = Specular | Reflection,
	SpecularTransmission = Specular | Transmission,

	All = Diffuse | Glossy | Specular | Reflection | Transmission
};

#define BSDFSample(...) static BSDFSample ctor(__VA_ARGS__)
struct BSDFSample
{
	BSDFSample()
	{
		return (BSDFSample)0;
	}

	BSDFSample(float3 f, float3 wi, float pdf, BxDFFlags flags)
	{
		BSDFSample object = ctor();
		object.f		  = f;
		object.wi		  = wi;
		object.pdf		  = pdf;
		object.flags	  = flags;
		return object;
	}

	float3	  f;
	float3	  wi;
	float	  pdf;
	BxDFFlags flags;
};
#undef BSDFSample
#define BSDFSample(...) BSDFSample::ctor(__VA_ARGS__)

struct LambertianReflection
{
	float3 R;

	float3 f(float3 wo, float3 wi)
	{
		if (!SameHemisphere(wo, wi))
		{
			return float3(0.0f, 0.0f, 0.0f);
		}

		return R * g_1DIVPI;
	}

	float Pdf(float3 wo, float3 wi)
	{
		if (!SameHemisphere(wo, wi))
		{
			return 0.0f;
		}

		return CosineHemispherePdf(AbsCosTheta(wi));
	}

	bool Samplef(float3 wo, float2 Xi, inout BSDFSample bsdfSample)
	{
		float3 wi = SampleCosineHemisphere(Xi);
		if (wo.z < 0.0f)
		{
			wi.z *= -1.0f;
		}

		float pdf = CosineHemispherePdf(AbsCosTheta(wi));

		bsdfSample = BSDFSample(R * g_1DIVPI, wi, pdf, Flags());

		return true;
	}

	BxDFFlags Flags() { return BxDFFlags::DiffuseReflection; }
};

struct Mirror
{
	float3 R;

	float3 f(float3 wo, float3 wi) { return float3(0.0f, 0.0f, 0.0f); }

	float Pdf(float3 wo, float3 wi) { return 0.0f; }

	bool Samplef(float3 wo, float2 Xi, inout BSDFSample bsdfSample)
	{
		float3 wi = float3(-wo.x, -wo.y, wo.z);

		bsdfSample = BSDFSample(R / AbsCosTheta(wi), wi, 1.0f, Flags());

		return true;
	}

	BxDFFlags Flags() { return BxDFFlags::SpecularReflection; }
};

struct Glass
{
	float3 R;
	float3 T;
	float  etaA, etaB;

	float3 f(float3 wo, float3 wi) { return float3(0.0f, 0.0f, 0.0f); }

	float Pdf(float3 wo, float3 wi) { return 0.0f; }

	bool Samplef(float3 wo, float2 Xi, inout BSDFSample bsdfSample)
	{
		float3	  f	  = float3(0, 0, 0);
		float3	  wi  = float3(0, 0, 0);
		float	  pdf = 0;
		BxDFFlags flags;

		float F = FrDielectric(CosTheta(wo), etaA, etaB);
		if (Xi[0] < F)
		{
			// Compute specular reflection
			// Compute perfect specular reflection direction
			wi	  = float3(-wo.x, -wo.y, wo.z);
			pdf	  = F;
			f	  = F * R / AbsCosTheta(wi);
			flags = BxDFFlags::SpecularReflection;
		}
		else
		{
			// Compute specular transmission
			// Figure out which $\eta$ is incident and which is transmitted
			bool  entering = CosTheta(wo) > 0;
			float etaI = etaA, etaT = etaB;
			if (!entering)
			{
				swap(etaI, etaT);
			}

			// Compute ray direction for specular transmission
			if (!Refract(wo, Faceforward(float3(0, 0, 1), wo), etaI / etaT, wi))
			{
				return false;
			}

			float3 ft = T * (1.0f - F);

			// Account for non-symmetry with transmission to different medium
			ft *= (etaI * etaI) / (etaT * etaT);
			pdf = 1 - F;

			f	  = ft / AbsCosTheta(wi);
			flags = BxDFFlags::SpecularTransmission;
		}

		bsdfSample = BSDFSample(f, wi, pdf, flags);
		return true;
	}

	BxDFFlags Flags() { return BxDFFlags::Reflection | BxDFFlags::Transmission | BxDFFlags::Specular; }
};

// Sampling functions for disney brdf
// Refer to B in paper for sampling functions and distribution functions
float3 SampleGTR1(float2 Xi, float alpha)
{
	float phi	= 2.0f * g_PI * Xi[0];
	float theta = 0.0f;
	if (alpha < 1.0f)
	{
		theta = acos(sqrt((1 - pow(pow(alpha, 2), 1 - Xi[1])) / (1 - pow(alpha, 2))));
	}
	return float3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
}

float3 SampleGTR2(float2 Xi, float alpha)
{
	float phi	= 2.0f * g_PI * Xi[0];
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
	float t	 = 1.0f + (a2 - 1.0f) * cosTheta * cosTheta;
	return a2 / (g_PI * t * t);
}

// Smith masking/shadowing term.
inline float smithG_GGX(float cosTheta, float alpha)
{
	float alpha2	= alpha * alpha;
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
	float3 R;

	float3 f(float3 wo, float3 wi, float3 wh)
	{
		float cosThetaD = dot(wi, wh);

		return R * SchlickWeight(cosThetaD);
	}
};

struct DisneyClearcoat
{
	float weight, gloss;

	float3 f(float3 wo, float3 wi, float3 wh)
	{
		// Clearcoat has ior = 1.5 hardcoded -> F0 = 0.04. It then uses the
		// GTR1 distribution, which has even fatter tails than Trowbridge-Reitz
		// (which is GTR2).
		float Dr = D_GTR1(AbsCosTheta(wh), gloss);
		float Fr = FrSchlick(0.04f, dot(wo, wh));
		// The geometric term always based on alpha = 0.25.
		float Gr = smithG_GGX(AbsCosTheta(wo), 0.25f) * smithG_GGX(AbsCosTheta(wi), 0.25f);

		return weight * Gr * Fr * Dr / 4.0f;
	}
};

struct Disney
{
	float3 baseColor;
	float  metallic;
	float  subsurface;
	float  specular;
	float  roughness;
	float  specularTint;
	float  anisotropic;
	float  sheen;
	float  sheenTint;
	float  clearcoat;
	float  clearcoatGloss;

	float3 f(float3 wo, float3 wi)
	{
		float3 wh		 = normalize(wi + wo);
		float  cosThetaD = dot(wi, wh);

		float  luminance = dot(baseColor, float3(0.212671f, 0.715160f, 0.072169f));
		float3 Ctint	 = luminance > 0.0f ? baseColor / luminance : float3(1.0f, 1.0f, 1.0f);
		float3 Cspec0 =
			lerp(specular * 0.08f * lerp(float3(1.0f, 1.0f, 1.0f), Ctint, specularTint), baseColor, metallic);
		float3 Csheen = lerp(float3(1.0f, 1.0f, 1.0f), Ctint, sheenTint);

		// Diffuse fresnel - go from 1 at normal incidence to .5 at grazing
		// and mix in diffuse retro-reflection based on roughness
		float Fo   = SchlickWeight(AbsCosTheta(wo));
		float Fi   = SchlickWeight(AbsCosTheta(wi));
		float Fd90 = 0.5f + 2.0f * cosThetaD * cosThetaD * roughness;
		float Fd   = lerp(1.0f, Fd90, Fo) * lerp(1.0f, Fd90, Fi);

		// Based on Hanrahan-Krueger brdf approximation of isotropic bssrdf
		// Fss90 used to "flatten" retroreflection based on roughness
		float Fss90 = cosThetaD * cosThetaD * roughness;
		float Fss	= lerp(1.0f, Fss90, Fo) * lerp(1.0f, Fss90, Fi);
		// 1.25 scale is used to (roughly) preserve albedo
		float ss = 1.25f * (Fss * (1.0f / (AbsCosTheta(wo) + AbsCosTheta(wi)) - 0.5f) + 0.5f);

		// Sheen
		DisneySheen DisneySheen;
		DisneySheen.R = sheen * Csheen;

		float  a	  = max(0.001f, roughness);
		float  Ds	  = D_GTR2(AbsCosTheta(wh), a);
		float  FH	  = SchlickWeight(cosThetaD);
		float3 Fs	  = lerp(Cspec0, float3(1.0f, 1.0f, 1.0f), FH);
		float  roughg = Sqr(roughness * 0.5f + 0.5f);
		float  Gs	  = smithG_GGX(AbsCosTheta(wo), roughg) * smithG_GGX(AbsCosTheta(wi), roughg);

		// Clearcoat
		DisneyClearcoat DisneyClearcoat;
		DisneyClearcoat.weight = clearcoat;
		DisneyClearcoat.gloss  = lerp(0.1f, 0.001f, clearcoatGloss);

		return ((1.0f / g_PI) * lerp(Fd, ss, subsurface) * baseColor + DisneySheen.f(wo, wi, wh)) * (1.0f - metallic) +
			   Gs * Fs * Ds + DisneyClearcoat.f(wo, wi, wh);
	}

	float Pdf(float3 wo, float3 wi)
	{
		float3 wh		= normalize(wo + wi);
		float  cosTheta = AbsCosTheta(wh);

		float specularAlpha	 = max(0.001f, roughness);
		float clearcoatAlpha = lerp(0.1f, 0.001f, clearcoatGloss);

		float diffuseR	= 0.5f * (1.0f - metallic);
		float specularR = 1.0f - diffuseR;

		float pdfGTR2 = D_GTR2(cosTheta, specularAlpha) * cosTheta;
		float pdfGTR1 = D_GTR1(cosTheta, clearcoatAlpha) * cosTheta;

		// calculate diffuse and specular pdfs and mix ratio
		float GTR2R	  = 1.0f / (1.0f + clearcoat);
		float pdfSpec = lerp(pdfGTR1, pdfGTR2, GTR2R) / (4.0 * abs(dot(wi, wh)));
		float pdfDiff = AbsCosTheta(wi) * g_1DIVPI;

		// weight pdfs according to ratios
		return diffuseR * pdfDiff + specularR * pdfSpec;
	}

	bool Samplef(float3 wo, float2 Xi, out BSDFSample bsdfSample)
	{
		// http://simon-kallweit.me/rendercompo2015/report/#disneybrdf, refer to this link
		// for how to importance sample the disney brdf

		float2 remappedXi;
		float3 wi;
		float  diffuseR = 0.5f * (1.0f - metallic);

		// Sample diffuse
		if (Xi[0] < diffuseR)
		{
			remappedXi = float2(Xi[0] / diffuseR, Xi[1]);
			wi		   = SampleCosineHemisphere(remappedXi);
			bsdfSample = BSDFSample(f(wo, wi), wi, Pdf(wo, wi), Flags());
		}
		// Sample specular
		else
		{
			remappedXi = float2((Xi[0] - diffuseR) / (1.0f - diffuseR), Xi[1]);

			float GTR2R = 1.0f / (1.0f + clearcoat);

			if (remappedXi[0] < GTR2R)
			{
				remappedXi[0] /= GTR2R;

				float  alpha = max(0.01f, pow(roughness, 2.0f));
				float3 wh	 = SampleGTR2(remappedXi, alpha);
				wi			 = normalize(Reflect(wo, wh));
			}
			else
			{
				remappedXi[0] = (remappedXi[0] - GTR2R) / (1 - GTR2R);

				float  alpha = lerp(0.1f, 0.001f, clearcoatGloss);
				float3 wh	 = SampleGTR1(remappedXi, alpha);
				wi			 = normalize(Reflect(wo, wh));
			}

			bsdfSample = BSDFSample(f(wo, wi), wi, Pdf(wo, wi), Flags());
		}

		return true;
	}

	BxDFFlags Flags() { return BxDFFlags::Reflection | BxDFFlags::Diffuse | BxDFFlags::Glossy; }
};
