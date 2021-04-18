#pragma once
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <cmath>
#include <limits>

// NOTE: [] = inclusive, () = exclusive

// Returns random int in [a, b].
int Rand(int min, int max);

// Returns random float in [0, 1).
float RandF();

// Returns random float in [a, b).
float RandF(float min, float max);

DirectX::XMVECTOR CartesianToSpherical(float x, float y, float z);
DirectX::XMVECTOR SphericalToCartesian(float radius, float theta, float phi);

DirectX::XMVECTOR RandomVector();
DirectX::XMVECTOR RandomVector(float Min, float Max);
DirectX::XMVECTOR RandomUnitVector();
DirectX::XMVECTOR RandomInUnitDisk();
DirectX::XMVECTOR RandomInUnitSphere();
DirectX::XMVECTOR RandomInHemisphere(DirectX::FXMVECTOR Normal);

DirectX::XMFLOAT4X4 Identity();
DirectX::XMVECTOR QuaternionToEulerAngles(DirectX::CXMVECTOR Q);

template<typename T>
constexpr inline T AlignUp(T Size, T Alignment)
{
	return (T)(((size_t)Size + (size_t)Alignment - 1) & ~((size_t)Alignment - 1));
}

template<typename T>
constexpr inline T AlignDown(T Size, T Alignment)
{
	return (T)((size_t)Size & ~((size_t)Alignment - 1));
}

template <typename T>
constexpr inline T RoundUpAndDivide(T Value, size_t Alignment)
{
	return (T)((Value + Alignment - 1) / Alignment);
}

// Returns radians
constexpr inline float operator"" _Deg(long double Degrees)
{
	return DirectX::XMConvertToRadians(static_cast<float>(Degrees));
}

// Returns degrees
constexpr inline float operator"" _Rad(long double Radians)
{
	return DirectX::XMConvertToDegrees(static_cast<float>(Radians));
}

constexpr inline std::size_t operator"" _KiB(std::size_t X)
{
	return X * 1024;
}

constexpr inline std::size_t operator"" _MiB(std::size_t X)
{
	return X * 1024 * 1024;
}

constexpr inline std::size_t operator"" _GiB(std::size_t X)
{
	return X * 1024 * 1024 * 1024;
}

inline std::size_t ToKiB(std::size_t Byte)
{
	return Byte / 1024;
}

inline std::size_t ToMiB(std::size_t Byte)
{
	return Byte / 1024 / 1024;
}

inline std::size_t ToGiB(std::size_t Byte)
{
	return Byte / 1024 / 1024 / 1024;
}

template <typename T>
struct Vector2
{
	Vector2()
	{
		x = y = static_cast<T>(0);
	}

	Vector2(T v)
		: x(v), y(v)
	{

	}

	Vector2(T x, T y)
		: x(x), y(y)
	{

	}

	auto operator<=>(const Vector2&) const = default;

	T operator[](int i) const
	{
		return e[i];
	}

	T& operator[](int i)
	{
		return e[i];
	}

	Vector2 operator-() const
	{
		return Vector2(-x, -y);
	}

	Vector2 operator+(const Vector2& v) const
	{
		return Vector2(x + v.x, y + v.y);
	}

	Vector2& operator+=(const Vector2& v)
	{
		x += v.x; y += v.y;
		return *this;
	}

	Vector2 operator-(const Vector2& v) const
	{
		return Vector2(x - v.x, y - v.y);
	}

	Vector2& operator-=(const Vector2& v)
	{
		x -= v.x; y -= v.y;
		return *this;
	}

	Vector2 operator*(T s) const
	{
		return Vector2(x * s, y * s);
	}

	Vector2& operator*=(T s)
	{
		x *= s; y *= s;
		return *this;
	}

	Vector2 operator/(T s) const
	{
		float inv = static_cast<T>(1) / s;
		return Vector2<T>(x * inv, y * inv);
	}

	Vector2& operator/=(T s)
	{
		float inv = static_cast<T>(1) / s;
		x *= inv; y *= inv;
		return *this;
	}

	T LengthSquared() const
	{
		return x * x + y * y;
	}

	T Length() const
	{
		return std::sqrt(LengthSquared());
	}

	bool HasNans() const
	{
		return std::isnan(x) || std::isnan(y);
	}

	DirectX::XMVECTOR ToXMVECTOR() const
	{
		return DirectX::XMVectorSet(x, y, 0.0f, 0.0f);
	}

	void operator=(DirectX::FXMVECTOR v)
	{
		XMStoreFloat2(reinterpret_cast<DirectX::XMFLOAT2*>(this), v);
	}

	union
	{
		T e[2];
		struct
		{
			T x, y;
		};
	};
};

template <typename T>
inline Vector2<T> operator*(T s, const Vector2<T>& v)
{
	return v * s;
}

template <typename T>
Vector2<T> Abs(const Vector2<T>& v)
{
	return Vector2<T>(std::abs(v.x), std::abs(v.y));
}

template <typename T>
T Dot(const Vector2<T>& v1, const Vector2<T>& v2)
{
	return v1.x * v2.x + v1.y * v2.y;
}

template <typename T>
T AbsDot(const Vector2<T>& v1, const Vector2<T>& v2)
{
	return std::abs(Dot(v1, v2));
}

template <typename T>
Vector2<T> Normalize(const Vector2<T>& v)
{
	return v / v.Length();
}

using Vector2i = Vector2<int>;
using Vector2f = Vector2<float>;

template <typename T>
struct Vector3
{
	Vector3()
	{
		x = y = z = static_cast<T>(0);
	}

	Vector3(T v)
		: x(v), y(v), z(v)
	{

	}

	Vector3(T x, T y, T z)
		: x(x), y(y), z(z)
	{

	}

	auto operator<=>(const Vector3&) const = default;

	T operator[](int i) const
	{
		return e[i];
	}

	T& operator[](int i)
	{
		return e[i];
	}

	Vector3 operator-() const
	{
		return Vector3(-x, -y, -z);
	}

	Vector3 operator+(const Vector3& v) const
	{
		return Vector3(x + v.x, y + v.y, z + v.z);
	}

	Vector3& operator+=(const Vector3& v)
	{
		x += v.x; y += v.y; z += v.z;
		return *this;
	}

	Vector3 operator-(const Vector3& v) const
	{
		return Vector3(x - v.x, y - v.y, z - v.z);
	}

	Vector3& operator-=(const Vector3& v)
	{
		x -= v.x; y -= v.y; z -= v.z;
		return *this;
	}

	Vector3 operator*(T s) const
	{
		return Vector3(x * s, y * s, z * s);
	}

	Vector3& operator*=(T s)
	{
		x *= s; y *= s; z *= s;
		return *this;
	}

	Vector3 operator/(T s) const
	{
		float inv = static_cast<T>(1) / s;
		return Vector3<T>(x * inv, y * inv, z * inv);
	}

	Vector3& operator/=(T s)
	{
		float inv = static_cast<T>(1) / s;
		x *= inv; y *= inv; z *= inv;
		return *this;
	}

	T LengthSquared() const
	{
		return x * x + y * y + z * z;
	}

	T Length() const
	{
		return std::sqrt(LengthSquared());
	}

	bool HasNans() const
	{
		return std::isnan(x) || std::isnan(y) || std::isnan(z);
	}

	DirectX::XMVECTOR ToXMVECTOR(bool Homogeneous = false) const
	{
		float w = Homogeneous ? 1.0f : 0.0f;
		return DirectX::XMVectorSet(x, y, z, w);
	}

	void operator=(DirectX::FXMVECTOR v)
	{
		XMStoreFloat3(reinterpret_cast<DirectX::XMFLOAT3*>(this), v);
	}

	union
	{
		T e[3];
		struct
		{
			T x, y, z;
		};
	};
};

template <typename T>
[[nodiscard]] inline Vector3<T> operator*(T s, const Vector3<T>& v)
{
	return v * s;
}

template <typename T>
[[nodiscard]] inline Vector3<T> Abs(const Vector3<T>& v)
{
	return Vector3<T>(std::abs(v.x), std::abs(v.y), std::abs(v.z));
}

template <typename T>
[[nodiscard]] inline T Dot(const Vector3<T>& v1, const Vector3<T>& v2)
{
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

template <typename T>
[[nodiscard]] inline T AbsDot(const Vector3<T>& v1, const Vector3<T>& v2)
{
	return std::abs(Dot(v1, v2));
}

template <typename T>
[[nodiscard]] inline Vector3<T> Cross(const Vector3<T>& v1, const Vector3<T>& v2)
{
	float v1x = v1.x, v1y = v1.y, v1z = v1.z;
	float v2x = v2.x, v2y = v2.y, v2z = v2.z;
	return Vector3<T>((v1y * v2z) - (v1z * v2y),
		(v1z * v2x) - (v1x * v2z),
		(v1x * v2y) - (v1y * v2x));
}

template <typename T>
[[nodiscard]] inline Vector3<T> Normalize(const Vector3<T>& v)
{
	return v / v.Length();
}

template <typename T>
[[nodiscard]] inline Vector3<T> Faceforward(const Vector3<T>& n, const Vector3<T>& v)
{
	return (Dot(n, v) < 0.0f) ? -n : n;
}

template <typename T>
[[nodiscard]] inline float DistanceSquared(const Vector3<T>& p1, const Vector3<T>& p2)
{
	return (p1 - p2).LengthSquared();
}

template <typename T>
inline void CoordinateSystem(const Vector3<T>& v1, Vector3<T>* v2, Vector3<T>* v3)
{
	if (std::abs(v1.x) > std::abs(v1.y))
	{
		*v2 = Vector3<T>(-v1.z, 0.0f, v1.x) / std::sqrt(v1.x * v1.x + v1.z * v1.z);
	}
	else
	{
		*v2 = Vector3<T>(0.0f, v1.z, -v1.y) / std::sqrt(v1.y * v1.y + v1.z * v1.z);
	}
	*v3 = Normalize(Cross(v1, *v2));
}

using Vector3i = Vector2<int>;
using Vector3f = Vector3<float>;