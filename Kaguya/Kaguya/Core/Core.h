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
#include "RHI/D3D12/D3D12Profiler.h"

#include "RHI/D3D12/D3D12Common.h"
#include "RHI/D3D12/D3D12Device.h"
#include "RHI/D3D12/D3D12LinkedDevice.h"
#include "RHI/D3D12/D3D12CommandQueue.h"
#include "RHI/D3D12/D3D12PipelineState.h"
#include "RHI/D3D12/D3D12RaytracingPipelineState.h"
#include "RHI/D3D12/D3D12Resource.h"
#include "RHI/D3D12/D3D12ResourceUploader.h"
#include "RHI/D3D12/D3D12RootSignature.h"
#include "RHI/D3D12/D3D12CommandSignature.h"
#include "RHI/D3D12/D3D12SwapChain.h"
#include "RHI/D3D12/D3D12Raytracing.h"
#include "RHI/D3D12/D3D12RaytracingShaderTable.h"

#include "RHI/D3D12/ShaderCompiler.h"
