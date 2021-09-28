#include "RenderCore.h"

namespace RenderCore
{
D3D12Device*				Device			   = nullptr;
D3D12SwapChain*				SwapChain		   = nullptr;
ShaderCompiler*				Compiler		   = nullptr;
D3D12_CPU_DESCRIPTOR_HANDLE ImGuiCpuDescriptor = {};
D3D12_GPU_DESCRIPTOR_HANDLE ImGuiGpuDescriptor = {};

void Initialize()
{
	DeviceOptions  DeviceOptions	= { .EnableDebugLayer		  = true,
									.EnableGpuBasedValidation = false,
									.EnableAutoDebugName	  = true };
	DeviceFeatures DeviceFeatures	= {};
	DeviceFeatures.FeatureLevel		= D3D_FEATURE_LEVEL_12_0;
	DeviceFeatures.DynamicResources = true;

	Device = new D3D12Device();
	Device->Initialize(DeviceOptions);
	Device->InitializeDevice(DeviceFeatures);

	SwapChain = new D3D12SwapChain(
		Application::GetWindowHandle(),
		Device->GetDxgiFactory6(),
		Device->GetD3D12Device(),
		Device->GetDevice()->GetGraphicsQueue()->GetCommandQueue());

	UINT TempIndex = 0;
	Device->GetDevice()->GetResourceDescriptorHeap().Allocate(ImGuiCpuDescriptor, ImGuiGpuDescriptor, TempIndex);

	// Initialize ImGui for d3d12
	ImGui_ImplDX12_Init(
		Device->GetD3D12Device(),
		1,
		D3D12SwapChain::Format,
		Device->GetDevice()->GetResourceDescriptorHeap(),
		ImGuiCpuDescriptor,
		ImGuiGpuDescriptor);

	Compiler = new ShaderCompiler();
	Compiler->Initialize();
	Compiler->SetShaderModel(EShaderModel::ShaderModel_6_6);
	Compiler->SetIncludeDirectory(Application::ExecutableDirectory / L"Shaders");
}

void Shutdown()
{
	Device->GetDevice()->GetGraphicsQueue()->WaitIdle();
	Device->GetDevice()->GetAsyncComputeQueue()->WaitIdle();
	Device->GetDevice()->GetCopyQueue1()->WaitIdle();
	Device->GetDevice()->GetCopyQueue2()->WaitIdle();

	ImGui_ImplDX12_Shutdown();

	delete Compiler;
	delete SwapChain;
	delete Device;

	D3D12Device::ReportLiveObjects();
}
} // namespace RenderCore
