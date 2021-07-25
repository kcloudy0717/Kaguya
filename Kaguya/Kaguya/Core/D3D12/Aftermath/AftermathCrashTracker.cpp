//*********************************************************
//
// Copyright (c) 2019-2020, NVIDIA CORPORATION. All rights reserved.
//
//  Permission is hereby granted, free of charge, to any person obtaining a
//  copy of this software and associated documentation files (the "Software"),
//  to deal in the Software without restriction, including without limitation
//  the rights to use, copy, modify, merge, publish, distribute, sublicense,
//  and/or sell copies of the Software, and to permit persons to whom the
//  Software is furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
//  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//  DEALINGS IN THE SOFTWARE.
//
//*********************************************************
#include "AftermathCrashTracker.h"
#include <fstream>

AftermathCrashTracker::AftermathCrashTracker()
	: Initialized(false)
{
}

AftermathCrashTracker::~AftermathCrashTracker()
{
	if (Initialized)
	{
		GFSDK_Aftermath_DisableGpuCrashDumps();
	}
}

void AftermathCrashTracker::Initialize()
{
	// Enable GPU crash dumps and set up the callbacks for crash dump notifications,
	// shader debug information notifications, and providing additional crash
	// dump description data.Only the crash dump callback is mandatory. The other two
	// callbacks are optional and can be omitted, by passing nullptr, if the corresponding
	// functionality is not used.
	// The DeferDebugInfoCallbacks flag enables caching of shader debug information data
	// in memory. If the flag is set, ShaderDebugInfoCallback will be called only
	// in the event of a crash, right before GpuCrashDumpCallback. If the flag is not set,
	// ShaderDebugInfoCallback will be called for every shader that is compiled.
	if (GFSDK_Aftermath_SUCCEED(GFSDK_Aftermath_EnableGpuCrashDumps(
			GFSDK_Aftermath_Version_API,
			GFSDK_Aftermath_GpuCrashDumpWatchedApiFlags_DX,
			GFSDK_Aftermath_GpuCrashDumpFeatureFlags_DeferDebugInfoCallbacks,
			CrashDumpCallback,
			ShaderDebugInfoCallback,
			CrashDumpDescriptionCallback,
			this)))
	{
		Initialized = true;
	}
}

void AftermathCrashTracker::RegisterDevice(ID3D12Device* pDevice)
{
	// Initialize Nsight Aftermath for this device.
	//
	// * EnableMarkers - this will include information about the Aftermath
	//   event marker nearest to the crash.
	//
	//   Using event markers should be considered carefully as they can cause
	//   considerable CPU overhead when used in high frequency code paths.
	//
	// * EnableResourceTracking - this will include additional information about the
	//   resource related to a GPU virtual address seen in case of a crash due to a GPU
	//   page fault. This includes, for example, information about the size of the
	//   resource, its format, and an indication if the resource has been deleted.
	//
	// * CallStackCapturing - this will include call stack and module information for
	//   the draw call, compute dispatch, or resource copy nearest to the crash.
	//
	//   Using this option should be considered carefully. Enabling call stack
	//   capturing will cause very high CPU overhead.
	//
	// * GenerateShaderDebugInfo - this instructs the shader compiler to
	//   generate debug information (line tables) for all shaders. Using this option
	//   should be considered carefully. It may cause considerable shader compilation
	//   overhead and additional overhead for handling the corresponding shader debug
	//   information callbacks.
	//
	if (Initialized)
	{
		constexpr uint32_t AftermathFlags =
			GFSDK_Aftermath_FeatureFlags_EnableMarkers |		  // Enable event marker tracking.
			GFSDK_Aftermath_FeatureFlags_EnableResourceTracking | // Enable tracking of resources.
			GFSDK_Aftermath_FeatureFlags_CallStackCapturing |	  // Capture call stacks for all draw calls, compute
																  // dispatches, and resource copies.
			GFSDK_Aftermath_FeatureFlags_GenerateShaderDebugInfo; // Generate debug information for shaders.

		AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_DX12_Initialize(GFSDK_Aftermath_Version_API, AftermathFlags, pDevice));
	}
}

void AftermathCrashTracker::CrashDumpCallback(const void* pCrashDump, const uint32_t SizeInBytes, void* pUserData)
{
	auto pAftermathCrashTracker = reinterpret_cast<AftermathCrashTracker*>(pUserData);
	pAftermathCrashTracker->OnCrashDump(pCrashDump, SizeInBytes);
}

void AftermathCrashTracker::ShaderDebugInfoCallback(
	const void*	   pShaderDebugInfo,
	const uint32_t SizeInBytes,
	void*		   pUserData)
{
	auto pAftermathCrashTracker = reinterpret_cast<AftermathCrashTracker*>(pUserData);
	pAftermathCrashTracker->OnShaderDebugInfo(pShaderDebugInfo, SizeInBytes);
}

void AftermathCrashTracker::CrashDumpDescriptionCallback(
	PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription PFNAddGpuCrashDumpDescription,
	void*										   pUserData)
{
	auto pAftermathCrashTracker = reinterpret_cast<AftermathCrashTracker*>(pUserData);
	pAftermathCrashTracker->OnDescription(PFNAddGpuCrashDumpDescription);
}

void AftermathCrashTracker::JSONShaderDebugInfoLookupCallback(
	const GFSDK_Aftermath_ShaderDebugInfoIdentifier* pShaderDebugInfoIdentifier,
	PFN_GFSDK_Aftermath_SetData						 PFNSetData,
	void*											 pUserData)
{
	// Handler for shader debug information lookup callbacks.
	// This is used by the JSON decoder for mapping shader instruction
	// addresses to DXIL lines or HLSl source lines.
	if (auto iter = ShaderDebugInfoMap.find(*pShaderDebugInfoIdentifier); iter != ShaderDebugInfoMap.end())
	{
		PFNSetData(iter->second.data(), uint32_t(iter->second.size()));
	}
}

void AftermathCrashTracker::JSONShaderHashLookupCallback(
	const GFSDK_Aftermath_ShaderHash* pShaderHash,
	PFN_GFSDK_Aftermath_SetData		  PFNSetData,
	void*							  pUserData)
{
	// Handler for shader lookup callbacks.
	// This is used by the JSON decoder for mapping shader instruction
	// addresses to DXIL lines or HLSL source lines.
	// NOTE: If the application loads stripped shader binaries (-Qstrip_debug),
	// Aftermath will require access to both the stripped and the not stripped
	// shader binaries.
	IDxcBlob* Blob = AftermathShaderDatabase::FindShaderBinary(*pShaderHash);
	if (Blob)
	{
		PFNSetData(Blob->GetBufferPointer(), static_cast<uint32_t>(Blob->GetBufferSize()));
	}
}

// Static callback wrapper for OnShaderInstructionsLookup
void AftermathCrashTracker::JSONShaderInstructionsHashLookupCallback(
	const GFSDK_Aftermath_ShaderInstructionsHash* pShaderInstructionsHash,
	PFN_GFSDK_Aftermath_SetData					  PFNSetData,
	void*										  pUserData)
{
	// Handler for shader instructions lookup callbacks.
	// This is used by the JSON decoder for mapping shader instruction
	// addresses to DXIL lines or HLSL source lines.
	// NOTE: If the application loads stripped shader binaries (-Qstrip_debug),
	// Aftermath will require access to both the stripped and the not stripped
	// shader binaries.
	IDxcBlob* Blob = AftermathShaderDatabase::FindShaderBinary(*pShaderInstructionsHash);
	if (Blob)
	{
		PFNSetData(Blob->GetBufferPointer(), static_cast<uint32_t>(Blob->GetBufferSize()));
	}
}

// Static callback wrapper for OnShaderSourceDebugInfoLookup
void AftermathCrashTracker::JSONShaderDebugNameLookupCallback(
	const GFSDK_Aftermath_ShaderDebugName* pShaderDebugName,
	PFN_GFSDK_Aftermath_SetData			   PFNSetData,
	void*								   pUserData)
{
	// Handler for shader source debug info lookup callbacks.
	// This is used by the JSON decoder for mapping shader instruction addresses to
	// HLSL source lines, if the shaders used by the application were compiled with
	// separate debug info data files.
	IDxcBlob* Blob = AftermathShaderDatabase::FindSourceShaderDebugData(*pShaderDebugName);
	if (Blob)
	{
		PFNSetData(Blob->GetBufferPointer(), static_cast<uint32_t>(Blob->GetBufferSize()));
	}
}

// Handler for GPU crash dump callbacks from Nsight Aftermath
void AftermathCrashTracker::OnCrashDump(const void* pCrashDump, const uint32_t SizeInBytes)
{
	std::lock_guard _(Mutex);

	// Write to file for later in-depth analysis with Nsight Graphics.
	WriteCrashDumpToFile(pCrashDump, SizeInBytes);
}

// Handler for shader debug information callbacks
void AftermathCrashTracker::OnShaderDebugInfo(const void* pShaderDebugInfo, const uint32_t SizeInBytes)
{
	std::lock_guard _(Mutex);

	// Get shader debug information identifier
	GFSDK_Aftermath_ShaderDebugInfoIdentifier ShaderDebugInfoIdentifier = {};
	AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GetShaderDebugInfoIdentifier(
		GFSDK_Aftermath_Version_API,
		pShaderDebugInfo,
		SizeInBytes,
		&ShaderDebugInfoIdentifier));

	// Store information for decoding of GPU crash dumps with shader address mapping
	// from within the application.
	std::vector<uint8_t> data((uint8_t*)pShaderDebugInfo, (uint8_t*)pShaderDebugInfo + SizeInBytes);
	ShaderDebugInfoMap[ShaderDebugInfoIdentifier].swap(data);

	// Write to file for later in-depth analysis of crash dumps with Nsight Graphics
	WriteShaderDebugInformationToFile(ShaderDebugInfoIdentifier, pShaderDebugInfo, SizeInBytes);
}

// Handler for GPU crash dump description callbacks
void AftermathCrashTracker::OnDescription(PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription AddGpuCrashDumpDescription)
{
	// Add some basic description about the crash. This is called after the GPU crash happens, but before
	// the actual GPU crash dump callback. The provided data is included in the crash dump and can be
	// retrieved using GFSDK_Aftermath_GpuCrashDump_GetDescription().
	AddGpuCrashDumpDescription(GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationName, "Kaguya");
	AddGpuCrashDumpDescription(GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationVersion, "v1.0");
}

// Helper for writing a GPU crash dump to a file
void AftermathCrashTracker::WriteCrashDumpToFile(const void* pCrashDump, const uint32_t SizeInBytes)
{
	// Create a GPU crash dump decoder object for the GPU crash dump.
	GFSDK_Aftermath_GpuCrashDump_Decoder Decoder = {};
	AFTERMATH_CHECK_ERROR(
		GFSDK_Aftermath_GpuCrashDump_CreateDecoder(GFSDK_Aftermath_Version_API, pCrashDump, SizeInBytes, &Decoder));

	// Use the decoder object to read basic information, like application
	// name, PID, etc. from the GPU crash dump.
	GFSDK_Aftermath_GpuCrashDump_BaseInfo BaseInfo = {};
	AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GpuCrashDump_GetBaseInfo(Decoder, &BaseInfo));

	// Use the decoder object to query the application name that was set
	// in the GPU crash dump description.
	uint32_t ApplicationNameDescriptionSize = 0;
	AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GpuCrashDump_GetDescriptionSize(
		Decoder,
		GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationName,
		&ApplicationNameDescriptionSize));

	std::string ApplicationName(ApplicationNameDescriptionSize, '\0');

	AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GpuCrashDump_GetDescription(
		Decoder,
		GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationName,
		uint32_t(ApplicationName.size()),
		ApplicationName.data()));

	// Create a unique file name for writing the crash dump data to a file.
	// Note: due to an Nsight Aftermath bug (will be fixed in an upcoming
	// driver release) we may see redundant crash dumps. As a workaround,
	// attach a unique count to each generated file name.
	static int		  count = 0;
	const std::string BaseFileName =
		ApplicationName + "-" + std::to_string(BaseInfo.pid) + "-" + std::to_string(++count);

	// Write the the crash dump data to a file using the .nv-gpudmp extension
	// registered with Nsight Graphics.
	const std::string CrashDumpFileName = BaseFileName + ".nv-gpudmp";
	std::ofstream	  CrashDump(CrashDumpFileName, std::ios::out | std::ios::binary);
	if (CrashDump)
	{
		CrashDump.write((const char*)pCrashDump, SizeInBytes);
		CrashDump.close();
	}

	// Decode the crash dump to a JSON string.
	// Step 1: Generate the JSON and get the size.
	uint32_t JsonSize = 0;
	AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GpuCrashDump_GenerateJSON(
		Decoder,
		GFSDK_Aftermath_GpuCrashDumpDecoderFlags_ALL_INFO,
		GFSDK_Aftermath_GpuCrashDumpFormatterFlags_NONE,
		JSONShaderDebugInfoLookupCallback,
		JSONShaderHashLookupCallback,
		JSONShaderInstructionsHashLookupCallback,
		JSONShaderDebugNameLookupCallback,
		this,
		&JsonSize));
	// Step 2: Allocate a buffer and fetch the generated JSON.
	std::vector<char> Json(JsonSize);
	AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GpuCrashDump_GetJSON(Decoder, uint32_t(Json.size()), Json.data()));

	// Write the the crash dump data as JSON to a file.
	const std::string JsonDumpFileName = BaseFileName + ".json";
	std::ofstream	  JsonDump(JsonDumpFileName, std::ios::out | std::ios::binary);
	if (JsonDump)
	{
		JsonDump.write(Json.data(), Json.size());
		JsonDump.close();
	}

	// Destroy the GPU crash dump decoder object.
	AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GpuCrashDump_DestroyDecoder(Decoder));
}

// Helper for writing shader debug information to a file
void AftermathCrashTracker::WriteShaderDebugInformationToFile(
	GFSDK_Aftermath_ShaderDebugInfoIdentifier ShaderDebugInfoIdentifier,
	const void*								  pShaderDebugInfo,
	const uint32_t							  SizeInBytes)
{
	// Create a unique file name.
	const std::string filePath = "shader-" + std::to_string(ShaderDebugInfoIdentifier) + ".nvdbg";

	std::ofstream f(filePath, std::ios::out | std::ios::binary);
	if (f)
	{
		f.write((const char*)pShaderDebugInfo, SizeInBytes);
		f.close();
	}
}
