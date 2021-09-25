/*
* Copyright (c) 2019-2021, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

/*
*   █████  █████ ██████ ████  ████   ███████   ████  ██████ ██   ██
*   ██  ██ ██      ██   ██    ██  ██ ██ ██ ██ ██  ██   ██   ██   ██
*   ██  ██ ██      ██   ██    ██  ██ ██ ██ ██ ██  ██   ██   ██   ██
*   ██████ ████    ██   ████  █████  ██ ██ ██ ██████   ██   ███████
*   ██  ██ ██      ██   ██    ██  ██ ██    ██ ██  ██   ██   ██   ██
*   ██  ██ ██      ██   █████ ██  ██ ██    ██ ██  ██   ██   ██   ██   DEBUGGER
*                                                           ██   ██
*  ████████████████████████████████████████████████████████ ██ █ ██ ████████████
*
*
*  HOW TO USE AFTERMATH GPU CRASH DUMP COLLECTION:
*  -----------------------------------------------
*
*  1)  Call 'GFSDK_Aftermath_EnableGpuCrashDumps', to enable GPU crash dump collection.
*      This must be done before any other library calls are made and before any D3D
*      device is created by the application.
*
*      With this call the application can register a callback function that is invoked
*      with the GPU crash dump data once a TDR/hang occurs. In addition, it is also
*      possible to provide optional callback functions for collecting shader debug
*      information and for providing additional descriptive data from the application to
*      include in the crash dump.
*
*      Enabling GPU crash dumps will also override any settings from an also active
*      Nsight Graphics GPU crash dump monitor for the calling process.
*
*
*  2)  On DX11/DX12, call 'GFSDK_Aftermath_DXxx_Initialize', to initialize the library and
*      to enable additional Aftermath features that will affect the data captured in the
*      GPU crash dumps, such as Aftermath event markers; automatic call stack markers for
*      all draw calls, compute dispatches, ray dispatches, or copy operations; resource
*      tracking; or shader debug information. See GFSDK_Aftermath.h for more details.
*
*      On Vulkan use the VK_NV_device_diagnostics_config extension to enable additional
*      Aftermath features, such as automatic call stack markers for all draw calls, compute
*      dispatches, ray dispatches, or copy operations; resource tracking; or shader debug
*      information. See GFSDK_Aftermath.h for more details.
*
*
*  4)  Before the application shuts down, call 'GFSDK_Aftermath_DisableGpuCrashDumps' to
*      disable GPU crash dump collection.
*
*      Disabling GPU crash dumps will also re-establish any settings from an also active
*      Nsight Graphics GPU crash dump monitor for the calling process.
*
*
*  OPTIONAL:
*
*  o)  (Optional) Instrument the application with event markers as described in
*      GFSDK_Aftermath.h.
*  o)  (Optional, D3D12) Register D3D12 resource pointers with Aftermath as described in
*      GFSDK_Aftermath.h.
*  o)  (Optional) To disable all GPU crash dump functionality at runtime:
*          On Windows, set the registry key: 'HKEY_CURRENT_USER\Software\NVIDIA Corporation\Nsight Aftermath\ForceOff'.
*          On Linux, set environment 'NV_AFTERMATH_FORCE_OFF'.
*
*
*  PERFORMANCE TIPS:
*
*  o) Enabling shader debug information creation will introduce shader compile time
*     overhead as well as memory overhead for handling the debug information.
*
*  o) User event markers cause considerable overhead and should be used very carefully.
*     Using automatic callstack markers for draw calls, compute dispatches, ray dispatches,
*     and copy operations may be a less costly alternative to injecting an event marker for
*     every draw call. However, they also do not come for free and using them should be
*     also considered carefully.
*
*/

#ifndef GFSDK_Aftermath_GpuCrashDump_H
#define GFSDK_Aftermath_GpuCrashDump_H

#include "GFSDK_Aftermath_Defines.h"

#pragma pack(push, 8)

#ifdef __cplusplus
extern "C" {
#endif

// Flags to configure for which graphisc APIs to enable GPU crash dumps
GFSDK_AFTERMATH_DECLARE_ENUM(GpuCrashDumpWatchedApiFlags)
{
    // Default setting
    GFSDK_Aftermath_GpuCrashDumpWatchedApiFlags_None = 0x0,

    // Enable GPU crash dump tracking for the DX API
    GFSDK_Aftermath_GpuCrashDumpWatchedApiFlags_DX = 0x1,

    // Enable GPU crash dump tracking for the Vulkan API
    GFSDK_Aftermath_GpuCrashDumpWatchedApiFlags_Vulkan = 0x2,
};

// Flags to configure GPU crash dump-specific Aftermath features
GFSDK_AFTERMATH_DECLARE_ENUM(GpuCrashDumpFeatureFlags)
{
    // Default settings
    GFSDK_Aftermath_GpuCrashDumpFeatureFlags_Default = 0x0,

    // Defer shader debug information callbacks until an actual GPU crash
    // dump is generated and also provide shader debug information
    // for the shaders related to the crash dump only.
    // Note: using this option will increase the memory footprint of the
    // application.
    GFSDK_Aftermath_GpuCrashDumpFeatureFlags_DeferDebugInfoCallbacks = 0x1,
};

// Key definitions for user-defined GPU crash dump description
GFSDK_AFTERMATH_DECLARE_ENUM(GpuCrashDumpDescriptionKey)
{
    // Predefined key for application name
    GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationName = 0x00000001,

    // Predefined key for application version
    GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationVersion = 0x00000002,

    // Base key for creating user-defined key-value pairs.
    // Any value >= GFSDK_Aftermath_GpuCrashDumpDescriptionKey_UserDefined
    // will create a user-defined key-value pair.
    GFSDK_Aftermath_GpuCrashDumpDescriptionKey_UserDefined = 0x00010000,
};

// Function for adding user-defined description key-value pairs.
// Key must be one of the predefined keys of GFSDK_Aftermath_GpuCrashDumpDescriptionKey
// or a user-defined key based on GFSDK_Aftermath_GpuCrashDumpDescriptionKey_UserDefined.
// All keys greater than the last predefined key in GFSDK_Aftermath_GpuCrashDumpDescriptionKey
// and smaller than GFSDK_Aftermath_GpuCrashDumpDescriptionKey_UserDefined are
// considered illegal and ignored.
typedef void (GFSDK_AFTERMATH_CALL *PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription)(uint32_t key, const char* value);

// GPU crash dump callback definitions.
// NOTE: Except for the pUserData pointer, all pointer values passed to the
// callbacks are only valid for the duration of the call! An implementation
// must make copies of the data, if it intends to store it beyond that.
typedef void (GFSDK_AFTERMATH_CALL *PFN_GFSDK_Aftermath_GpuCrashDumpCb)(const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize, void* pUserData);
typedef void (GFSDK_AFTERMATH_CALL *PFN_GFSDK_Aftermath_ShaderDebugInfoCb)(const void* pShaderDebugInfo, const uint32_t shaderDebugInfoSize, void* pUserData);
typedef void (GFSDK_AFTERMATH_CALL *PFN_GFSDK_Aftermath_GpuCrashDumpDescriptionCb)(PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription addValue, void* pUserData);

/////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_EnableGpuCrashDumps
// ---------------------------------
//
// apiVersion;
//      Must be set to GFSDK_Aftermath_Version_API. Used for checking against library
//      version.
//
// watchedApis;
//      Controls which graphics APIs to watch for crashes. A combination (with OR) of
//      GFSDK_Aftermath_GpuCrashDumpWatchedApiFlags.
//
// flags;
//      Controls GPU crash dump specific behavior. A combination (with OR) of
//      GFSDK_Aftermath_GpuCrashDumpFeatureFlags.
//
// gpuCrashDumpCb;
//      Callback function to be called when new GPU crash dump data is available.
//      Callback is free-threaded, ensure the provided function is thread-safe.
//
// shaderDebugInfoCb;
//      Callback function to be called when new shader debug information data is
//      available.
//      Callback is free-threaded, ensure the provided function is thread-safe.
//      Optional, can be NULL.
//      Note: if not using GFSDK_Aftermath_GpuCrashDumpFeatureFlags_DeferDebugInfoCallbacks
//      shaderDebugInfoCb will be invoked for every shader compilation trigegred by the
//      application, even if there will be never an invocation of gpuCrashDumpCb.
//
// descriptionCb;
//      Callback function that allows the application to provide additional
//      descriptive values to be include in crash dumps. This will be called
//      before gpuCrashDumpCb.
//      Callback is free-threaded, ensure the provided function is thread-safe.
//      Optional, can be NULL.
//
// pUserData;
//      User data made available in the callbacks.
//
//// DESCRIPTION;
//      Device independent initialization call to enable Aftermath GPU crash dump
//      creation. Overrides any settings from an also active GPU crash dump monitor
//      for this process! This function must be called before any D3D device is
//      created by the application.
//
/////////////////////////////////////////////////////////////////////////
GFSDK_Aftermath_API GFSDK_Aftermath_EnableGpuCrashDumps(
    GFSDK_Aftermath_Version apiVersion,
    uint32_t watchedApis,
    uint32_t flags,
    PFN_GFSDK_Aftermath_GpuCrashDumpCb gpuCrashDumpCb,
    PFN_GFSDK_Aftermath_ShaderDebugInfoCb shaderDebugInfoCb,
    PFN_GFSDK_Aftermath_GpuCrashDumpDescriptionCb descriptionCb,
    void* pUserData);

/////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_DisableGpuCrashDumps
// ---------------------------------
//
//// DESCRIPTION;
//      Device independent call to disable Aftermath GPU crash dump creation.
//      Re-enables settings from an also active GPU crash dump monitor for
//      the current process!
//
/////////////////////////////////////////////////////////////////////////
GFSDK_Aftermath_API GFSDK_Aftermath_DisableGpuCrashDumps();

/////////////////////////////////////////////////////////////////////////
//
// NOTE: Function table provided - if dynamic loading is preferred.
//
/////////////////////////////////////////////////////////////////////////
GFSDK_Aftermath_PFN(GFSDK_AFTERMATH_CALL *PFN_GFSDK_Aftermath_EnableGpuCrashDumps)(GFSDK_Aftermath_Version apiVersion, uint32_t watchedApis, uint32_t flags, PFN_GFSDK_Aftermath_GpuCrashDumpCb gpuCrashDumpCb, PFN_GFSDK_Aftermath_ShaderDebugInfoCb shaderDebugInfoCb, PFN_GFSDK_Aftermath_GpuCrashDumpDescriptionCb descriptionCb, void* pUserData);
GFSDK_Aftermath_PFN(GFSDK_AFTERMATH_CALL *PFN_GFSDK_Aftermath_DisableGpuCrashDumps)();

#ifdef __cplusplus
} // extern "C"
#endif

#pragma pack(pop)

#endif // GFSDK_Aftermath_GpuCrashDump_H
