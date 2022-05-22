#pragma once
#include "RHI/RHI.h"
#include "Core/Asset/AssetManager.h"

namespace Kaguya
{
	extern ShaderCompiler*		Compiler;
	extern RHI::D3D12Device*	Device;
	extern Asset::AssetManager* AssetManager;
} // namespace Kaguya

// Manages global state for RHI, should be initialized before anything else
class GlobalRHIContext
{
public:
	GlobalRHIContext();
	~GlobalRHIContext();

private:
	std::unique_ptr<ShaderCompiler>	  Compiler;
	std::unique_ptr<RHI::D3D12Device> Device;
};

// Manages global state for asset management, should be initialized before anything else
class GlobalAssetManagerContext
{
public:
	GlobalAssetManagerContext();
	~GlobalAssetManagerContext();

private:
	std::unique_ptr<Asset::AssetManager> AssetManager;
};
