#include "Profiler.h"

//=================================================================================================
//
//  MJP's DX12 Sample Framework
//  http://mynameismjp.wordpress.com/
//
//  All code licensed under the MIT license
//
//=================================================================================================

std::pair<double, double> ProfileData::Update(UINT64 Index, UINT64 GpuFrequency, const UINT64* pFrameQueryData)
{
	QueryFinished = false;

	double time = 0.0f;
	if (pFrameQueryData)
	{
		// Get the query data
		UINT64 startTime = pFrameQueryData[Index * 2 + 0];
		UINT64 endTime	 = pFrameQueryData[Index * 2 + 1];

		if (endTime > startTime)
		{
			UINT64 delta = endTime - startTime;
			time		 = (delta / double(GpuFrequency)) * 1000.0;
		}
	}

	TimeSamples[CurrSample] = time;
	CurrSample				= (CurrSample + 1) % ProfileData::FilterSize;

	double maxTime		  = 0.0;
	double avgTime		  = 0.0;
	UINT64 avgTimeSamples = 0;
	for (UINT i = 0; i < ProfileData::FilterSize; ++i)
	{
		if (TimeSamples[i] <= 0.0)
			continue;
		maxTime = std::max(TimeSamples[i], maxTime);
		avgTime += TimeSamples[i];
		++avgTimeSamples;
	}

	if (avgTimeSamples > 0)
		avgTime /= double(avgTimeSamples);

	return std::make_pair(avgTime, maxTime);
}

void Profiler::Initialize(ID3D12Device* pDevice, UINT FrameLatency)
{
	GpuProfiling = true;

	m_FrameIndex   = 0;
	m_FrameLatency = FrameLatency;

	D3D12_QUERY_HEAP_DESC heapDesc = { .Type	 = D3D12_QUERY_HEAP_TYPE_TIMESTAMP,
									   .Count	 = MaxProfiles * 2,
									   .NodeMask = 0 };
	pDevice->CreateQueryHeap(&heapDesc, IID_PPV_ARGS(&m_QueryHeap));

	m_ReadbackBuffer = ReadbackBuffer(pDevice, MaxProfiles * m_FrameLatency * 2 * sizeof(UINT64));
	m_ReadbackBuffer.Resource->SetName(L"Query Readback Buffer");

	Profiles.resize(MaxProfiles);
}

void Profiler::Shutdown()
{
	m_ReadbackBuffer.Resource.Reset();
	m_QueryHeap.Reset();
}

void Profiler::Update(UINT FrameIndex)
{
	m_FrameIndex = FrameIndex;
}

UINT64 Profiler::StartProfile(ID3D12GraphicsCommandList* CommandList, const char* Name)
{
	if (GpuProfiling == false)
		return UINT64_MAX;

	UINT64 profileIdx = UINT64_MAX;
	for (UINT64 i = 0; i < NumProfiles; ++i)
	{
		if (Profiles[i].Name == Name)
		{
			profileIdx = i;
			break;
		}
	}

	if (profileIdx == UINT64_MAX)
	{
		assert(NumProfiles < MaxProfiles);
		profileIdx				  = NumProfiles++;
		Profiles[profileIdx].Name = Name;
	}

	ProfileData& profileData = Profiles[profileIdx];
	assert(profileData.QueryStarted == false);
	assert(profileData.QueryFinished == false);
	profileData.Active = true;

	// Insert the start timestamp
	const UINT32 startQueryIdx = UINT32(profileIdx * 2);
	CommandList->EndQuery(m_QueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, startQueryIdx);

	profileData.QueryStarted = true;

	return profileIdx;
}

void Profiler::EndProfile(ID3D12GraphicsCommandList* CommandList, UINT64 Index)
{
	if (GpuProfiling == false)
		return;

	assert(Index < NumProfiles);

	ProfileData& profileData = Profiles[Index];
	assert(profileData.QueryStarted == true);
	assert(profileData.QueryFinished == false);

	// Insert the end timestamp
	const UINT32 startQueryIdx = UINT32(Index * 2);
	const UINT32 endQueryIdx   = startQueryIdx + 1;
	CommandList->EndQuery(m_QueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, endQueryIdx);

	// Resolve the data
	const UINT64 dstOffset = ((m_FrameIndex * MaxProfiles * 2) + startQueryIdx) * sizeof(UINT64);
	CommandList->ResolveQueryData(
		m_QueryHeap.Get(),
		D3D12_QUERY_TYPE_TIMESTAMP,
		startQueryIdx,
		2,
		m_ReadbackBuffer.Resource.Get(),
		dstOffset);

	profileData.QueryStarted  = false;
	profileData.QueryFinished = true;
}

ProfileBlock::ProfileBlock(ID3D12GraphicsCommandList* CommandList, const char* Name)
	: CommandList(CommandList)
{
	Index = Profiler::StartProfile(CommandList, Name);
}

ProfileBlock::~ProfileBlock()
{
	Profiler::EndProfile(CommandList, Index);
}
