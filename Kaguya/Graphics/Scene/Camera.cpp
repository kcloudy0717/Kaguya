#include "pch.h"
#include "Camera.h"

using namespace DirectX;

Camera::Camera()
{
	FoVY		= DirectX::XM_PIDIV4;
	AspectRatio = 1.0f;
	NearZ		= 0.1f;
	FarZ		= 500.0f;

	FocalLength		  = 10.0f;
	RelativeAperture  = 0.0f;
	ShutterTime		  = 1.0f / 125.0f;
	SensorSensitivity = 200.0f;
}

DirectX::XMVECTOR Camera::GetUVector() const
{
	return Transform.Right() * FocalLength * tanf(FoVY * 0.5f) * AspectRatio;
}

DirectX::XMVECTOR Camera::GetVVector() const
{
	return Transform.Up() * FocalLength * tanf(FoVY * 0.5f);
}

DirectX::XMVECTOR Camera::GetWVector() const
{
	return Transform.Forward() * FocalLength;
}

void Camera::SetLookAt(DirectX::XMVECTOR EyePosition, DirectX::XMVECTOR FocusPosition, DirectX::XMVECTOR UpDirection)
{
	XMMATRIX view = XMMatrixLookAtLH(EyePosition, FocusPosition, UpDirection);
	Transform.SetTransform(XMMatrixInverse(nullptr, view));
}

void Camera::Rotate(float AngleX, float AngleY)
{
	Transform.Rotate(AngleX, AngleY, 0.0f);
}

void Camera::Update()
{
	PrevViewProjectionMatrix = ViewProjectionMatrix;

	ViewMatrix			 = XMMatrixInverse(nullptr, Transform.Matrix());
	ProjectionMatrix	 = XMMatrixPerspectiveFovLH(FoVY, AspectRatio, NearZ, FarZ);
	ViewProjectionMatrix = XMMatrixMultiply(ViewMatrix, ProjectionMatrix);

	InverseViewMatrix			= XMMatrixInverse(nullptr, ViewMatrix);
	InverseProjectionMatrix		= XMMatrixInverse(nullptr, ProjectionMatrix);
	InverseViewProjectionMatrix = XMMatrixInverse(nullptr, ViewProjectionMatrix);
}
