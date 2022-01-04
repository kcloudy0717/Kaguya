#include "RenderCore.h"

namespace RenderCore
{
D3D12Device*	Device	 = nullptr;
ShaderCompiler* Compiler = nullptr;
} // namespace RenderCore

RenderCoreInitializer::RenderCoreInitializer(const DeviceOptions& Options)
{
	assert(!Device && !Compiler);

	Device = std::make_unique<D3D12Device>(Options);

	// First descriptor always reserved for imgui
	UINT						ImGuiIndex		   = 0;
	D3D12_CPU_DESCRIPTOR_HANDLE ImGuiCpuDescriptor = {};
	D3D12_GPU_DESCRIPTOR_HANDLE ImGuiGpuDescriptor = {};
	Device->GetDevice()->GetResourceDescriptorHeap().Allocate(ImGuiCpuDescriptor, ImGuiGpuDescriptor, ImGuiIndex);

	// Initialize ImGui for d3d12
	ImGuiD3D12Initialized = ImGui_ImplDX12_Init(
		Device->GetD3D12Device(),
		1,
		D3D12SwapChain::Format,
		Device->GetDevice()->GetResourceDescriptorHeap(),
		ImGuiCpuDescriptor,
		ImGuiGpuDescriptor);

	Compiler = std::make_unique<ShaderCompiler>();
	Compiler->SetShaderModel(RHI_SHADER_MODEL::ShaderModel_6_6);

	RenderCore::Device	 = Device.get();
	RenderCore::Compiler = Compiler.get();
}

RenderCoreInitializer::~RenderCoreInitializer()
{
	Device->WaitIdle();
	if (ImGuiD3D12Initialized)
	{
		ImGui_ImplDX12_Shutdown();
	}
}
