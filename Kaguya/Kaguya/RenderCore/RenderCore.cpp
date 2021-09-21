#include "RenderCore.h"

namespace RenderCore
{
D3D12Device*				pDevice			   = nullptr;
D3D12SwapChain*				pSwapChain		   = nullptr;
ShaderCompiler*				pShaderCompiler	   = nullptr;
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

	pDevice = new D3D12Device();
	pDevice->Initialize(DeviceOptions);
	pDevice->InitializeDevice(DeviceFeatures);

	pSwapChain = new D3D12SwapChain(
		Application::GetWindowHandle(),
		pDevice->GetFactory6(),
		pDevice->GetD3D12Device(),
		pDevice->GetDevice()->GetGraphicsQueue()->GetCommandQueue());

	UINT TempIndex = 0;
	pDevice->GetDevice()->GetResourceDescriptorHeap().Allocate(ImGuiCpuDescriptor, ImGuiGpuDescriptor, TempIndex);

	// Initialize ImGui for d3d12
	ImGui_ImplDX12_Init(
		pDevice->GetD3D12Device(),
		1,
		D3D12SwapChain::Format,
		pDevice->GetDevice()->GetResourceDescriptorHeap(),
		ImGuiCpuDescriptor,
		ImGuiGpuDescriptor);

	pShaderCompiler = new ShaderCompiler();
	pShaderCompiler->Initialize();
	pShaderCompiler->SetShaderModel(EShaderModel::ShaderModel_6_6);
	pShaderCompiler->SetIncludeDirectory(Application::ExecutableDirectory / L"Shaders");
}

void Shutdown()
{
	pDevice->GetDevice()->GetGraphicsQueue()->WaitIdle();
	pDevice->GetDevice()->GetAsyncComputeQueue()->WaitIdle();
	pDevice->GetDevice()->GetCopyQueue1()->WaitIdle();
	pDevice->GetDevice()->GetCopyQueue2()->WaitIdle();

	ImGui_ImplDX12_Shutdown();

	delete pShaderCompiler;
	delete pSwapChain;
	delete pDevice;

	D3D12Device::ReportLiveObjects();
}
} // namespace RenderCore
