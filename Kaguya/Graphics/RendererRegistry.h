#pragma once
#include "RenderDevice.h"

// Gives a float2 barycentric, the last element can be found via 1 - b.x - b.y since the sum of barycentrics = 1
static constexpr size_t SizeOfBuiltInTriangleIntersectionAttributes = 2 * sizeof(float);

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
		inline static Shader InstanceGeneration;

		inline static Shader PostProcess_BloomMask;
		inline static Shader PostProcess_BloomDownsample;
		inline static Shader PostProcess_BloomBlur;
		inline static Shader PostProcess_BloomUpsampleBlurAccumulation;
		inline static Shader PostProcess_BloomComposition;
	};

	static void Compile(const ShaderCompiler& ShaderCompiler)
	{
		const auto& ExecutableFolderPath = Application::ExecutableDirectory;

		// Load VS
		{
			VS::FullScreenTriangle = ShaderCompiler.CompileShader(
				Shader::EType::Vertex,
				ExecutableFolderPath / L"Shaders/FullScreenTriangle.hlsl",
				g_VSEntryPoint,
				{});
		}

		// Load PS
		{
			PS::ToneMap = ShaderCompiler.CompileShader(
				Shader::EType::Pixel,
				ExecutableFolderPath / L"Shaders/PostProcess/ToneMap.hlsl",
				g_PSEntryPoint,
				{});
		}

		// Load CS
		{
			// Not used, need to add them back
			// CS::InstanceGeneration			= ShaderCompiler.CompileShader(Shader::Type::Compute,
			// ExecutableFolderPath / L"Shaders/InstanceGeneration.hlsl",		g_CSEntryPoint, {});

			// CS::PostProcess_BloomMask						= ShaderCompiler.CompileShader(Shader::Type::Compute,
			// ExecutableFolderPath / L"Shaders/PostProcess/BloomMask.hlsl",						g_CSEntryPoint, {});
			// CS::PostProcess_BloomDownsample					= ShaderCompiler.CompileShader(Shader::Type::Compute,
			// ExecutableFolderPath / L"Shaders/PostProcess/BloomDownsample.hlsl",				g_CSEntryPoint, {});
			// CS::PostProcess_BloomBlur						= ShaderCompiler.CompileShader(Shader::Type::Compute,
			// ExecutableFolderPath / L"Shaders/PostProcess/BloomBlur.hlsl",						g_CSEntryPoint, {});
			// CS::PostProcess_BloomUpsampleBlurAccumulation	= ShaderCompiler.CompileShader(Shader::Type::Compute,
			// ExecutableFolderPath / L"Shaders/PostProcess/BloomUpsampleBlurAccumulation.hlsl",	g_CSEntryPoint, {});
			// CS::PostProcess_BloomComposition				= ShaderCompiler.CompileShader(Shader::Type::Compute,
			// ExecutableFolderPath / L"Shaders/PostProcess/BloomComposition.hlsl",				g_CSEntryPoint, {});
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
