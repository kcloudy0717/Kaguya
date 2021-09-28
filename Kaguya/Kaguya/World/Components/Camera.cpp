#include "../Components.h"

using namespace DirectX;

XMVECTOR Camera::GetUVector() const
{
	return pTransform->Right() * tanf(XMConvertToRadians(FoVY) * 0.5f) * AspectRatio;
}

XMVECTOR Camera::GetVVector() const
{
	return pTransform->Up() * tanf(XMConvertToRadians(FoVY) * 0.5f);
}

XMVECTOR Camera::GetWVector() const
{
	return pTransform->Forward();
}

void Camera::SetLookAt(DirectX::XMVECTOR EyePosition, DirectX::XMVECTOR FocusPosition, DirectX::XMVECTOR UpDirection)
{
	XMMATRIX View = XMMatrixLookAtLH(EyePosition, FocusPosition, UpDirection);
	pTransform->SetTransform(XMMatrixInverse(nullptr, View));
}

void Camera::Update()
{
	PrevViewProjection = ViewProjection;

	XMMATRIX WorldMatrix		  = pTransform->Matrix();
	XMMATRIX ViewMatrix			  = XMMatrixInverse(nullptr, WorldMatrix);
	XMMATRIX ProjectionMatrix	  = XMMatrixPerspectiveFovLH(XMConvertToRadians(FoVY), AspectRatio, NearZ, FarZ);
	XMMATRIX ViewProjectionMatrix = XMMatrixMultiply(ViewMatrix, ProjectionMatrix);

	XMStoreFloat4x4(&View, XMMatrixTranspose(ViewMatrix));
	XMStoreFloat4x4(&Projection, XMMatrixTranspose(ProjectionMatrix));
	XMStoreFloat4x4(&ViewProjection, XMMatrixTranspose(ViewProjectionMatrix));

	XMStoreFloat4x4(&InverseView, XMMatrixTranspose(WorldMatrix));
	XMStoreFloat4x4(&InverseProjection, XMMatrixTranspose(XMMatrixInverse(nullptr, ProjectionMatrix)));
	XMStoreFloat4x4(&InverseViewProjection, XMMatrixTranspose(XMMatrixInverse(nullptr, ViewProjectionMatrix)));
}

void Camera::Translate(float DeltaX, float DeltaY, float DeltaZ)
{
	Dirty = true;
	pTransform->Translate(DeltaX, DeltaY, DeltaZ);
}

void Camera::Rotate(float AngleX, float AngleY, float AngleZ)
{
	Dirty = true;
	pTransform->Rotate(AngleX, AngleY, AngleZ);
}
