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

#pragma once
#include "AftermathUtility.h"
#include "AftermathShaderDatabase.h"

class AftermathCrashTracker
{
public:
	AftermathCrashTracker();
	~AftermathCrashTracker();

	// Initialize AftermathCrashTracker
	void Initialize();

	// Initialize Nsight Aftermath for this device
	void RegisterDevice(ID3D12Device* pDevice);

private:
	static void CrashDumpCallback(const void* pCrashDump, const uint32_t SizeInBytes, void* pUserData);

	static void ShaderDebugInfoCallback(const void* pShaderDebugInfo, const uint32_t SizeInBytes, void* pUserData);

	static void CrashDumpDescriptionCallback(
		PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription PFNAddGpuCrashDumpDescription,
		void*										   pUserData);

	static void JSONShaderDebugInfoLookupCallback(
		const GFSDK_Aftermath_ShaderDebugInfoIdentifier* pShaderDebugInfoIdentifier,
		PFN_GFSDK_Aftermath_SetData						 PFNSetData,
		void*											 pUserData);

	static void JSONShaderHashLookupCallback(
		const GFSDK_Aftermath_ShaderHash* pShaderHash,
		PFN_GFSDK_Aftermath_SetData		  PFNSetData,
		void*							  pUserData);

	static void JSONShaderInstructionsHashLookupCallback(
		const GFSDK_Aftermath_ShaderInstructionsHash* pShaderInstructionsHash,
		PFN_GFSDK_Aftermath_SetData					  PFNSetData,
		void*										  pUserData);

	static void JSONShaderDebugNameLookupCallback(
		const GFSDK_Aftermath_ShaderDebugName* pShaderDebugName,
		PFN_GFSDK_Aftermath_SetData			   PFNSetData,
		void*								   pUserData);

	void OnCrashDump(const void* pGpuCrashDump, uint32_t SizeInBytes);

	void OnShaderDebugInfo(const void* pShaderDebugInfo, const uint32_t SizeInBytes);

	void OnDescription(PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription AddGpuCrashDumpDescription);

	void WriteCrashDumpToFile(const void* pCrashDump, const uint32_t SizeInBytes);

	void WriteShaderDebugInformationToFile(
		GFSDK_Aftermath_ShaderDebugInfoIdentifier ShaderDebugInfoIdentifier,
		const void*								  pShaderDebugInfo,
		const uint32_t							  SizeInBytes);

private:
	bool Initialized;

	inline static std::map<GFSDK_Aftermath_ShaderDebugInfoIdentifier, std::vector<uint8_t>> ShaderDebugInfoMap;

	mutable std::mutex Mutex;
};
