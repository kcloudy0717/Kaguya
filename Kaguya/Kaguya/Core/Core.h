#pragma once
#include "CoreDefines.h"
#include "CoreException.h"

#include "Log.h"
#include "Console.h"

// Application
#include "Application.h"

// Math
#include "Math.h"

// Threading
#include "CriticalSection.h"
#include "RWLock.h"

// Asset
#include "Asset/AssetCache.h"
#include "Asset/AsyncLoader.h"
#include "Asset/Image.h"
#include "Asset/Mesh.h"

// D3D12
#include "D3D12/Profiler.h"

#include "D3D12/D3D12Common.h"
#include "D3D12/Adapter.h"
#include "D3D12/Device.h"
#include "D3D12/CommandQueue.h"
#include "D3D12/PipelineState.h"
#include "D3D12/RaytracingPipelineState.h"
#include "D3D12/Resource.h"
#include "D3D12/ResourceUploader.h"
#include "D3D12/RootSignature.h"
#include "D3D12/CommandSignature.h"
#include "D3D12/SwapChain.h"
#include "D3D12/Raytracing.h"
#include "D3D12/RaytracingShaderTable.h"

#include "D3D12/ShaderCompiler.h"
