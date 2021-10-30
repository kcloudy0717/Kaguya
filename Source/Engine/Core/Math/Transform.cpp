#include "Math.h"

using namespace DirectX;

Transform::Transform()
{
	XMStoreFloat3(&Position, XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f));
	XMStoreFloat4(&Orientation, XMQuaternionIdentity());
	XMStoreFloat3(&Scale, XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f));
}

void Transform::SetTransform(DirectX::FXMMATRIX M)
{
	XMVECTOR vTranslation, vOrientation, vScale;
	XMMatrixDecompose(&vScale, &vOrientation, &vTranslation, M);

	XMStoreFloat3(&Position, vTranslation);
	XMStoreFloat4(&Orientation, vOrientation);
	XMStoreFloat3(&Scale, vScale);
}

void Transform::Translate(float DeltaX, float DeltaY, float DeltaZ)
{
	XMVECTOR vPosition = XMLoadFloat3(&Position);
	XMVECTOR vVelocity = XMVector3Rotate(XMVectorSet(DeltaX, DeltaY, DeltaZ, 0.0f), XMLoadFloat4(&Orientation));
	XMStoreFloat3(&Position, XMVectorAdd(vPosition, vVelocity));
}

void Transform::SetScale(float ScaleX, float ScaleY, float ScaleZ)
{
	Scale.x = ScaleX;
	Scale.y = ScaleY;
	Scale.z = ScaleZ;
}

void Transform::SetOrientation(float AngleX, float AngleY, float AngleZ)
{
	XMVECTOR vEulerRotation = XMQuaternionRotationRollPitchYaw(AngleX, AngleY, AngleZ);
	XMStoreFloat4(&Orientation, vEulerRotation);
}

void Transform::Rotate(float AngleX, float AngleY, float AngleZ)
{
	AngleX = XMConvertToRadians(AngleX);
	AngleY = XMConvertToRadians(AngleY);

	XMVECTOR vOrientation = XMLoadFloat4(&Orientation);
	XMVECTOR vPitch		  = XMQuaternionNormalize(XMQuaternionRotationAxis(Right(), AngleX));
	XMVECTOR vYaw		  = XMQuaternionNormalize(XMQuaternionRotationAxis(g_XMIdentityR1, AngleY));
	XMVECTOR vRoll		  = XMQuaternionNormalize(XMQuaternionRotationAxis(Forward(), AngleZ));

	XMStoreFloat4(&Orientation, XMQuaternionMultiply(vOrientation, vPitch));
	vOrientation = XMLoadFloat4(&Orientation);
	XMStoreFloat4(&Orientation, XMQuaternionMultiply(vOrientation, vYaw));
	vOrientation = DirectX::XMLoadFloat4(&Orientation);
	XMStoreFloat4(&Orientation, XMQuaternionMultiply(vOrientation, vRoll));
}

DirectX::XMMATRIX Transform::Matrix() const
{
	XMVECTOR vScale		  = XMLoadFloat3(&Scale);
	XMVECTOR vOrientation = XMLoadFloat4(&Orientation);
	XMVECTOR vPosition	  = XMLoadFloat3(&Position);

	XMMATRIX mS = XMMatrixScalingFromVector(vScale);
	XMMATRIX mR = XMMatrixRotationQuaternion(vOrientation);
	XMMATRIX mT = XMMatrixTranslationFromVector(vPosition);

	// mS * mR * mT
	return mS * mR * mT;
}

DirectX::XMVECTOR Transform::Right() const
{
	return XMVector3Rotate(g_XMIdentityR0, XMLoadFloat4(&Orientation));
}

DirectX::XMVECTOR Transform::Up() const
{
	return XMVector3Rotate(g_XMIdentityR1, XMLoadFloat4(&Orientation));
}

DirectX::XMVECTOR Transform::Forward() const
{
	return XMVector3Rotate(g_XMIdentityR2, XMLoadFloat4(&Orientation));
}

bool Transform::operator==(const Transform& Transform) const
{
	return XMVector3Equal(XMLoadFloat3(&Position), XMLoadFloat3(&Transform.Position)) &&
		   XMVector3Equal(XMLoadFloat3(&Scale), XMLoadFloat3(&Transform.Scale)) &&
		   XMVector4Equal(XMLoadFloat4(&Orientation), XMLoadFloat4(&Transform.Orientation));
}

bool Transform::operator!=(const Transform& Transform) const
{
	return !(*this == Transform);
}
