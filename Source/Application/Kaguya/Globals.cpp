#include "Globals.h"

namespace Kaguya
{
	ShaderCompiler*		 Compiler	  = nullptr;
	RHI::D3D12Device*	 Device		  = nullptr;
	Asset::AssetManager* AssetManager = nullptr;
} // namespace Kaguya

D3D12RHIInitializer::D3D12RHIInitializer(const RHI::DeviceOptions& Options)
{
	assert(!Kaguya::Compiler && !Kaguya::Device);

	Compiler = std::make_unique<ShaderCompiler>();
	Device	 = std::make_unique<RHI::D3D12Device>(Options);

	// First descriptor always reserved for imgui
	UINT						ImGuiIndex		   = 0;
	D3D12_CPU_DESCRIPTOR_HANDLE ImGuiCpuDescriptor = {};
	D3D12_GPU_DESCRIPTOR_HANDLE ImGuiGpuDescriptor = {};
	Device->GetLinkedDevice()->GetResourceDescriptorHeap().Allocate(ImGuiCpuDescriptor, ImGuiGpuDescriptor, ImGuiIndex);

	// Initialize ImGui for d3d12
	ImGuiD3D12Initialized = ImGui_ImplDX12_Init(
		Device->GetD3D12Device(),
		1,
		RHI::D3D12SwapChain::Format,
		Device->GetLinkedDevice()->GetResourceDescriptorHeap(),
		ImGuiCpuDescriptor,
		ImGuiGpuDescriptor);

	if (Device->SupportsDynamicResources())
	{
		Compiler->SetShaderModel(RHI_SHADER_MODEL::ShaderModel_6_6);
	}

	Kaguya::Device	 = Device.get();
	Kaguya::Compiler = Compiler.get();
}

D3D12RHIInitializer::~D3D12RHIInitializer()
{
	Device->WaitIdle();
	if (ImGuiD3D12Initialized)
	{
		ImGui_ImplDX12_Shutdown();
	}
}

AssetManagerInitializer::AssetManagerInitializer()
{
	assert(!Kaguya::AssetManager);

	AssetManager = std::make_unique<Asset::AssetManager>(Kaguya::Device);

	Kaguya::AssetManager = AssetManager.get();
}
