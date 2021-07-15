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
#include "NsightAftermathShaderDatabase.h"

NsightAftermathShaderDatabase::NsightAftermathShaderDatabase()
{
}

NsightAftermathShaderDatabase::~NsightAftermathShaderDatabase()
{
}

void NsightAftermathShaderDatabase::AddShader(
	Microsoft::WRL::ComPtr<IDxcBlob> Blob,
	Microsoft::WRL::ComPtr<IDxcBlob> PDBBlob)
{
	// Create shader hashes for the shader bytecode
	D3D12_SHADER_BYTECODE shader = { Blob->GetBufferPointer(), Blob->GetBufferSize() };

	GFSDK_Aftermath_ShaderHash			   shaderHash;
	GFSDK_Aftermath_ShaderInstructionsHash shaderInstructionsHash;
	AFTERMATH_CHECK_ERROR(
		GFSDK_Aftermath_GetShaderHash(GFSDK_Aftermath_Version_API, &shader, &shaderHash, &shaderInstructionsHash));

	// Store the data for shader instruction address mapping when decoding GPU crash dumps.
	// cf. FindShaderBinary()
	m_shaderBinaries[shaderHash].Swap(Blob);
	m_shaderInstructionsToShaderHash[shaderInstructionsHash] = shaderHash;

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
	GFSDK_Aftermath_ShaderDebugName debugName;
	GFSDK_Aftermath_GetShaderDebugName(GFSDK_Aftermath_Version_API, &shader, &debugName);

	// Store the data for shader instruction address mapping when decoding GPU crash dumps.
	// cf. FindSourceShaderDebugData()
	m_sourceShaderDebugData[debugName].Swap(PDBBlob);
}

IDxcBlob* NsightAftermathShaderDatabase::FindShaderBinary(const GFSDK_Aftermath_ShaderHash& ShaderHash) const
{
	// Find shader binary data for the shader hash
	if (auto iter = m_shaderBinaries.find(ShaderHash); iter != m_shaderBinaries.end())
	{
		// Nothing found.
		return iter->second.Get();
	}

	return nullptr;
}

IDxcBlob* NsightAftermathShaderDatabase::FindShaderBinary(
	const GFSDK_Aftermath_ShaderInstructionsHash& ShaderInstructionsHash) const
{
	// First, find shader hash corresponding to shader instruction hash.
	if (auto iter = m_shaderInstructionsToShaderHash.find(ShaderInstructionsHash);
		iter != m_shaderInstructionsToShaderHash.end())
	{
		// Find shader binary data
		const GFSDK_Aftermath_ShaderHash& shaderHash = iter->second;
		return FindShaderBinary(shaderHash);
	}

	return nullptr;
}

IDxcBlob* NsightAftermathShaderDatabase::FindSourceShaderDebugData(
	const GFSDK_Aftermath_ShaderDebugName& ShaderDebugName) const
{
	// Find shader debug data for the shader debug name.
	if (auto iter = m_sourceShaderDebugData.find(ShaderDebugName); iter != m_sourceShaderDebugData.end())
	{
		return iter->second.Get();
	}

	return nullptr;
}
