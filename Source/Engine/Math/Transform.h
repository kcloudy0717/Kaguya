#pragma once
#include <DirectXMath.h>

namespace Math
{
	struct Transform
	{
		Transform();

		void SetTransform(DirectX::FXMMATRIX M);

		void Translate(float DeltaX, float DeltaY, float DeltaZ);

		void SetScale(float ScaleX, float ScaleY, float ScaleZ);

		// Radians
		void SetOrientation(float AngleX, float AngleY, float AngleZ);

		void Rotate(float AngleX, float AngleY, float AngleZ);

		[[nodiscard]] DirectX::XMMATRIX Matrix() const;

		[[nodiscard]] DirectX::XMVECTOR Right() const;

		[[nodiscard]] DirectX::XMVECTOR Up() const;

		[[nodiscard]] DirectX::XMVECTOR Forward() const;

		[[nodiscard]] bool operator==(const Transform& Transform) const;
		[[nodiscard]] bool operator!=(const Transform& Transform) const;

		// TODO: Eventually use our own math types
		DirectX::XMFLOAT3 Position;
		DirectX::XMFLOAT3 Scale;
		DirectX::XMFLOAT4 Orientation;
	};

	inline Transform::Transform()
	{
		using namespace DirectX;

		XMStoreFloat3(&Position, XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f));
		XMStoreFloat4(&Orientation, XMQuaternionIdentity());
		XMStoreFloat3(&Scale, XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f));
	}

	inline void Transform::SetTransform(DirectX::FXMMATRIX M)
	{
		using namespace DirectX;

		XMVECTOR vTranslation, vOrientation, vScale;
		XMMatrixDecompose(&vScale, &vOrientation, &vTranslation, M);

		XMStoreFloat3(&Position, vTranslation);
		XMStoreFloat4(&Orientation, vOrientation);
		XMStoreFloat3(&Scale, vScale);
	}

	inline void Transform::Translate(float DeltaX, float DeltaY, float DeltaZ)
	{
		using namespace DirectX;

		XMVECTOR vPosition = XMLoadFloat3(&Position);
		XMVECTOR vVelocity = XMVector3Rotate(XMVectorSet(DeltaX, DeltaY, DeltaZ, 0.0f), XMLoadFloat4(&Orientation));
		XMStoreFloat3(&Position, XMVectorAdd(vPosition, vVelocity));
	}

	inline void Transform::SetScale(float ScaleX, float ScaleY, float ScaleZ)
	{
		Scale.x = ScaleX;
		Scale.y = ScaleY;
		Scale.z = ScaleZ;
	}

	inline void Transform::SetOrientation(float AngleX, float AngleY, float AngleZ)
	{
		using namespace DirectX;

		XMVECTOR vEulerRotation = XMQuaternionRotationRollPitchYaw(AngleX, AngleY, AngleZ);
		XMStoreFloat4(&Orientation, vEulerRotation);
	}

	inline void Transform::Rotate(float AngleX, float AngleY, float AngleZ)
	{
		using namespace DirectX;

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

	inline DirectX::XMMATRIX Transform::Matrix() const
	{
		using namespace DirectX;

		XMVECTOR vScale		  = XMLoadFloat3(&Scale);
		XMVECTOR vOrientation = XMLoadFloat4(&Orientation);
		XMVECTOR vPosition	  = XMLoadFloat3(&Position);

		XMMATRIX mS = XMMatrixScalingFromVector(vScale);
		XMMATRIX mR = XMMatrixRotationQuaternion(vOrientation);
		XMMATRIX mT = XMMatrixTranslationFromVector(vPosition);

		// mS * mR * mT
		return mS * mR * mT;
	}

	inline DirectX::XMVECTOR Transform::Right() const
	{
		using namespace DirectX;

		return XMVector3Rotate(g_XMIdentityR0, XMLoadFloat4(&Orientation));
	}

	inline DirectX::XMVECTOR Transform::Up() const
	{
		using namespace DirectX;

		return XMVector3Rotate(g_XMIdentityR1, XMLoadFloat4(&Orientation));
	}

	inline DirectX::XMVECTOR Transform::Forward() const
	{
		using namespace DirectX;

		return XMVector3Rotate(g_XMIdentityR2, XMLoadFloat4(&Orientation));
	}

	inline bool Transform::operator==(const Transform& Transform) const
	{
		using namespace DirectX;

		return XMVector3Equal(XMLoadFloat3(&Position), XMLoadFloat3(&Transform.Position)) &&
			   XMVector3Equal(XMLoadFloat3(&Scale), XMLoadFloat3(&Transform.Scale)) &&
			   XMVector4Equal(XMLoadFloat4(&Orientation), XMLoadFloat4(&Transform.Orientation));
	}

	inline bool Transform::operator!=(const Transform& Transform) const
	{
		return !(*this == Transform);
	}
} // namespace Math
