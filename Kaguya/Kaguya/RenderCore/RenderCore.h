#pragma once

namespace RenderCore
{
void Initialize();

void Shutdown();

extern D3D12Device*				   Device;
extern D3D12SwapChain*			   SwapChain;
extern ShaderCompiler*			   Compiler;
extern D3D12_CPU_DESCRIPTOR_HANDLE ImGuiCpuDescriptor;
extern D3D12_GPU_DESCRIPTOR_HANDLE ImGuiGpuDescriptor;

static constexpr DXGI_FORMAT DepthFormat		= DXGI_FORMAT_D32_FLOAT;
static constexpr DXGI_FORMAT DepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

} // namespace RenderCore
