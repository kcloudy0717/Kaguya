#ifndef MATH_HLSLI
#define MATH_HLSLI

static const float g_EPSILON = 1e-4f;

static const float g_PI		 = 3.141592654f;
static const float g_2PI	 = 6.283185307f;
static const float g_1DIVPI	 = 0.318309886f;
static const float g_1DIV2PI = 0.159154943f;
static const float g_1DIV4PI = 0.079577471f;
static const float g_PIDIV2	 = 1.570796327f;
static const float g_PIDIV4	 = 0.785398163f;

static const float FLT_MAX = asfloat(0x7F7FFFFF);

float3 Faceforward(float3 n, float3 v)
{
	return (dot(n, v) < 0.0f) ? -n : n;
}

float Sqr(float x)
{
	return x * x;
}

float SphericalTheta(float3 v)
{
	return acos(clamp(v.z, -1.0f, 1.0f));
}

float SphericalPhi(float3 v)
{
	float p = atan2(v.y, v.x);
	return (p < 0) ? (p + g_2PI) : p;
}

void CoordinateSystem(float3 v1, out float3 v2, out float3 v3)
{
	float sign = (v1.z >= 0.0f) * 2.0f - 1.0f; // copysign(1.0f, v1.z); // No HLSL support yet
	float a	   = -1.0f / (sign + v1.z);
	float b	   = v1.x * v1.y * a;
	v2		   = float3(1.0f + sign * v1.x * v1.x * a, sign * b, -sign * v1.x);
	v3		   = float3(b, sign + v1.y * v1.y * a, -v1.y);
}

struct Frame
{
	float3 x;
	float3 y;
	float3 z;

	float3 ToWorld(float3 v) { return x * v.x + y * v.y + z * v.z; }

	float3 ToLocal(float3 v) { return float3(dot(v, x), dot(v, y), dot(v, z)); }
};

Frame InitFrameFromZ(float3 z)
{
	Frame frame;
	frame.z = z;
	CoordinateSystem(frame.z, frame.x, frame.y);
	return frame;
}

Frame InitFrameFromXY(float3 x, float3 y)
{
	Frame frame;
	frame.x = x;
	frame.y = y;
	frame.z = cross(x, y);
	return frame;
}

// float SignNotZero(float v)
//{
//	return (v < 0.0f) ? -1.0f : 1.0f;
//}

// struct OctahedralVector
//{
//	uint16_t x, y;

//	float3 Decode()
//	{
//		float3 v;
//		v.x = -1.0f + 2.0f * (x / 65535.0f);
//		v.y = -1.0f + 2.0f * (y / 65535.0f);
//		v.z = 1.0f - (abs(v.x) + abs(v.y));
//		// Reparameterize directions in the $z<0$ portion of the octahedron
//		if (v.z < 0.0f)
//		{
//			float xo = v.x;
//			v.x		 = (1.0f - abs(v.y)) * SignNotZero(xo);
//			v.y		 = (1.0f - abs(xo)) * SignNotZero(v.y);
//		}

//		return normalize(v);
//	}
//};

// uint16_t Encode(float f)
//{
//	return (uint16_t)round(clamp((f + 1.0f) / 2.0f, 0.0f, 1.0f) * 65535.0f);
//}

// OctahedralVector InitOctahedralVector(float3 v)
//{
//	OctahedralVector ov;

//	v /= abs(v.x) + abs(v.y) + abs(v.z);
//	if (v.z >= 0.0f)
//	{
//		ov.x = Encode(v.x);
//		ov.y = Encode(v.y);
//	}
//	else
//	{
//		// Encode octahedral vector with $z < 0$
//		ov.x = Encode((1.0f - abs(v.y)) * SignNotZero(v.x));
//		ov.y = Encode((1.0f - abs(v.x)) * SignNotZero(v.y));
//	}

//	return ov;
//}

float2 octWrap(float2 v)
{
	return float2((1.0f - abs(v.y)) * (v.x >= 0.0f ? 1.0f : -1.0f), (1.0f - abs(v.x)) * (v.y >= 0.0f ? 1.0f : -1.0f));
}

struct OctahedralVector
{
	float x, y;

	float3 Decode()
	{
		float3 v   = float3(x, y, 1.0f - abs(x) - abs(y));
		float2 tmp = (v.z < 0.0f) ? octWrap(float2(v.x, v.y)) : float2(v.x, v.y);
		v.x		   = tmp.x;
		v.y		   = tmp.y;
		return normalize(v);
	}
};

OctahedralVector InitOctahedralVector(float3 v)
{
	OctahedralVector ov;

	float2 p = float2(v.x, v.y) * (1.0f / (abs(v.x) + abs(v.y) + abs(v.z)));
	p		 = (v.z < 0.0f) ? octWrap(p) : p;

	ov.x = p.x;
	ov.y = p.y;

	return ov;
}

// Ray tracing gems chapter 06: A fast and robust method for avoiding self-intersection
// Offsets the ray origin from current position p, along normal n (which must be geometric normal)
// so that no self-intersection can occur.
float3 OffsetRay(float3 p, float3 ng)
{
	static const float origin	   = 1.0f / 32.0f;
	static const float float_scale = 1.0f / 65536.0f;
	static const float int_scale   = 256.0f;

	int3 of_i = int3(int_scale * ng.x, int_scale * ng.y, int_scale * ng.z);

	float3 p_i = float3(
		asfloat(asint(p.x) + ((p.x < 0) ? -of_i.x : of_i.x)),
		asfloat(asint(p.y) + ((p.y < 0) ? -of_i.y : of_i.y)),
		asfloat(asint(p.z) + ((p.z < 0) ? -of_i.z : of_i.z)));

	return float3(
		abs(p.x) < origin ? p.x + float_scale * ng.x : p_i.x,
		abs(p.y) < origin ? p.y + float_scale * ng.y : p_i.y,
		abs(p.z) < origin ? p.z + float_scale * ng.z : p_i.z);
}

#endif // MATH_HLSLI
