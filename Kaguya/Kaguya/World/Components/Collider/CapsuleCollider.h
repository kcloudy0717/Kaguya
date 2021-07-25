#pragma once

struct CapsuleCollider : Component
{
	CapsuleCollider() noexcept = default;
	CapsuleCollider(float Radius, float Height)
		: Radius(Radius)
		, Height(Height)
	{
	}

	float Radius = 0.5f;
	float Height = 1.0f;
};

REGISTER_CLASS_ATTRIBUTES(
	CapsuleCollider,
	CLASS_ATTRIBUTE(CapsuleCollider, Radius),
	CLASS_ATTRIBUTE(CapsuleCollider, Height))
