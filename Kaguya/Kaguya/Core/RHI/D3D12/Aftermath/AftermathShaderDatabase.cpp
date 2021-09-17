//*********************************************************
//
// Copyright (c) 2019, NVIDIA CORPORATION. All rights reserved.
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
#include "AftermathShaderDatabase.h"

void AftermathShaderDatabase::AddShader(
	Microsoft::WRL::ComPtr<IDxcBlob> Blob,
	Microsoft::WRL::ComPtr<IDxcBlob> PdbBlob) noexcept
{
#ifdef NVIDIA_NSIGHT_AFTERMATH
	// Create shader hashes for the shader bytecode
	D3D12_SHADER_BYTECODE Bytecode = { Blob->GetBufferPointer(), Blob->GetBufferSize() };

	GFSDK_Aftermath_ShaderHash			   ShaderHash			  = {};
	GFSDK_Aftermath_ShaderInstructionsHash ShaderInstructionsHash = {};
	if (GFSDK_Aftermath_SUCCEED(GFSDK_Aftermath_GetShaderHash(
			GFSDK_Aftermath_Version_API,
			&Bytecode,
			&ShaderHash,
			&ShaderInstructionsHash)))
	{
		// Store the data for shader instruction address mapping when decoding GPU crash dumps.
		// cf. FindShaderBinary()
		ShaderBinaries[ShaderHash].Swap(Blob);
		ShaderInstructionsToShaderHash[ShaderInstructionsHash] = ShaderHash;

		// Populate shader debug name.
		// The shaders used in this sample are compiled with compiler-generated debug data
		// file names. That means the debug data file's name matches the corresponding
		// shader's DebugName.
		// If shaders are built with user-defined debug data file names, the shader database
		// must maintain a mapping between the shader DebugName (queried from the shader
		// binary with GFSDK_Aftermath_GetShaderDebugName()) and the name of the file
		// containing the corresponding debug data.
		// Please see the documentation of GFSDK_Aftermath_GpuCrashDump_GenerateJSON() for
		// additional information.
		GFSDK_Aftermath_ShaderDebugName ShaderDebugName;
		if (GFSDK_Aftermath_SUCCEED(
				GFSDK_Aftermath_GetShaderDebugName(GFSDK_Aftermath_Version_API, &Bytecode, &ShaderDebugName)))
		{
			// Store the data for shader instruction address mapping when decoding GPU crash dumps.
			// cf. FindSourceShaderDebugData()
			ShaderPdbs[ShaderDebugName].Swap(PdbBlob);
		}
	}
#else
	UNREFERENCED_PARAMETER(Blob);
	UNREFERENCED_PARAMETER(PdbBlob);
#endif
}

IDxcBlob* AftermathShaderDatabase::FindShaderBinary(const GFSDK_Aftermath_ShaderHash& ShaderHash)
{
	if (auto iter = ShaderBinaries.find(ShaderHash); iter != ShaderBinaries.end())
	{
		return iter->second.Get();
	}

	return nullptr;
}

IDxcBlob* AftermathShaderDatabase::FindShaderBinary(
	const GFSDK_Aftermath_ShaderInstructionsHash& ShaderInstructionsHash)
{
	if (auto iter = ShaderInstructionsToShaderHash.find(ShaderInstructionsHash);
		iter != ShaderInstructionsToShaderHash.end())
	{
		return FindShaderBinary(iter->second);
	}

	return nullptr;
}

IDxcBlob* AftermathShaderDatabase::FindSourceShaderDebugData(const GFSDK_Aftermath_ShaderDebugName& ShaderDebugName)
{
	if (auto iter = ShaderPdbs.find(ShaderDebugName); iter != ShaderPdbs.end())
	{
		return iter->second.Get();
	}

	return nullptr;
}
