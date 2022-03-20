#pragma once

class CameraComponent
{
public:
	[[nodiscard]] DirectX::XMVECTOR GetUVector() const;
	[[nodiscard]] DirectX::XMVECTOR GetVVector() const;
	[[nodiscard]] DirectX::XMVECTOR GetWVector() const;

	void SetLookAt(DirectX::XMVECTOR EyePosition, DirectX::XMVECTOR FocusPosition, DirectX::XMVECTOR UpDirection);

	void Update();

	void Translate(float DeltaX, float DeltaY, float DeltaZ);

	void Rotate(float AngleX, float AngleY, float AngleZ);

	// This only exist to verify the more efficient way of constructing frustum, refer to Frustum constructor
	[[nodiscard]] Frustum CreateFrustum() const;

	Transform Transform;

	float FoVY		  = 90.0f; // Degrees
	float AspectRatio = 16.0f / 9.0f;
	float NearZ		  = 0.1f;
	float FarZ		  = 1000.0f;

	float FocalLength	   = 10.0f; // controls the focus distance of the camera. Impacts the depth of field.
	float RelativeAperture = 0.0f;	// controls how wide the aperture is opened. Impacts the depth of field.

	float MovementSpeed		= 20.0f;
	float StrafeSpeed		= 20.0f;
	float MouseSensitivityX = 25.0f;
	float MouseSensitivityY = 25.0f;

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
	CameraComponent,
	"Camera",
	CLASS_ATTRIBUTE(CameraComponent, FoVY),
	CLASS_ATTRIBUTE(CameraComponent, AspectRatio),
	CLASS_ATTRIBUTE(CameraComponent, NearZ),
	CLASS_ATTRIBUTE(CameraComponent, FarZ),
	CLASS_ATTRIBUTE(CameraComponent, FocalLength),
	CLASS_ATTRIBUTE(CameraComponent, RelativeAperture),
	CLASS_ATTRIBUTE(CameraComponent, MovementSpeed),
	CLASS_ATTRIBUTE(CameraComponent, StrafeSpeed))
