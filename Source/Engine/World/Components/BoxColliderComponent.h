#pragma once

class BoxColliderComponent
{
public:
	BoxColliderComponent() noexcept = default;
	explicit BoxColliderComponent(float hx, float hy, float hz)
		: Extents(hx, hy, hz)
	{
	}

	DirectX::XMFLOAT3 Extents = { 1.0f, 1.0f, 1.0f };
};

REGISTER_CLASS_ATTRIBUTES(BoxColliderComponent, "BoxCollider", CLASS_ATTRIBUTE(BoxColliderComponent, Extents))
