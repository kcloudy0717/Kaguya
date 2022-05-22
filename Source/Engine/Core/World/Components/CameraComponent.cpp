#include "../Components.h"

using namespace DirectX;

XMVECTOR CameraComponent::GetUVector() const
{
	return Transform.Right() * tanf(XMConvertToRadians(FoVY) * 0.5f) * AspectRatio;
}

XMVECTOR CameraComponent::GetVVector() const
{
	return Transform.Up() * tanf(XMConvertToRadians(FoVY) * 0.5f);
}

XMVECTOR CameraComponent::GetWVector() const
{
	return Transform.Forward();
}

void CameraComponent::SetLookAt(DirectX::XMVECTOR EyePosition, DirectX::XMVECTOR FocusPosition, DirectX::XMVECTOR UpDirection)
{
	XMMATRIX View = XMMatrixLookAtLH(EyePosition, FocusPosition, UpDirection);
	Transform.SetTransform(XMMatrixInverse(nullptr, View));
}

void CameraComponent::Update()
{
	PrevViewProjection = ViewProjection;

	XMMATRIX WorldMatrix		  = Transform.Matrix();
	XMMATRIX ViewMatrix			  = XMMatrixInverse(nullptr, WorldMatrix);
	XMMATRIX ProjectionMatrix	  = XMMatrixPerspectiveFovLH(XMConvertToRadians(FoVY), AspectRatio, NearZ, FarZ);
	XMMATRIX ViewProjectionMatrix = XMMatrixMultiply(ViewMatrix, ProjectionMatrix);

	XMStoreFloat4x4(&View, ViewMatrix);
	XMStoreFloat4x4(&Projection, ProjectionMatrix);
	XMStoreFloat4x4(&ViewProjection, ViewProjectionMatrix);

	XMStoreFloat4x4(&InverseView, WorldMatrix);
	XMStoreFloat4x4(&InverseProjection, XMMatrixInverse(nullptr, ProjectionMatrix));
	XMStoreFloat4x4(&InverseViewProjection, XMMatrixInverse(nullptr, ViewProjectionMatrix));
}

void CameraComponent::Translate(float DeltaX, float DeltaY, float DeltaZ)
{
	Dirty = true;
	Transform.Translate(DeltaX, DeltaY, DeltaZ);
}

void CameraComponent::Rotate(float AngleX, float AngleY, float AngleZ)
{
	Dirty = true;
	Transform.Rotate(AngleX, AngleY, AngleZ);
}
