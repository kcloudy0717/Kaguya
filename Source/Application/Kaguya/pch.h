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
#include <shobjidl.h>
#include <strsafe.h>

#include <wil/resource.h>

#include <imgui.h>
#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_dx12.h>
#include <ImGuizmo.h>
