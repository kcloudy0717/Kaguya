#pragma once

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

#define PLANE_INTERSECTION_POSITIVE_HALFSPACE 0
#define PLANE_INTERSECTION_NEGATIVE_HALFSPACE 1
#define PLANE_INTERSECTION_INTERSECTING		  2

#define CONTAINMENT_DISJOINT				  0
#define CONTAINMENT_INTERSECTS				  1
#define CONTAINMENT_CONTAINS				  2

// ax + by + cz = d where d = dot(n, P)
struct Plane
{
	float3 Normal; // Plane normal. Points x on the plane satisfy dot(n, x) = d
	float  Offset; // d = dot(n, p) for a given point p on the plane
};

struct BoundingSphere
{
	float3 Center;
	float  Radius;

	bool Intersects(BoundingSphere Other)
	{
		// The distance between the sphere centers is computed and compared
		// against the sum of the sphere radii. To avoid an square root operation, the
		// squared distances are compared with squared sum radii instead.
		float3 d		 = Center - Other.Center;
		float  dist2	 = dot(d, d);
		float  radiusSum = Radius + Other.Radius;
		float  r2		 = radiusSum * radiusSum;
		return dist2 <= r2;
	}
};

struct BoundingBox
{
	float3 Center;
	float3 Extents;

	bool Intersects(BoundingBox Other)
	{
		float3 minA = Center - Extents;
		float3 maxA = Center + Extents;

		float3 minB = Other.Center - Other.Extents;
		float3 maxB = Other.Center + Other.Extents;

		// All axis needs to overlap for a intersection
		return maxA.x >= minB.x && minA.x <= maxB.x && // Overlap on x-axis?
			   maxA.y >= minB.y && minA.y <= maxB.y && // Overlap on y-axis?
			   maxA.z >= minB.z && minA.z <= maxB.z;   // Overlap on z-axis?
	}

	void Transform(float4x4 m, inout BoundingBox b)
	{
		float3 t = m[3].xyz;

		b.Center  = t;
		b.Extents = float3(0.0f, 0.0f, 0.0f);
		for (int i = 0; i < 3; ++i)
		{
			for (int j = 0; j < 3; ++j)
			{
				b.Center[i] += m[i][j] * Center[j];
				b.Extents[i] += abs(m[i][j]) * Extents[j];
			}
		}
	}
};

Plane ComputePlane(float3 a, float3 b, float3 c)
{
	Plane plane;
	plane.Normal = normalize(cross(b - a, c - a));
	plane.Offset = dot(plane.Normal, a);
	return plane;
}

#define FRUSTUM_PLANE_LEFT	 0
#define FRUSTUM_PLANE_RIGHT	 1
#define FRUSTUM_PLANE_BOTTOM 2
#define FRUSTUM_PLANE_TOP	 3
#define FRUSTUM_PLANE_NEAR	 4
#define FRUSTUM_PLANE_FAR	 5

struct Frustum
{
	Plane Left;	  // -x
	Plane Right;  // +x
	Plane Bottom; // -y
	Plane Top;	  // +y
	Plane Near;	  // -z
	Plane Far;	  // +z
};

int BoundingSphereToPlane(BoundingSphere s, Plane p)
{
	// Compute signed distance from plane to sphere center
	float sd = dot(s.Center, p.Normal) - p.Offset;
	if (sd > s.Radius)
	{
		return PLANE_INTERSECTION_POSITIVE_HALFSPACE;
	}
	if (sd < -s.Radius)
	{
		return PLANE_INTERSECTION_NEGATIVE_HALFSPACE;
	}
	return PLANE_INTERSECTION_INTERSECTING;
}

int BoundingBoxToPlane(BoundingBox b, Plane p)
{
	// Compute signed distance from plane to box center
	float sd = dot(b.Center, p.Normal) - p.Offset;

	// Compute the projection interval radius of b onto L(t) = b.Center + t * p.Normal
	// Projection radii r_i of the 8 bounding box vertices
	// r_i = dot((V_i - C), n)
	// r_i = dot((C +- e0*u0 +- e1*u1 +- e2*u2 - C), n)
	// Cancel C and distribute dot product
	// r_i = +-(dot(e0*u0, n)) +-(dot(e1*u1, n)) +-(dot(e2*u2, n))
	// We take the maximum position radius by taking the absolute value of the terms, we assume Extents to be positive
	// r = e0*|dot(u0, n)| + e1*|dot(u1, n)| + e2*|dot(u2, n)|
	// When the separating axis vector Normal is not a unit vector, we need to divide the radii by the length(Normal)
	// u0,u1,u2 are the local axes of the box, which is = [(1,0,0), (0,1,0), (0,0,1)] respectively for axis aligned bb
	float r = dot(b.Extents, abs(p.Normal));

	if (sd > r)
	{
		return PLANE_INTERSECTION_POSITIVE_HALFSPACE;
	}
	if (sd < -r)
	{
		return PLANE_INTERSECTION_NEGATIVE_HALFSPACE;
	}
	return PLANE_INTERSECTION_INTERSECTING;
}

int FrustumContainsBoundingSphere(Frustum f, BoundingSphere s)
{
	int	 p0			= BoundingSphereToPlane(s, f.Left);
	int	 p1			= BoundingSphereToPlane(s, f.Right);
	int	 p2			= BoundingSphereToPlane(s, f.Bottom);
	int	 p3			= BoundingSphereToPlane(s, f.Top);
	int	 p4			= BoundingSphereToPlane(s, f.Near);
	int	 p5			= BoundingSphereToPlane(s, f.Far);
	bool anyOutside = p0 == PLANE_INTERSECTION_NEGATIVE_HALFSPACE;
	anyOutside |= p1 == PLANE_INTERSECTION_NEGATIVE_HALFSPACE;
	anyOutside |= p2 == PLANE_INTERSECTION_NEGATIVE_HALFSPACE;
	anyOutside |= p3 == PLANE_INTERSECTION_NEGATIVE_HALFSPACE;
	anyOutside |= p4 == PLANE_INTERSECTION_NEGATIVE_HALFSPACE;
	anyOutside |= p5 == PLANE_INTERSECTION_NEGATIVE_HALFSPACE;
	bool allInside = p0 == PLANE_INTERSECTION_POSITIVE_HALFSPACE;
	allInside &= p1 == PLANE_INTERSECTION_POSITIVE_HALFSPACE;
	allInside &= p2 == PLANE_INTERSECTION_POSITIVE_HALFSPACE;
	allInside &= p3 == PLANE_INTERSECTION_POSITIVE_HALFSPACE;
	allInside &= p4 == PLANE_INTERSECTION_POSITIVE_HALFSPACE;
	allInside &= p5 == PLANE_INTERSECTION_POSITIVE_HALFSPACE;

	if (anyOutside)
	{
		return CONTAINMENT_DISJOINT;
	}

	if (allInside)
	{
		return CONTAINMENT_CONTAINS;
	}

	return CONTAINMENT_INTERSECTS;
}

int FrustumContainsBoundingBox(Frustum f, BoundingBox b)
{
	int	 p0			= BoundingBoxToPlane(b, f.Left);
	int	 p1			= BoundingBoxToPlane(b, f.Right);
	int	 p2			= BoundingBoxToPlane(b, f.Bottom);
	int	 p3			= BoundingBoxToPlane(b, f.Top);
	int	 p4			= BoundingBoxToPlane(b, f.Near);
	int	 p5			= BoundingBoxToPlane(b, f.Far);
	bool anyOutside = p0 == PLANE_INTERSECTION_NEGATIVE_HALFSPACE;
	anyOutside |= p1 == PLANE_INTERSECTION_NEGATIVE_HALFSPACE;
	anyOutside |= p2 == PLANE_INTERSECTION_NEGATIVE_HALFSPACE;
	anyOutside |= p3 == PLANE_INTERSECTION_NEGATIVE_HALFSPACE;
	anyOutside |= p4 == PLANE_INTERSECTION_NEGATIVE_HALFSPACE;
	anyOutside |= p5 == PLANE_INTERSECTION_NEGATIVE_HALFSPACE;
	bool allInside = p0 == PLANE_INTERSECTION_POSITIVE_HALFSPACE;
	allInside &= p1 == PLANE_INTERSECTION_POSITIVE_HALFSPACE;
	allInside &= p2 == PLANE_INTERSECTION_POSITIVE_HALFSPACE;
	allInside &= p3 == PLANE_INTERSECTION_POSITIVE_HALFSPACE;
	allInside &= p4 == PLANE_INTERSECTION_POSITIVE_HALFSPACE;
	allInside &= p5 == PLANE_INTERSECTION_POSITIVE_HALFSPACE;

	if (anyOutside)
	{
		return CONTAINMENT_DISJOINT;
	}

	if (allInside)
	{
		return CONTAINMENT_CONTAINS;
	}

	return CONTAINMENT_INTERSECTS;
}
