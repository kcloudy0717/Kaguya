#include "D3D12Profiler.h"

static D3D12Profiler* g_Profiler = nullptr;

void ProfileData::Update(UINT64 Index, UINT64 GpuFrequency, const UINT64* pFrameQueryData)
{
	QueryFinished = false;

	double Time		 = 0.0;
	UINT64 StartTime = pFrameQueryData[Index * 2 + 0];
	UINT64 EndTime	 = pFrameQueryData[Index * 2 + 1];

	if (EndTime > StartTime)
	{
		UINT64 Delta = EndTime - StartTime;
		Time		 = (Delta / double(GpuFrequency)) * 1000.0;
	}

	TimeSamples[CurrSample] = Time;
	CurrSample				= (CurrSample + 1) % ProfileData::FilterSize;

	UINT64 AvgTimeSamples = 0;
	for (double TimeSample : TimeSamples)
	{
		if (TimeSample <= 0.0)
		{
			continue;
		}
		MaxTime = std::max(TimeSample, MaxTime);
		AvgTime += TimeSample;
		++AvgTimeSamples;
	}

	if (AvgTimeSamples > 0)
	{
		AvgTime /= double(AvgTimeSamples);
	}
}

void D3D12EventNode::StartTiming(ID3D12GraphicsCommandList* CommandList)
{
	if (CommandList)
	{
		Index = g_Profiler->StartProfile(CommandList, Name.data(), Depth);
	}
}

void D3D12EventNode::EndTiming(ID3D12GraphicsCommandList* CommandList)
{
	if (CommandList)
	{
		g_Profiler->EndProfile(CommandList, Index);
	}
}

D3D12Profiler::D3D12Profiler(UINT FrameLatency)
	: FrameLatency(FrameLatency)
	, Profiles(MaxProfiles)
{
	assert(!g_Profiler);
	g_Profiler = this;
}

void D3D12Profiler::Initialize(ID3D12Device* pDevice, UINT64 Frequency)
{
	this->Frequency = Frequency;
	FrameIndex		= 0;

	D3D12_QUERY_HEAP_DESC QueryHeapDesc = { .Type	  = D3D12_QUERY_HEAP_TYPE_TIMESTAMP,
											.Count	  = MaxProfiles * 2,
											.NodeMask = 0 };
	VERIFY_D3D12_API(pDevice->CreateQueryHeap(&QueryHeapDesc, IID_PPV_ARGS(&QueryHeap)));
	QueryHeap->SetName(L"Timestamp Query Heap");

	D3D12_HEAP_PROPERTIES HeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
	D3D12_RESOURCE_DESC	  ResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(MaxProfiles * FrameLatency * 2 * sizeof(UINT64));
	VERIFY_D3D12_API(pDevice->CreateCommittedResource(
		&HeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&ResourceDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(QueryReadback.ReleaseAndGetAddressOf())));
	QueryReadback->SetName(L"Timestamp Query Readback");

	Profiles.resize(MaxProfiles);
}

UINT64 D3D12Profiler::StartProfile(ID3D12GraphicsCommandList* CommandList, const char* Name, INT Depth)
{
	assert(NumProfiles < MaxProfiles);
	UINT64 ProfileIdx		  = NumProfiles++;
	Profiles[ProfileIdx].Name = Name;

	ProfileData& ProfileData = Profiles[ProfileIdx];
	assert(ProfileData.QueryStarted == false);
	assert(ProfileData.QueryFinished == false);

	// Insert the start timestamp
	const UINT32 StartQueryIdx = UINT32(ProfileIdx * 2);
	CommandList->EndQuery(QueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, StartQueryIdx);

	ProfileData.QueryStarted = true;

	ProfileData.Depth = Depth;

	return ProfileIdx;
}

void D3D12Profiler::EndProfile(ID3D12GraphicsCommandList* CommandList, UINT64 Index)
{
	assert(Index < NumProfiles);

	ProfileData& ProfileData = Profiles[Index];
	assert(ProfileData.QueryStarted == true);
	assert(ProfileData.QueryFinished == false);

	// Insert the end timestamp
	const UINT32 StartQueryIdx = UINT32(Index * 2);
	const UINT32 EndQueryIdx   = UINT32(Index * 2 + 1);
	CommandList->EndQuery(QueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, EndQueryIdx);

	// Resolve the data
	const UINT64 AlignedDestinationBufferOffset = ((FrameIndex * MaxProfiles * 2) + StartQueryIdx) * sizeof(UINT64);
	CommandList->ResolveQueryData(
		QueryHeap.Get(),
		D3D12_QUERY_TYPE_TIMESTAMP,
		StartQueryIdx,
		2,
		QueryReadback.Get(),
		AlignedDestinationBufferOffset);

	ProfileData.QueryStarted  = false;
	ProfileData.QueryFinished = true;
}

D3D12ProfileBlock::D3D12ProfileBlock(ID3D12GraphicsCommandList* CommandList, const char* Name)
	: CommandList(CommandList)
{
	D3D12EventGraph::PushEventNode(Name, CommandList);
}

D3D12ProfileBlock::~D3D12ProfileBlock()
{
	D3D12EventGraph::PopEventNode(CommandList);
}
