#pragma once

struct Camera : Component
{
	DirectX::XMVECTOR GetUVector() const;
	DirectX::XMVECTOR GetVVector() const;
	DirectX::XMVECTOR GetWVector() const;

	void SetLookAt(DirectX::XMVECTOR EyePosition, DirectX::XMVECTOR FocusPosition, DirectX::XMVECTOR UpDirection);

	void Update();

	void Translate(float DeltaX, float DeltaY, float DeltaZ);

	void Rotate(float AngleX, float AngleY, float AngleZ);

	Transform* pTransform;

	bool Dirty = true;

	float FoVY		  = 65.0f; // Degrees
	float AspectRatio = 16.0f / 9.0f;
	float NearZ		  = 0.1f;
	float FarZ		  = 100.0f;

	float FocalLength	   = 10.0f; // controls the focus distance of the camera. Impacts the depth of field.
	float RelativeAperture = 0.0f;	// controls how wide the aperture is opened. Impacts the depth of field.

	DirectX::XMMATRIX ViewMatrix		   = DirectX::XMMatrixIdentity();
	DirectX::XMMATRIX ProjectionMatrix	   = DirectX::XMMatrixIdentity();
	DirectX::XMMATRIX ViewProjectionMatrix = DirectX::XMMatrixIdentity();

	DirectX::XMMATRIX InverseViewMatrix			  = DirectX::XMMatrixIdentity();
	DirectX::XMMATRIX InverseProjectionMatrix	  = DirectX::XMMatrixIdentity();
	DirectX::XMMATRIX InverseViewProjectionMatrix = DirectX::XMMatrixIdentity();

	DirectX::XMMATRIX PrevViewProjectionMatrix = DirectX::XMMatrixIdentity();
};

REGISTER_CLASS_ATTRIBUTES(
	Camera,
	CLASS_ATTRIBUTE(Camera, FoVY),
	CLASS_ATTRIBUTE(Camera, AspectRatio),
	CLASS_ATTRIBUTE(Camera, NearZ),
	CLASS_ATTRIBUTE(Camera, FarZ),
	CLASS_ATTRIBUTE(Camera, FocalLength),
	CLASS_ATTRIBUTE(Camera, RelativeAperture))
