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

	XMStoreFloat4x4(&View, ViewMatrix);
	XMStoreFloat4x4(&Projection, ProjectionMatrix);
	XMStoreFloat4x4(&ViewProjection, ViewProjectionMatrix);

	XMStoreFloat4x4(&InverseView, WorldMatrix);
	XMStoreFloat4x4(&InverseProjection, XMMatrixInverse(nullptr, ProjectionMatrix));
	XMStoreFloat4x4(&InverseViewProjection, XMMatrixInverse(nullptr, ViewProjectionMatrix));
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

Frustum Camera::CreateFrustum() const
{
	float NearHeight = 2.0f * tanf(XMConvertToRadians(FoVY) * 0.5f) * NearZ;
	float FarHeight	 = 2.0f * tanf(XMConvertToRadians(FoVY) * 0.5f) * FarZ;
	float NearWidth	 = NearHeight * AspectRatio;
	float FarWidth	 = FarHeight * AspectRatio;

	Vector3f XAxis(InverseView(0, 0), InverseView(0, 1), InverseView(0, 2));
	Vector3f YAxis(InverseView(1, 0), InverseView(1, 1), InverseView(1, 2));
	Vector3f ZAxis(InverseView(2, 0), InverseView(2, 1), InverseView(2, 2));
	Vector3f Translation(InverseView(3, 0), InverseView(3, 1), InverseView(3, 2));

	Vector3f NearCenter = Translation + (ZAxis * NearZ);
	Vector3f FarCenter	= Translation + (ZAxis * FarZ);

	Vector3f Ntl = NearCenter + (YAxis * (NearHeight * 0.5f)) - (XAxis * (NearWidth * 0.5f));
	Vector3f Ntr = NearCenter + (YAxis * (NearHeight * 0.5f)) + (XAxis * (NearWidth * 0.5f));
	Vector3f Nbl = NearCenter - (YAxis * (NearHeight * 0.5f)) - (XAxis * (NearWidth * 0.5f));
	Vector3f Nbr = NearCenter - (YAxis * (NearHeight * 0.5f)) + (XAxis * (NearWidth * 0.5f));

	Vector3f Ftl = FarCenter + (YAxis * (FarHeight * 0.5f)) - (XAxis * (FarWidth * 0.5f));
	Vector3f Ftr = FarCenter + (YAxis * (FarHeight * 0.5f)) + (XAxis * (FarWidth * 0.5f));
	Vector3f Fbl = FarCenter - (YAxis * (FarHeight * 0.5f)) - (XAxis * (FarWidth * 0.5f));
	Vector3f Fbr = FarCenter - (YAxis * (FarHeight * 0.5f)) + (XAxis * (FarWidth * 0.5f));

	Frustum Frustum;
	Frustum.Left   = Plane(Ntl, Ftl, Nbl);
	Frustum.Right  = Plane(Ftr, Ntr, Fbr);
	Frustum.Bottom = Plane(Nbl, Fbl, Fbr);
	Frustum.Top	   = Plane(Ntl, Ntr, Ftl);
	Frustum.Near   = Plane(Ntr, Ntl, Nbl);
	Frustum.Far	   = Plane(Ftl, Ftr, Fbr);
	return Frustum;
}
