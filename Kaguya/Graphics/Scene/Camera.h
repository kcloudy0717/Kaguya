#pragma once
#include "Components/Transform.h"

using FStop = float;
using ISO = float;
using EV = float;

struct Camera
{
	Camera();
	void RenderGui();

	auto operator<=>(const Camera&) const = default;

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
	EV ExposureValue100() const;

	DirectX::XMMATRIX ViewMatrix() const;
	DirectX::XMMATRIX ProjectionMatrix() const;
	DirectX::XMMATRIX ViewProjectionMatrix() const;
	DirectX::XMMATRIX InverseViewMatrix() const;
	DirectX::XMMATRIX InverseProjectionMatrix() const;
	DirectX::XMMATRIX InverseViewProjectionMatrix() const;

	void SetLookAt(DirectX::XMVECTOR EyePosition, DirectX::XMVECTOR FocusPosition, DirectX::XMVECTOR UpDirection);

	void Translate(float DeltaX, float DeltaY, float DeltaZ);
	void Rotate(float AngleX, float AngleY);

	Transform Transform;

	float FoVY;
	float AspectRatio;
	float NearZ;
	float FarZ;

	float FocalLength;		// controls the focus distance of the camera. Impacts the depth of field.
	FStop RelativeAperture;	// controls how wide the aperture is opened. Impacts the depth of field.
	float ShutterTime;		// controls how long the aperture is opened. Impacts the motion blur.
	ISO SensorSensitivity;	// controls how photons are counted/quantized on the digital sensor.
};