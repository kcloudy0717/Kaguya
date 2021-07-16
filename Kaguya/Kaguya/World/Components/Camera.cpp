#include "../Components.h"

using namespace DirectX;

DirectX::XMVECTOR Camera::GetUVector() const
{
	return pTransform->Right() * tanf(XMConvertToRadians(FoVY) * 0.5f) * AspectRatio;
}

DirectX::XMVECTOR Camera::GetVVector() const
{
	return pTransform->Up() * tanf(XMConvertToRadians(FoVY) * 0.5f);
}

DirectX::XMVECTOR Camera::GetWVector() const
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
	PrevViewProjectionMatrix = ViewProjectionMatrix;

	ViewMatrix			 = XMMatrixInverse(nullptr, pTransform->Matrix());
	ProjectionMatrix	 = XMMatrixPerspectiveFovLH(XMConvertToRadians(FoVY), AspectRatio, NearZ, FarZ);
	ViewProjectionMatrix = XMMatrixMultiply(ViewMatrix, ProjectionMatrix);

	InverseViewMatrix			= XMMatrixInverse(nullptr, ViewMatrix);
	InverseProjectionMatrix		= XMMatrixInverse(nullptr, ProjectionMatrix);
	InverseViewProjectionMatrix = XMMatrixInverse(nullptr, ViewProjectionMatrix);
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
