#pragma once
#include "Resource.h"

// https://github.com/TheRealMJP/DXRPathTracer/blob/master/SampleFramework12/v1.02/Graphics/Profiler.cpp
struct ProfileData
{
	const char* Name = nullptr;

	bool QueryStarted  = false;
	bool QueryFinished = false;
	bool Active		   = false;

	INT64 StartTime = 0;
	INT64 EndTime	= 0;

	static const UINT64 FilterSize				= 64;
	double				TimeSamples[FilterSize] = {};
	UINT64				CurrSample				= 0;

	std::pair<double, double> Update(UINT64 Index, UINT64 GpuFrequency, const UINT64* pFrameQueryData);
};

class Profiler
{
public:
	static constexpr UINT64 MaxProfiles = 64;

	static void Initialize(ID3D12Device* pDevice, UINT FrameLatency);
	static void Shutdown();

	static void Update(UINT FrameIndex);

	static UINT64 StartProfile(ID3D12GraphicsCommandList* CommandList, const char* Name);
	static void	  EndProfile(ID3D12GraphicsCommandList* CommandList, UINT64 Index);

	template<typename T>
	static const T* Map()
	{
		return m_ReadbackBuffer.Map<T>();
	}

	static void Unmap() { m_ReadbackBuffer.Unmap(); }

public:
	inline static bool					   GpuProfiling = false;
	inline static std::vector<ProfileData> Profiles;
	inline static UINT64				   NumProfiles = 0;

private:
	inline static UINT									  m_FrameLatency;
	inline static UINT									  m_FrameIndex;
	inline static Microsoft::WRL::ComPtr<ID3D12QueryHeap> m_QueryHeap;
	inline static ReadbackBuffer						  m_ReadbackBuffer;
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
