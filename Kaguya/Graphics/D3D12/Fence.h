#pragma once

class Fence
{
public:
	Fence() noexcept = default;
	Fence(_In_ ID3D12Device* pDevice);

	Fence(Fence&&) noexcept = default;
	Fence& operator=(Fence&&) noexcept = default;

	Fence(const Fence&) = delete;
	Fence& operator=(const Fence&) = delete;

				 operator ID3D12Fence*() const { return m_Fence.Get(); }
	ID3D12Fence* operator->() const { return m_Fence.Get(); }

private:
	Microsoft::WRL::ComPtr<ID3D12Fence> m_Fence;
};
