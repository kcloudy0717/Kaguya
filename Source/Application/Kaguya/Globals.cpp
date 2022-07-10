#include "Globals.h"

namespace Kaguya
{
	ShaderCompiler*		 Compiler	  = nullptr;
	RHI::D3D12Device*	 Device		  = nullptr;
	Asset::AssetManager* AssetManager = nullptr;
} // namespace Kaguya

GlobalRHIContext::GlobalRHIContext()
{
	assert(!Kaguya::Compiler && !Kaguya::Device);

	RHI::DeviceOptions DeviceOptions = {};
#if _DEBUG
	DeviceOptions.EnableDebugLayer		   = true;
	DeviceOptions.EnableGpuBasedValidation = false;
	DeviceOptions.EnableAutoDebugName	   = true;
#endif
	DeviceOptions.FeatureLevel	   = D3D_FEATURE_LEVEL_12_0;
	DeviceOptions.Raytracing	   = true;
	DeviceOptions.DynamicResources = true;
	DeviceOptions.MeshShaders	   = true;

	Compiler = std::make_unique<ShaderCompiler>();
	Device	 = std::make_unique<RHI::D3D12Device>(DeviceOptions);

	Kaguya::Device	 = Device.get();
	Kaguya::Compiler = Compiler.get();
}

GlobalRHIContext::~GlobalRHIContext()
{
	Device->WaitIdle();
}

GlobalAssetManagerContext::GlobalAssetManagerContext()
{
	assert(!Kaguya::AssetManager);

	AssetManager = std::make_unique<Asset::AssetManager>(Kaguya::Device);

	Kaguya::AssetManager = AssetManager.get();
}

GlobalAssetManagerContext::~GlobalAssetManagerContext()
{
}
