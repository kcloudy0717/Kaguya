#pragma once
#include "RenderDevice.h"

struct Shaders
{
	static constexpr LPCWSTR g_VSEntryPoint = L"VSMain";
	static constexpr LPCWSTR g_MSEntryPoint = L"MSMain";
	static constexpr LPCWSTR g_PSEntryPoint = L"PSMain";
	static constexpr LPCWSTR g_CSEntryPoint = L"CSMain";

	// Vertex Shaders
	struct VS
	{
		inline static Shader FullScreenTriangle;
	};

	// Mesh Shaders
	struct MS
	{
	};

	// Pixel Shaders
	struct PS
	{
		inline static Shader ToneMap;
	};

	// Compute Shaders
	struct CS
	{
	};

	static void Compile(const ShaderCompiler& ShaderCompiler)
	{
		const auto& ExecutableFolderPath = Application::ExecutableDirectory;

		// Load VS
		{
			VS::FullScreenTriangle = ShaderCompiler.CompileShader(
				EShaderType::Vertex,
				ExecutableFolderPath / L"Shaders/FullScreenTriangle.hlsl",
				g_VSEntryPoint,
				{});
		}

		// Load PS
		{
			PS::ToneMap = ShaderCompiler.CompileShader(
				EShaderType::Pixel,
				ExecutableFolderPath / L"Shaders/PostProcess/ToneMap.hlsl",
				g_PSEntryPoint,
				{});
		}
	}
};

struct Libraries
{
	inline static Library PathTrace;
	inline static Library Picking;

	static void Compile(const ShaderCompiler& ShaderCompiler)
	{
		const auto& ExecutableFolderPath = Application::ExecutableDirectory;

		PathTrace = ShaderCompiler.CompileLibrary(ExecutableFolderPath / L"Shaders/PathTrace.hlsl");
		Picking	  = ShaderCompiler.CompileLibrary(ExecutableFolderPath / L"Shaders/Picking.hlsl");
	}
};
