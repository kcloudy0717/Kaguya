#include "RenderCore.h"

namespace RenderCore
{
D3D12Device*					pAdapter		   = nullptr;
D3D12SwapChain*					pSwapChain		   = nullptr;
ShaderCompiler*				pShaderCompiler	   = nullptr;
D3D12_CPU_DESCRIPTOR_HANDLE ImGuiCpuDescriptor = {};
D3D12_GPU_DESCRIPTOR_HANDLE ImGuiGpuDescriptor = {};

const char* GetD3D12MessageSeverity(D3D12_MESSAGE_SEVERITY Severity)
{
	switch (Severity)
	{
	case D3D12_MESSAGE_SEVERITY_CORRUPTION:
		return "[Corruption]";
	case D3D12_MESSAGE_SEVERITY_ERROR:
		return "[Error]";
	case D3D12_MESSAGE_SEVERITY_WARNING:
		return "[Warning]";
	case D3D12_MESSAGE_SEVERITY_INFO:
		return "[Info]";
	case D3D12_MESSAGE_SEVERITY_MESSAGE:
		return "[Message]";
	default:
		return "<unknown>";
	}
}

void Initialize()
{
	DeviceOptions  DeviceOptions  = { .EnableDebugLayer			= true,
									  .EnableGpuBasedValidation = true,
									  .EnableAutoDebugName		= true };
	DeviceFeatures DeviceFeatures = { .FeatureLevel = D3D_FEATURE_LEVEL_12_0, .WaveOperation = true };

	pAdapter = new D3D12Device();
	pAdapter->Initialize(DeviceOptions);
	pAdapter->InitializeDevice(DeviceFeatures);

	pAdapter->RegisterMessageCallback(
		[](D3D12_MESSAGE_CATEGORY Category,
		   D3D12_MESSAGE_SEVERITY Severity,
		   D3D12_MESSAGE_ID		  ID,
		   LPCSTR				  pDescription,
		   void*				  pContext)
		{
			LOG_INFO("Severity: {}\n{}", GetD3D12MessageSeverity(Severity), pDescription);
		});

	pSwapChain = new D3D12SwapChain(
		Application::GetWindowHandle(),
		pAdapter->GetFactory6(),
		pAdapter->GetD3D12Device(),
		pAdapter->GetDevice()->GetGraphicsQueue()->GetCommandQueue());

	UINT TempIndex = 0;
	pAdapter->GetDevice()->GetResourceDescriptorHeap().Allocate(ImGuiCpuDescriptor, ImGuiGpuDescriptor, TempIndex);

	// Initialize ImGui for d3d12
	ImGui_ImplDX12_Init(
		pAdapter->GetD3D12Device(),
		1,
		D3D12SwapChain::Format,
		pAdapter->GetDevice()->GetResourceDescriptorHeap(),
		ImGuiCpuDescriptor,
		ImGuiGpuDescriptor);

	pShaderCompiler = new ShaderCompiler();
	pShaderCompiler->Initialize();
	pShaderCompiler->SetShaderModel(EShaderModel::ShaderModel_6_6);
	pShaderCompiler->SetIncludeDirectory(Application::ExecutableDirectory / L"Shaders");
}

void Shutdown()
{
	pAdapter->GetDevice()->GetGraphicsQueue()->Flush();
	pAdapter->GetDevice()->GetAsyncComputeQueue()->Flush();
	pAdapter->GetDevice()->GetCopyQueue1()->Flush();
	pAdapter->GetDevice()->GetCopyQueue2()->Flush();

	ImGui_ImplDX12_Shutdown();
	pAdapter->UnregisterMessageCallback();

	delete pShaderCompiler;
	delete pSwapChain;
	delete pAdapter;

	D3D12Device::ReportLiveObjects();
}

} // namespace RenderCore
