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
		inline static Shader EASU;
		inline static Shader RCAS;
	};

	static void Compile(const ShaderCompiler& ShaderCompiler)
	{
		const auto& ExecutableDirectory = Application::ExecutableDirectory;

		// VS
		{
			VS::FullScreenTriangle = ShaderCompiler.CompileShader(
				EShaderType::Vertex,
				ExecutableDirectory / L"Shaders/FullScreenTriangle.hlsl",
				g_VSEntryPoint,
				{});
		}

		// PS
		{
			PS::ToneMap = ShaderCompiler.CompileShader(
				EShaderType::Pixel,
				ExecutableDirectory / L"Shaders/PostProcess/ToneMap.hlsl",
				g_PSEntryPoint,
				{});
		}

		// CS
		{
			CS::EASU = ShaderCompiler.CompileShader(
				EShaderType::Compute,
				ExecutableDirectory / L"Shaders/PostProcess/FSR.hlsl",
				g_CSEntryPoint,
				{
					{ L"SAMPLE_SLOW_FALLBACK", L"0" },
					{ L"SAMPLE_BILINEAR", L"0" },
					{ L"SAMPLE_EASU", L"1" },
					{ L"SAMPLE_RCAS", L"0" },
				});

			CS::RCAS = ShaderCompiler.CompileShader(
				EShaderType::Compute,
				ExecutableDirectory / L"Shaders/PostProcess/FSR.hlsl",
				g_CSEntryPoint,
				{
					{ L"SAMPLE_SLOW_FALLBACK", L"0" },
					{ L"SAMPLE_BILINEAR", L"0" },
					{ L"SAMPLE_EASU", L"0" },
					{ L"SAMPLE_RCAS", L"1" },
				});
		}
	}
};

struct Libraries
{
	inline static Library PathTrace;

	static void Compile(const ShaderCompiler& ShaderCompiler)
	{
		const auto& ExecutableDirectory = Application::ExecutableDirectory;

		PathTrace = ShaderCompiler.CompileLibrary(ExecutableDirectory / L"Shaders/PathTrace.hlsl");
	}
};
