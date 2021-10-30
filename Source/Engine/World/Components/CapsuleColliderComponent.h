#pragma once

class CapsuleColliderComponent
{
public:
	CapsuleColliderComponent() noexcept = default;
	explicit CapsuleColliderComponent(float Radius, float Height)
		: Radius(Radius)
		, Height(Height)
	{
	}

	float Radius = 0.5f;
	float Height = 1.0f;
};

REGISTER_CLASS_ATTRIBUTES(
	CapsuleColliderComponent,
	"CapsuleCollider",
	CLASS_ATTRIBUTE(CapsuleColliderComponent, Radius),
	CLASS_ATTRIBUTE(CapsuleColliderComponent, Height))
