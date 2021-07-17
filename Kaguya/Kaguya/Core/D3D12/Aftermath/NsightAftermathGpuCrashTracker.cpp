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
#include "NsightAftermathGpuCrashTracker.h"
#include <fstream>
#include <iomanip>
#include <string>

AftermathCrashTracker::AftermathCrashTracker()
	: Initialized(false)
{
}

AftermathCrashTracker::~AftermathCrashTracker()
{
	// If initialized, disable GPU crash dumps
	if (Initialized)
	{
		GFSDK_Aftermath_DisableGpuCrashDumps();
	}
}

// Initialize the GPU Crash Dump Tracker
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
	GFSDK_Aftermath_Result result = GFSDK_Aftermath_EnableGpuCrashDumps(
		GFSDK_Aftermath_Version_API,
		GFSDK_Aftermath_GpuCrashDumpWatchedApiFlags_DX,
		GFSDK_Aftermath_GpuCrashDumpFeatureFlags_DeferDebugInfoCallbacks, // Let the Nsight Aftermath library cache
																		  // shader debug information.
		GpuCrashDumpCallback,											  // Register callback for GPU crash dumps.
		ShaderDebugInfoCallback,	  // Register callback for shader debug information.
		CrashDumpDescriptionCallback, // Register callback for GPU crash dump description.
		this);						  // Set the GpuCrashTracker object as user data for the above callbacks.

	Initialized = true;
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
	constexpr uint32_t AftermathFlags =
		GFSDK_Aftermath_FeatureFlags_EnableMarkers |		  // Enable event marker tracking.
		GFSDK_Aftermath_FeatureFlags_EnableResourceTracking | // Enable tracking of resources.
		GFSDK_Aftermath_FeatureFlags_CallStackCapturing | // Capture call stacks for all draw calls, compute dispatches,
														  // and resource copies.
		GFSDK_Aftermath_FeatureFlags_GenerateShaderDebugInfo; // Generate debug information for shaders.

	GFSDK_Aftermath_Result result =
		GFSDK_Aftermath_DX12_Initialize(GFSDK_Aftermath_Version_API, AftermathFlags, pDevice);
}

// Handler for GPU crash dump callbacks from Nsight Aftermath
void AftermathCrashTracker::OnCrashDump(const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize)
{
	std::lock_guard _(Mutex);

	// Write to file for later in-depth analysis with Nsight Graphics.
	WriteGpuCrashDumpToFile(pGpuCrashDump, gpuCrashDumpSize);
}

// Handler for shader debug information callbacks
void AftermathCrashTracker::OnShaderDebugInfo(const void* pShaderDebugInfo, const uint32_t shaderDebugInfoSize)
{
	std::lock_guard _(Mutex);

	// Get shader debug information identifier
	GFSDK_Aftermath_ShaderDebugInfoIdentifier identifier = {};
	AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GetShaderDebugInfoIdentifier(
		GFSDK_Aftermath_Version_API,
		pShaderDebugInfo,
		shaderDebugInfoSize,
		&identifier));

	// Store information for decoding of GPU crash dumps with shader address mapping
	// from within the application.
	std::vector<uint8_t> data((uint8_t*)pShaderDebugInfo, (uint8_t*)pShaderDebugInfo + shaderDebugInfoSize);
	ShaderDebugInfo[identifier].swap(data);

	// Write to file for later in-depth analysis of crash dumps with Nsight Graphics
	WriteShaderDebugInformationToFile(identifier, pShaderDebugInfo, shaderDebugInfoSize);
}

// Handler for GPU crash dump description callbacks
void AftermathCrashTracker::OnDescription(PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription addDescription)
{
	// Add some basic description about the crash. This is called after the GPU crash happens, but before
	// the actual GPU crash dump callback. The provided data is included in the crash dump and can be
	// retrieved using GFSDK_Aftermath_GpuCrashDump_GetDescription().
	addDescription(GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationName, "D3D12HelloNsightAftermath");
	addDescription(GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationVersion, "v1.0");
	addDescription(GFSDK_Aftermath_GpuCrashDumpDescriptionKey_UserDefined, "This is a GPU crash dump example.");
	addDescription(GFSDK_Aftermath_GpuCrashDumpDescriptionKey_UserDefined + 1, "Engine State: Rendering.");
	addDescription(GFSDK_Aftermath_GpuCrashDumpDescriptionKey_UserDefined + 2, "More user-defined information...");
}

// Helper for writing a GPU crash dump to a file
void AftermathCrashTracker::WriteGpuCrashDumpToFile(const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize)
{
	// Create a GPU crash dump decoder object for the GPU crash dump.
	GFSDK_Aftermath_GpuCrashDump_Decoder decoder = {};
	AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GpuCrashDump_CreateDecoder(
		GFSDK_Aftermath_Version_API,
		pGpuCrashDump,
		gpuCrashDumpSize,
		&decoder));

	// Use the decoder object to read basic information, like application
	// name, PID, etc. from the GPU crash dump.
	GFSDK_Aftermath_GpuCrashDump_BaseInfo baseInfo = {};
	AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GpuCrashDump_GetBaseInfo(decoder, &baseInfo));

	// Use the decoder object to query the application name that was set
	// in the GPU crash dump description.
	uint32_t applicationNameLength = 0;
	AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GpuCrashDump_GetDescriptionSize(
		decoder,
		GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationName,
		&applicationNameLength));

	std::vector<char> applicationName(applicationNameLength, '\0');

	AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GpuCrashDump_GetDescription(
		decoder,
		GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationName,
		uint32_t(applicationName.size()),
		applicationName.data()));

	// Create a unique file name for writing the crash dump data to a file.
	// Note: due to an Nsight Aftermath bug (will be fixed in an upcoming
	// driver release) we may see redundant crash dumps. As a workaround,
	// attach a unique count to each generated file name.
	static int		  count = 0;
	const std::string baseFileName =
		std::string(applicationName.data()) + "-" + std::to_string(baseInfo.pid) + "-" + std::to_string(++count);

	// Write the the crash dump data to a file using the .nv-gpudmp extension
	// registered with Nsight Graphics.
	const std::string crashDumpFileName = baseFileName + ".nv-gpudmp";
	std::ofstream	  dumpFile(crashDumpFileName, std::ios::out | std::ios::binary);
	if (dumpFile)
	{
		dumpFile.write((const char*)pGpuCrashDump, gpuCrashDumpSize);
		dumpFile.close();
	}

	// Decode the crash dump to a JSON string.
	// Step 1: Generate the JSON and get the size.
	uint32_t jsonSize = 0;
	AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GpuCrashDump_GenerateJSON(
		decoder,
		GFSDK_Aftermath_GpuCrashDumpDecoderFlags_ALL_INFO,
		GFSDK_Aftermath_GpuCrashDumpFormatterFlags_NONE,
		ShaderDebugInfoLookupCallback,
		ShaderLookupCallback,
		ShaderInstructionsLookupCallback,
		ShaderSourceDebugInfoLookupCallback,
		this,
		&jsonSize));
	// Step 2: Allocate a buffer and fetch the generated JSON.
	std::vector<char> json(jsonSize);
	AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GpuCrashDump_GetJSON(decoder, uint32_t(json.size()), json.data()));

	// Write the the crash dump data as JSON to a file.
	const std::string jsonFileName = crashDumpFileName + ".json";
	std::ofstream	  jsonFile(jsonFileName, std::ios::out | std::ios::binary);
	if (jsonFile)
	{
		jsonFile.write(json.data(), json.size());
		jsonFile.close();
	}

	// Destroy the GPU crash dump decoder object.
	AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GpuCrashDump_DestroyDecoder(decoder));
}

// Helper for writing shader debug information to a file
void AftermathCrashTracker::WriteShaderDebugInformationToFile(
	GFSDK_Aftermath_ShaderDebugInfoIdentifier identifier,
	const void*								  pShaderDebugInfo,
	const uint32_t							  shaderDebugInfoSize)
{
	// Create a unique file name.
	const std::string filePath = "shader-" + std::to_string(identifier) + ".nvdbg";

	std::ofstream f(filePath, std::ios::out | std::ios::binary);
	if (f)
	{
		f.write((const char*)pShaderDebugInfo, shaderDebugInfoSize);
	}
}

// Handler for shader debug information lookup callbacks.
// This is used by the JSON decoder for mapping shader instruction
// addresses to DXIL lines or HLSl source lines.
void AftermathCrashTracker::OnShaderDebugInfoLookup(
	const GFSDK_Aftermath_ShaderDebugInfoIdentifier& identifier,
	PFN_GFSDK_Aftermath_SetData						 setShaderDebugInfo) const
{
	// Search the list of shader debug information blobs received earlier.
	auto i_debugInfo = ShaderDebugInfo.find(identifier);
	if (i_debugInfo == ShaderDebugInfo.end())
	{
		// Early exit, nothing found. No need to call setShaderDebugInfo.
		return;
	}

	// Let the GPU crash dump decoder know about the shader debug information
	// that was found.
	setShaderDebugInfo(i_debugInfo->second.data(), uint32_t(i_debugInfo->second.size()));
}

// Handler for shader lookup callbacks.
// This is used by the JSON decoder for mapping shader instruction
// addresses to DXIL lines or HLSL source lines.
// NOTE: If the application loads stripped shader binaries (-Qstrip_debug),
// Aftermath will require access to both the stripped and the not stripped
// shader binaries.
void AftermathCrashTracker::OnShaderLookup(
	const GFSDK_Aftermath_ShaderHash& ShaderHash,
	PFN_GFSDK_Aftermath_SetData		  PFNSetData) const
{
	// Find shader binary data for the shader hash in the shader database.
	IDxcBlob* Blob = ShaderDatabase.FindShaderBinary(ShaderHash);
	if (Blob)
	{
		// Let the GPU crash dump decoder know about the shader data
		// that was found.
		PFNSetData(Blob->GetBufferPointer(), Blob->GetBufferSize());
	}
}

// Handler for shader instructions lookup callbacks.
// This is used by the JSON decoder for mapping shader instruction
// addresses to DXIL lines or HLSL source lines.
// NOTE: If the application loads stripped shader binaries (-Qstrip_debug),
// Aftermath will require access to both the stripped and the not stripped
// shader binaries.
void AftermathCrashTracker::OnShaderInstructionsLookup(
	const GFSDK_Aftermath_ShaderInstructionsHash& ShaderInstructionsHash,
	PFN_GFSDK_Aftermath_SetData					  PFNSetData) const
{
	// Find shader binary data for the shader instruction hash in the shader database.
	IDxcBlob* Blob = ShaderDatabase.FindShaderBinary(ShaderInstructionsHash);
	if (Blob)
	{
		// Let the GPU crash dump decoder know about the shader data
		// that was found.
		PFNSetData(Blob->GetBufferPointer(), Blob->GetBufferSize());
	}
}

// Handler for shader source debug info lookup callbacks.
// This is used by the JSON decoder for mapping shader instruction addresses to
// HLSL source lines, if the shaders used by the application were compiled with
// separate debug info data files.
void AftermathCrashTracker::OnShaderSourceDebugInfoLookup(
	const GFSDK_Aftermath_ShaderDebugName& ShaderDebugName,
	PFN_GFSDK_Aftermath_SetData			   PFNSetData) const
{
	// Find source debug info for the shader DebugName in the shader database.
	IDxcBlob* Blob = ShaderDatabase.FindSourceShaderDebugData(ShaderDebugName);
	if (Blob)
	{
		// Let the GPU crash dump decoder know about the shader debug data that was
		// found.
		PFNSetData(Blob->GetBufferPointer(), Blob->GetBufferSize());
	}
}

// Static callback wrapper for OnCrashDump
void AftermathCrashTracker::GpuCrashDumpCallback(
	const void*	   pGpuCrashDump,
	const uint32_t gpuCrashDumpSize,
	void*		   pUserData)
{
	auto pGpuCrashTracker = reinterpret_cast<AftermathCrashTracker*>(pUserData);
	pGpuCrashTracker->OnCrashDump(pGpuCrashDump, gpuCrashDumpSize);
}

// Static callback wrapper for OnShaderDebugInfo
void AftermathCrashTracker::ShaderDebugInfoCallback(
	const void*	   pShaderDebugInfo,
	const uint32_t shaderDebugInfoSize,
	void*		   pUserData)
{
	auto pGpuCrashTracker = reinterpret_cast<AftermathCrashTracker*>(pUserData);
	pGpuCrashTracker->OnShaderDebugInfo(pShaderDebugInfo, shaderDebugInfoSize);
}

// Static callback wrapper for OnDescription
void AftermathCrashTracker::CrashDumpDescriptionCallback(
	PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription addDescription,
	void*										   pUserData)
{
	auto pGpuCrashTracker = reinterpret_cast<AftermathCrashTracker*>(pUserData);
	pGpuCrashTracker->OnDescription(addDescription);
}

// Static callback wrapper for OnShaderDebugInfoLookup
void AftermathCrashTracker::ShaderDebugInfoLookupCallback(
	const GFSDK_Aftermath_ShaderDebugInfoIdentifier* pIdentifier,
	PFN_GFSDK_Aftermath_SetData						 setShaderDebugInfo,
	void*											 pUserData)
{
	auto pGpuCrashTracker = reinterpret_cast<AftermathCrashTracker*>(pUserData);
	pGpuCrashTracker->OnShaderDebugInfoLookup(*pIdentifier, setShaderDebugInfo);
}

// Static callback wrapper for OnShaderLookup
void AftermathCrashTracker::ShaderLookupCallback(
	const GFSDK_Aftermath_ShaderHash* pShaderHash,
	PFN_GFSDK_Aftermath_SetData		  setShaderBinary,
	void*							  pUserData)
{
	auto pGpuCrashTracker = reinterpret_cast<AftermathCrashTracker*>(pUserData);
	pGpuCrashTracker->OnShaderLookup(*pShaderHash, setShaderBinary);
}

// Static callback wrapper for OnShaderInstructionsLookup
void AftermathCrashTracker::ShaderInstructionsLookupCallback(
	const GFSDK_Aftermath_ShaderInstructionsHash* pShaderInstructionsHash,
	PFN_GFSDK_Aftermath_SetData					  setShaderBinary,
	void*										  pUserData)
{
	auto pGpuCrashTracker = reinterpret_cast<AftermathCrashTracker*>(pUserData);
	pGpuCrashTracker->OnShaderInstructionsLookup(*pShaderInstructionsHash, setShaderBinary);
}

// Static callback wrapper for OnShaderSourceDebugInfoLookup
void AftermathCrashTracker::ShaderSourceDebugInfoLookupCallback(
	const GFSDK_Aftermath_ShaderDebugName* pShaderDebugName,
	PFN_GFSDK_Aftermath_SetData			   setShaderBinary,
	void*								   pUserData)
{
	auto pGpuCrashTracker = reinterpret_cast<AftermathCrashTracker*>(pUserData);
	pGpuCrashTracker->OnShaderSourceDebugInfoLookup(*pShaderDebugName, setShaderBinary);
}
