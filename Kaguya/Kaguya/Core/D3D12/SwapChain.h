#pragma once
#include "D3D12Common.h"

class SwapChain
{
public:
	static constexpr UINT		 BackBufferCount = 3;
	static constexpr DXGI_FORMAT Format			 = DXGI_FORMAT_R8G8B8A8_UNORM;
	static constexpr DXGI_FORMAT Format_sRGB	 = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	SwapChain() noexcept = default;

	SwapChain(HWND hWnd, IDXGIFactory5* pFactory5, ID3D12Device* pDevice, ID3D12CommandQueue* pCommandQueue);

	ID3D12Resource* GetBackBuffer(UINT Index) const;

	std::pair<ID3D12Resource*, D3D12_CPU_DESCRIPTOR_HANDLE> GetCurrentBackBufferResource() const;

	void Resize(UINT Width, UINT Height);

	void Present(bool VSync);

private:
	void CreateRenderTargetViews();

private:
	ID3D12Device*											 pDevice = nullptr;
	Microsoft::WRL::ComPtr<IDXGISwapChain4>					 SwapChain4;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>			 RTVHeaps;
	std::array<D3D12_CPU_DESCRIPTOR_HANDLE, BackBufferCount> RenderTargetViews;
	bool													 TearingSupport = false;
};
