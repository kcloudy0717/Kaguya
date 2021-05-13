#pragma once
#include "DescriptorHeap.h"

class SwapChain
{
public:
	static constexpr UINT BackBufferCount = 3;
	static constexpr DXGI_FORMAT Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	static constexpr DXGI_FORMAT Format_sRGB = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	SwapChain() noexcept = default;
	SwapChain(
		HWND hWnd,
		IDXGIFactory5* pFactory5,
		ID3D12Device* pDevice,
		ID3D12CommandQueue* pCommandQueue);

	ID3D12Resource* GetBackBuffer(UINT Index) const;

	std::pair<ID3D12Resource*, Descriptor> GetCurrentBackBufferResource() const;

	void Resize(
		UINT Width,
		UINT Height);

	void Present(
		bool VSync);

private:
	void CreateRenderTargetViews();

private:
	ID3D12Device* m_pDevice;
	Microsoft::WRL::ComPtr<IDXGISwapChain4>	m_SwapChain;
	DescriptorHeap m_RTVHeaps;
	std::array<Descriptor, BackBufferCount> m_RenderTargetViews;
	bool m_TearingSupport;
};
