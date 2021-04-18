#include "pch.h"
#include "Math.h"

#include <cstdlib>
#include <random>

using namespace DirectX;

static std::random_device rd;
static std::mt19937 generator(rd());

int Rand(int a, int b)
{
	std::uniform_int_distribution uid(a, b);
	return uid(generator);
}

float RandF()
{
	std::uniform_real_distribution urd;
	return urd(generator);
}

float RandF(float min, float max)
{
	std::uniform_real_distribution urd(min, max);
	return urd(generator);
}

DirectX::XMVECTOR CartesianToSpherical(float x, float y, float z)
{
	float radius = sqrtf(x * x + y * y + z * z);
	float theta = acosf(z / radius);
	float phi = atanf(y / x);
	return XMVectorSet(radius, theta, phi, 1.0f);
}

DirectX::XMVECTOR SphericalToCartesian(float radius, float theta, float phi)
{
	float x = radius * sinf(theta) * cosf(phi);
	float y = radius * sinf(theta) * sinf(phi);
	float z = radius * cosf(theta);
	return XMVectorSet(x, y, z, 1.0f);
}

DirectX::XMVECTOR RandomVector()
{
	return XMVectorSet(RandF(), RandF(), RandF(), 0.0f);
}

DirectX::XMVECTOR RandomVector(float Min, float Max)
{
	return XMVectorSet(RandF(Min, Max), RandF(Min, Max), RandF(Min, Max), 0.0f);
}

DirectX::XMVECTOR RandomUnitVector()
{
	float a = RandF(0.0f, XM_2PI);
	float z = RandF(-1.0f, 1.0f);
	float r = sqrtf(1.0f - z * z);
	return XMVectorSet(r * cosf(a), r * sin(a), z, 0.0f);
}

DirectX::XMVECTOR RandomInUnitDisk()
{
	while (true)
	{
		XMVECTOR p = XMVectorSet(RandF(-1.0f, 1.0f), RandF(-1.0f, 1.0f), 0.0f, 0.0f);
		if (XMVectorGetX(XMVector3LengthSq(p)) >= 1.0f) continue;
		return p;
	}
}

DirectX::XMVECTOR RandomInUnitSphere()
{
	while (true)
	{
		XMVECTOR p = RandomVector(-1.0f, 1.0f);
		if (XMVectorGetX(XMVector3LengthSq(p)) >= 1.0f) continue;
		return p;
	}
}

DirectX::XMVECTOR RandomInHemisphere(DirectX::FXMVECTOR Normal)
{
	XMVECTOR p = RandomInUnitSphere();
	if (XMVectorGetX(XMVector3Dot(p, Normal)) > 0.0f) // In the same hemisphere as the normal
		return p;
	else
		return -p;
}

DirectX::XMFLOAT4X4 Identity()
{
	return XMFLOAT4X4(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);
}

DirectX::XMVECTOR QuaternionToEulerAngles(DirectX::CXMVECTOR Q)
{
	XMVECTOR EulerAngles;

	float Q_x = XMVectorGetX(Q);
	float Q_y = XMVectorGetY(Q);
	float Q_z = XMVectorGetZ(Q);
	float Q_w = XMVectorGetW(Q);

	// roll (x-axis rotation)
	float sinr_cosp = 2.0f * (Q_w * Q_x + Q_y * Q_z);
	float cosr_cosp = 1.0f - 2.0f * (Q_x * Q_x + Q_y * Q_y);
	float roll = std::atan2(sinr_cosp, cosr_cosp);

	// pitch (y-axis rotation)
	float pitch;
	float sinp = 2.0f * (Q_w * Q_y - Q_z * Q_x);
	if (std::abs(sinp) >= 1.0f)
		pitch = std::copysign(XM_PIDIV2, sinp); // use 90 degrees if out of range
	else
		pitch = std::asin(sinp);

	// yaw (z-axis rotation)
	float siny_cosp = 2.0f * (Q_w * Q_z + Q_x * Q_y);
	float cosy_cosp = 1.0f - 2.0f * (Q_y * Q_y + Q_z * Q_z);
	float yaw = std::atan2(siny_cosp, cosy_cosp);

	return DirectX::XMVectorSet(roll, pitch, yaw, 0.0f);
}