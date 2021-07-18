#pragma once
#include <d3d12.h>

// https://github.com/TheRealMJP/DXRPathTracer/blob/master/SampleFramework12/v1.02/Graphics/Profiler.cpp
struct ProfileData
{
	static constexpr UINT64 FilterSize = 64;

	const char* Name = nullptr;

	bool QueryStarted  = false;
	bool QueryFinished = false;

	INT64 StartTime = 0;
	INT64 EndTime	= 0;

	double TimeSamples[FilterSize] = {};
	UINT64 CurrSample			   = 0;

	double AvgTime;
	double MaxTime;

	void Update(UINT64 Index, UINT64 GpuFrequency, const UINT64* pFrameQueryData);
};

class Profiler
{
public:
	static constexpr UINT64 MaxProfiles = 64;

	static void Initialize(ID3D12Device* pDevice, UINT FrameLatency);
	static void Shutdown();

	static void OnBeginFrame(UINT64 Frequency)
	{
		const UINT64* FrameQueryData = nullptr;
		const UINT64* QueryData		 = Map<UINT64>();
		FrameQueryData				 = QueryData + (FrameIndex * MaxProfiles * 2);

		for (size_t i = 0; i < NumProfiles; ++i)
		{
			Profiles[i].Update(i, Frequency, FrameQueryData);
		}

		Unmap();

		Span = { Profiles.begin(), Profiles.begin() + NumProfiles };

		NumProfiles = 0;
	}
	static void OnEndFrame() { FrameIndex = (FrameIndex + 1) % FrameLatency; }

	static void Update(UINT FrameIndex);

	static UINT64 StartProfile(ID3D12GraphicsCommandList* CommandList, const char* Name);
	static void	  EndProfile(ID3D12GraphicsCommandList* CommandList, UINT64 Index);

	template<typename T>
	static const T* Map()
	{
		T* CPUVirtualAddress = nullptr;
		QueryReadback->Map(0, nullptr, reinterpret_cast<void**>(&CPUVirtualAddress));
		return CPUVirtualAddress;
	}

	static void Unmap() { QueryReadback->Unmap(0, nullptr); }

public:
	inline static std::vector<ProfileData> Profiles;
	inline static UINT64				   NumProfiles = 0;

	inline static std::span<ProfileData> Span;

private:
	inline static UINT									  FrameLatency;
	inline static UINT									  FrameIndex;
	inline static Microsoft::WRL::ComPtr<ID3D12QueryHeap> QueryHeap;
	inline static Microsoft::WRL::ComPtr<ID3D12Resource>  QueryReadback;
};

class ProfileBlock
{
public:
	ProfileBlock(ID3D12GraphicsCommandList* CommandList, const char* Name);
	~ProfileBlock();

private:
	ID3D12GraphicsCommandList* CommandList = nullptr;
	UINT64					   Index	   = UINT64_MAX;
};
