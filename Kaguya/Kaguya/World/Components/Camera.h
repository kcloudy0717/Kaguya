#pragma once

struct Camera : Component
{
	[[nodiscard]] DirectX::XMVECTOR GetUVector() const;
	[[nodiscard]] DirectX::XMVECTOR GetVVector() const;
	[[nodiscard]] DirectX::XMVECTOR GetWVector() const;

	void SetLookAt(DirectX::XMVECTOR EyePosition, DirectX::XMVECTOR FocusPosition, DirectX::XMVECTOR UpDirection);

	void Update();

	void Translate(float DeltaX, float DeltaY, float DeltaZ);

	void Rotate(float AngleX, float AngleY, float AngleZ);

	// This only exist to verify the more efficient way of constructing frustum, refer to Frustum constructor
	[[nodiscard]] Frustum CreateFrustum() const;

	Transform* pTransform;

	float FoVY		  = 65.0f; // Degrees
	float AspectRatio = 16.0f / 9.0f;
	float NearZ		  = 0.1f;
	float FarZ		  = 1000.0f;

	float FocalLength	   = 10.0f; // controls the focus distance of the camera. Impacts the depth of field.
	float RelativeAperture = 0.0f;	// controls how wide the aperture is opened. Impacts the depth of field.

	float MovementSpeed		= 20.0f;
	float StrafeSpeed		= 20.0f;
	float MouseSensitivityX = 15.0f;
	float MouseSensitivityY = 15.0f;

	bool Momentum = true;

	DirectX::XMFLOAT4X4 View;
	DirectX::XMFLOAT4X4 Projection;
	DirectX::XMFLOAT4X4 ViewProjection;

	DirectX::XMFLOAT4X4 InverseView;
	DirectX::XMFLOAT4X4 InverseProjection;
	DirectX::XMFLOAT4X4 InverseViewProjection;

	DirectX::XMFLOAT4X4 PrevViewProjection;

	bool Dirty = true;
	bool Main  = true;
};

REGISTER_CLASS_ATTRIBUTES(
	Camera,
	CLASS_ATTRIBUTE(Camera, FoVY),
	CLASS_ATTRIBUTE(Camera, AspectRatio),
	CLASS_ATTRIBUTE(Camera, NearZ),
	CLASS_ATTRIBUTE(Camera, FarZ),
	CLASS_ATTRIBUTE(Camera, FocalLength),
	CLASS_ATTRIBUTE(Camera, RelativeAperture))
