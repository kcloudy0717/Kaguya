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
#include <thread>
#include <condition_variable>
#include <span>
#include <ranges>

// c++ stl
#include <array>
#include <vector>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <queue>

// win32
#define WIN32_LEAN_AND_MEAN

#define NOGDICAPMASKS	 // CC_*, LC_*, PC_*, CP_*, TC_*, RC_
#define NORASTEROPS		 // Binary and Tertiary raster ops
#define NODRAWTEXT		 // DrawText() and DT_*
//#define NOGDI			 // All GDI defines and routines
//#define NONLS			 // All NLS defines and routines
#define NOMINMAX		 // Macros min(a,b) and max(a,b)
#define NOSERVICE		 // All Service Controller routines, SERVICE_ equates, etc.
#define NOSOUND			 // Sound driver routines
#define NOHELP			 // Help engine interface. (Deprecated)
#define NOPROFILER		 // Profiler interface.
#define NODEFERWINDOWPOS // DeferWindowPos routines
#define NOMCX			 // Modem Configuration Extensions

#include <Windows.h>
#include <synchapi.h>
#include <wrl/client.h>
#include <wrl/event.h>
#include <shobjidl.h>
#include <strsafe.h>

// DXGI
#include <dxgi1_6.h>
#if defined(_DEBUG)
#include <dxgidebug.h>
#endif
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")

// D3D12
#include "d3d12.h"
#include "d3d12sdklayers.h"
#include "d3d12shader.h"
#pragma comment(lib, "d3d12.lib")
#include <pix3.h>

#include <entt.hpp>
#include <imgui.h>
#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_dx12.h>
#include <ImGuizmo.h>
#include <wil/resource.h>
#include <city.h>
#include <DirectXMesh.h>
#include <DirectXTex.h>

#include <Core/Core.h>

// https://docs.microsoft.com/en-us/windows/win32/debug/retrieving-the-last-error-code?redirectedfrom=MSDN
inline static void ErrorExit(LPCTSTR lpszFunction)
{
	// Retrieve the system error message for the last-error code
	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	DWORD  dw = GetLastError();

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0,
		nullptr);

	// Display the error message and exit the process
	lpDisplayBuf = (LPVOID)LocalAlloc(
		LMEM_ZEROINIT,
		(lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
	StringCchPrintf(
		(LPTSTR)lpDisplayBuf,
		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		TEXT("%s failed with error %d: %s"),
		lpszFunction,
		dw,
		lpMsgBuf);
	MessageBox(nullptr, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
	ExitProcess(dw);
}
