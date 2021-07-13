#pragma once

struct BoxCollider : Component
{
	BoxCollider() noexcept = default;
	BoxCollider(float hx, float hy, float hz)
		: Extents(hx, hy, hz)
	{
	}

	DirectX::XMFLOAT3 Extents;
};
