#pragma once

struct CapsuleCollider : Component
{
	CapsuleCollider() noexcept = default;
	CapsuleCollider(float Radius, float Height)
		: Radius(Radius)
		, Height(Height)
	{
	}

	float Radius;
	float Height;
};
