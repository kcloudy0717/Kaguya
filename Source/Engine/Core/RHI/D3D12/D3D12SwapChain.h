#pragma once
#include "D3D12Common.h"
#include "D3D12Fence.h"

// This class allows for customizable presents
class IPresent
{
public:
	virtual ~IPresent() = default;

	virtual void PrePresent() {}

	virtual void PostPresent() {}
};

struct D3D12SwapChainResource
{
	ID3D12Resource*				Resource;
	D3D12_CPU_DESCRIPTOR_HANDLE View;
};

class D3D12SwapChain : public D3D12DeviceChild
{
public:
	static constexpr UINT BackBufferCount = 3;

	static constexpr DXGI_FORMAT Format		 = DXGI_FORMAT_R8G8B8A8_UNORM;
	static constexpr DXGI_FORMAT Format_sRGB = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	explicit D3D12SwapChain(
		D3D12Device* Parent,
		HWND		 HWnd);
	~D3D12SwapChain();

	D3D12SwapChain(D3D12SwapChain&&) noexcept = default;
	D3D12SwapChain& operator=(D3D12SwapChain&&) noexcept = default;

	D3D12SwapChain(const D3D12SwapChain&) = delete;
	D3D12SwapChain& operator=(const D3D12SwapChain&) = delete;

	[[nodiscard]] ID3D12Resource*		 GetBackBuffer(UINT Index) const;
	[[nodiscard]] D3D12SwapChainResource GetCurrentBackBufferResource() const;

	[[nodiscard]] D3D12_VIEWPORT GetViewport() const noexcept;
	[[nodiscard]] D3D12_RECT	 GetScissorRect() const noexcept;

	void Resize(UINT Width, UINT Height);

	void Present(bool VSync, IPresent& Present);

private:
	Microsoft::WRL::ComPtr<IDXGISwapChain4> InitializeSwapChain();

	void CreateRenderTargetViews();

private:
	HWND WindowHandle	= nullptr;
	UINT Width			= 0;
	UINT Height			= 0;
	bool TearingSupport = false;

	Microsoft::WRL::ComPtr<IDXGISwapChain4>								SwapChain4;
	std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, BackBufferCount> BackBuffers;
	D3D12DescriptorArray												RenderTargetViews;

	D3D12Fence		Fence;
	D3D12SyncHandle SyncHandle;
};
