#pragma once
#include "Components.h"

struct Camera
{
	using FStop = float;
	using ISO = float;
	using EV = float;

	Camera();

	bool operator==(const Camera& Camera) const
	{
		return Transform == Camera.Transform &&
			FoVY == Camera.FoVY &&
			NearZ == Camera.NearZ &&
			FarZ == Camera.FarZ;
	}

	bool operator!=(const Camera& Camera) const
	{
		return !(*this == Camera);
	}

	float static CalculateFoVX(int w, int h, float FoVY)
	{
		return 2.0f * atan(tan(FoVY * 0.5f) * float(w) / float(h));
	}
	float static CalculateFoVY(int w, int h, float FoVX)
	{
		return 2.0f * atan(tan(FoVX * 0.5f) * float(h) / float(w));
	}

	DirectX::XMVECTOR GetUVector() const;
	DirectX::XMVECTOR GetVVector() const;
	DirectX::XMVECTOR GetWVector() const;

	void SetLookAt(
		DirectX::XMVECTOR EyePosition,
		DirectX::XMVECTOR FocusPosition,
		DirectX::XMVECTOR UpDirection);

	void Rotate(
		float AngleX,
		float AngleY);

	void Update();

	Transform Transform;

	float FoVY;
	float AspectRatio;
	float NearZ;
	float FarZ;

	float FocalLength;		// controls the focus distance of the camera. Impacts the depth of field.
	FStop RelativeAperture;	// controls how wide the aperture is opened. Impacts the depth of field.
	float ShutterTime;		// controls how long the aperture is opened. Impacts the motion blur.
	ISO SensorSensitivity;	// controls how photons are counted/quantized on the digital sensor.

	DirectX::XMMATRIX ViewMatrix = DirectX::XMMatrixIdentity();
	DirectX::XMMATRIX ProjectionMatrix = DirectX::XMMatrixIdentity();
	DirectX::XMMATRIX ViewProjectionMatrix = DirectX::XMMatrixIdentity();

	DirectX::XMMATRIX InverseViewMatrix = DirectX::XMMatrixIdentity();
	DirectX::XMMATRIX InverseProjectionMatrix = DirectX::XMMatrixIdentity();
	DirectX::XMMATRIX InverseViewProjectionMatrix = DirectX::XMMatrixIdentity();

	DirectX::XMMATRIX PrevViewProjectionMatrix = DirectX::XMMatrixIdentity();
};
