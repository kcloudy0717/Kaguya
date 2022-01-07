#pragma once
#include "CoreDefines.h"
#include "Exception.h"
#include "Delegate.h"

#include "Log.h"
#include "Console.h"

// Application
#include "Application.h"

// Math
#include "Math/Math.h"

// Threading
#include "Sync.h"
#include "ThreadPool.h"

// Asset
#include "Asset/AssetCache.h"
#include "Asset/AsyncImporter.h"
#include "Asset/Texture.h"
#include "Asset/Mesh.h"

// Async
#include "Async/Async.h"
#include "Async/AsyncAction.h"
#include "Async/AsyncTask.h"
#include "Async/Generator.h"

// Math
#include "Math/Math.h"
#include "Math/Ray.h"
#include "Math/Plane.h"
#include "Math/BoundingBox.h"
#include "Math/Frustum.h"
#include "Math/Transform.h"

// System
#include "System/FileStream.h"
#include "System/AsyncFileStream.h"
#include "System/BinaryReader.h"
#include "System/BinaryWriter.h"
#include "System/MemoryMappedView.h"
#include "System/MemoryMappedFile.h"

#include "System/FileSystem.h"
#include "System/FileSystemWatcher.h"

// RHI
#include "RHI/ShaderCompiler.h"

// D3D12
#include "RHI/D3D12/D3D12Profiler.h"

#include "RHI/D3D12/D3D12Common.h"
#include "RHI/D3D12/D3D12Device.h"
#include "RHI/D3D12/D3D12LinkedDevice.h"
#include "RHI/D3D12/D3D12Profiler.h"
#include "RHI/D3D12/D3D12CommandQueue.h"
#include "RHI/D3D12/D3D12CommandList.h"
#include "RHI/D3D12/D3D12CommandContext.h"
#include "Rhi/D3D12/D3D12DescriptorHeap.h"
#include "RHI/D3D12/D3D12RootSignature.h"
#include "RHI/D3D12/D3D12PipelineState.h"
#include "RHI/D3D12/D3D12Resource.h"
#include "RHI/D3D12/D3D12Descriptor.h"
#include "RHI/D3D12/D3D12SwapChain.h"
#include "RHI/D3D12/D3D12Fence.h"
#include "RHI/D3D12/D3D12RenderTarget.h"
#include "RHI/D3D12/D3D12Raytracing.h"
#include "RHI/D3D12/D3D12CommandSignature.h"
