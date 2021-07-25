#pragma once

struct BoxCollider : Component
{
	BoxCollider() noexcept = default;
	BoxCollider(float hx, float hy, float hz)
		: Extents(hx, hy, hz)
	{
	}

	DirectX::XMFLOAT3 Extents = { 1.0f, 1.0f, 1.0f };
};

REGISTER_CLASS_ATTRIBUTES(BoxCollider, CLASS_ATTRIBUTE(BoxCollider, Extents))
