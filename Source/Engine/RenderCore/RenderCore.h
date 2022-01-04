#pragma once
#include "Core/RHI/D3D12/D3D12Device.h"

namespace RenderCore
{
extern D3D12Device*	   Device;
extern ShaderCompiler* Compiler;

static constexpr DXGI_FORMAT DepthFormat		= DXGI_FORMAT_D32_FLOAT;
static constexpr DXGI_FORMAT DepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
} // namespace RenderCore

class RenderCoreInitializer
{
public:
	RenderCoreInitializer(const DeviceOptions& Options);
	~RenderCoreInitializer();

private:
	std::unique_ptr<D3D12Device>	Device;
	std::unique_ptr<ShaderCompiler> Compiler;

	bool ImGuiD3D12Initialized = false;
};
