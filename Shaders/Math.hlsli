#ifndef MATH_HLSLI
#define MATH_HLSLI

static const float g_EPSILON = 1e-4f;

static const float g_PI = 3.141592654f;
static const float g_2PI = 6.283185307f;
static const float g_1DIVPI = 0.318309886f;
static const float g_1DIV2PI = 0.159154943f;
static const float g_1DIV4PI = 0.079577471f;
static const float g_PIDIV2 = 1.570796327f;
static const float g_PIDIV4 = 0.785398163f;

static const float FLT_MAX = asfloat(0x7F7FFFFF);

float3 Faceforward(float3 n, float3 v)
{
	return (dot(n, v) < 0.f) ? -n : n;
}

inline float Sqr(float x)
{
	return x * x;
}

inline float SphericalTheta(float3 v)
{
	return acos(clamp(v.z, -1, 1));
}

inline float SphericalPhi(float3 v)
{
	float p = atan2(v.y, v.x);
	return (p < 0) ? (p + g_2PI) : p;
}

void CoordinateSystem(float3 v1, out float3 v2, out float3 v3)
{
	float sign = (v1.z >= 0.0) * 2.0 - 1.0; //copysign(1.0f, v1.z); // No HLSL support yet
	float a = -1.0f / (sign + v1.z);
	float b = v1.x * v1.y * a;
	v2 = float3(1.0f + sign * v1.x * v1.x * a, sign * b, -sign * v1.x);
	v3 = float3(b, sign + v1.y * v1.y * a, -v1.y);
}

struct Frame
{
	float3 ToWorld(float3 v)
	{
		return x * v.x + y * v.y + z * v.z;
	}

	float3 ToLocal(float3 v)
	{
		return float3(dot(v, x), dot(v, y), dot(v, z));
	}
	
	float3 x;
	float3 y;
	float3 z;
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

#endif // MATH_HLSLI