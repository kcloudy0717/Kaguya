#pragma once
#include "RHI/RHI.h"
#include "Core/Asset/AssetManager.h"

namespace Kaguya
{
	extern ShaderCompiler*		Compiler;
	extern RHI::D3D12Device*	Device;
	extern Asset::AssetManager* AssetManager;
} // namespace Kaguya

class D3D12RHIInitializer
{
public:
	D3D12RHIInitializer(const RHI::DeviceOptions& Options);
	~D3D12RHIInitializer();

private:
	std::unique_ptr<ShaderCompiler>	  Compiler;
	std::unique_ptr<RHI::D3D12Device> Device;
	bool							  ImGuiD3D12Initialized = false;
};

class AssetManagerInitializer
{
public:
	AssetManagerInitializer();

private:
	std::unique_ptr<Asset::AssetManager> AssetManager;
};
