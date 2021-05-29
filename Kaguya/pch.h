#pragma once
#pragma warning(disable : 26812) // prefer enum class over enum warning

// c++ std
#include <stdexcept>
#include <exception>
#include <cassert>
#include <memory>
#include <functional>
#include <algorithm>
#include <numeric>
#include <string>
#include <filesystem>
#include <optional>
#include <compare>
#include <mutex>

// c++ stl
#include <array>
#include <vector>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <queue>

// win32
#define NOMINMAX

// DirectX apps don't need GDI
#define NODRAWTEXT
//#define NOGDI
#define NOBITMAP

// Include <mcx.h> if you need this
#define NOMCX

// Include <winsvc.h> if you need this
#define NOSERVICE

// WinHelp is deprecated
#define NOHELP

#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers.

#include <Windows.h>
#include <synchapi.h>
#include <wrl/client.h>
#include <wrl/event.h>

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

#include <DirectXMath.h>

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
#include <Graphics/D3D12/PIXCapture.h>

inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw hresult_exception(hr);
	}
}

template <typename T>
void SafeRelease(T*& Ptr)
{
	if (Ptr)
	{
		Ptr->Release();
		Ptr = nullptr;
	}
}

// https://docs.microsoft.com/en-us/windows/win32/debug/retrieving-the-last-error-code?redirectedfrom=MSDN
inline static void ErrorExit(LPCTSTR lpszFunction)
{
	// Retrieve the system error message for the last-error code
	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	DWORD dw = GetLastError();

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, nullptr);

	// Display the error message and exit the process
	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
		(lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
	StringCchPrintf((LPTSTR)lpDisplayBuf,
		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		TEXT("%s failed with error %d: %s"),
		lpszFunction, dw, lpMsgBuf);
	MessageBox(nullptr, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
	ExitProcess(dw);
}

using ShaderIdentifier = std::array<BYTE, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES>;
