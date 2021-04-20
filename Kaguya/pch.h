#pragma once
#pragma warning(disable : 26812) // prefer enum class over enum warning

// Win32 APIs
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers.
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif 

#include <Windows.h>
#include <DirectXMath.h>
#include <DirectXColors.h>
#include <wrl/client.h>

// C++ Standard Libraries
#include <cassert>
#include <memory>
#include <typeinfo>
#include <functional>
#include <algorithm>
#include <numeric>
#include <string>
#include <filesystem>
#include <iostream>
#include <exception>
#include <optional>
#include <variant>
#include <sstream>
#include <codecvt>
#include <optional>
#include <future>

// c++ stl
#include <array>
#include <vector>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <queue>

// operator <=>
#include <compare>

// dxgi
#include <dxgi1_6.h>
#if defined(_DEBUG)
#include <dxgidebug.h>
#endif
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")

// d3d12
#include "d3d12.h"
#include "d3d12sdklayers.h"
#include "d3d12shader.h"
#pragma comment(lib, "d3d12.lib")
#include <pix3.h>

// ext
#include <imgui.h>
#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_dx12.h>
#include <ImGuizmo.h>
#include <entt.hpp>
#include <wil/resource.h>
#include <D3D12MemAlloc.h>
#include <GraphicsMemory.h>
#include <DirectXTex.h>
#include <nfd.h>

#include <Core/Application.h>
#include <Core/Pool.h>
#include <Core/Utility.h>
#include <Core/Log.h>
#include <Core/Math.h>
#include <Core/Exception.h>
#include <Core/Synchronization/RWLock.h>
#include <Core/Synchronization/CriticalSection.h>
#include <Core/Synchronization/ConditionVariable.h>

template<typename T>
void SafeRelease(T*& Ptr)
{
	if (Ptr)
	{
		Ptr->Release();
		Ptr = nullptr;
	}
}

inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw COMException(__FILE__, __LINE__, hr);
	}
}

using ShaderIdentifier = std::array<BYTE, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES>;
