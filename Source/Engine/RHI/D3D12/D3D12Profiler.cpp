#include "D3D12Profiler.h"

namespace RHI
{
	D3D12EventNode	D3D12EventGraph::RootNode	 = D3D12EventNode(-1, "", nullptr);
	D3D12EventNode* D3D12EventGraph::CurrentNode = &D3D12EventGraph::RootNode;

	static D3D12Profiler* g_Profiler = nullptr;

	void UpdateProfileData(ProfileData& Data, UINT64 GpuFrequency, UINT64 StartTime, UINT64 EndTime)
	{
		Data.QueryFinished = false;

		double Time = 0.0;
		if (EndTime > StartTime)
		{
			UINT64 Delta = EndTime - StartTime;
			Time		 = (static_cast<double>(Delta) / static_cast<double>(GpuFrequency)) * 1000.0;
		}

		Data.TimeSamples[Data.Sample] = Time;
		Data.Sample					  = (Data.Sample + 1) % ProfileData::FilterSize;

		UINT64 AvgTimeSamples = 0;
		for (double TimeSample : Data.TimeSamples)
		{
			if (TimeSample <= 0.0)
			{
				continue;
			}
			Data.MaxTime = std::max(TimeSample, Data.MaxTime);
			Data.AverageTime += TimeSample;
			++AvgTimeSamples;
		}

		if (AvgTimeSamples > 0)
		{
			Data.AverageTime /= static_cast<double>(AvgTimeSamples);
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
			g_Profiler->EndProfile(CommandList, std::exchange(Index, UINT_MAX));
		}
	}

	std::span<ProfileData> D3D12Profiler::Data = {};

	D3D12Profiler::D3D12Profiler(
		UINT		  FrameLatency,
		ID3D12Device* Device,
		UINT64		  Frequency)
		: FrameLatency(FrameLatency)
		, Frequency(Frequency)
		, Profiles(MaxProfiles)
		, NumProfiles(0)
		, FrameIndex(0)
	{
		assert(!g_Profiler);
		g_Profiler = this;

		D3D12_QUERY_HEAP_DESC QueryHeapDesc = {
			.Type	  = D3D12_QUERY_HEAP_TYPE_TIMESTAMP,
			.Count	  = MaxProfiles * 2,
			.NodeMask = 0
		};
		VERIFY_D3D12_API(Device->CreateQueryHeap(&QueryHeapDesc, IID_PPV_ARGS(&QueryHeap)));
		QueryHeap->SetName(L"Timestamp Query Heap");

		D3D12_HEAP_PROPERTIES HeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
		D3D12_RESOURCE_DESC	  ResourceDesc	 = CD3DX12_RESOURCE_DESC::Buffer(static_cast<UINT64>(MaxProfiles) * FrameLatency * 2 * sizeof(UINT64));
		VERIFY_D3D12_API(Device->CreateCommittedResource(&HeapProperties, D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&QueryReadback)));
		QueryReadback->SetName(L"Timestamp Query Readback");
	}

	void D3D12Profiler::OnBeginFrame()
	{
		UINT64* QueryData = nullptr;
		if (SUCCEEDED(QueryReadback->Map(0, nullptr, reinterpret_cast<void**>(&QueryData))))
		{
			const UINT64* FrameQueryData = QueryData + static_cast<UINT64>(FrameIndex) * static_cast<UINT64>(MaxProfiles) * 2;

			for (UINT i = 0; i < NumProfiles; ++i)
			{
				UINT64 StartTime = FrameQueryData[i * 2 + 0];
				UINT64 EndTime	 = FrameQueryData[i * 2 + 1];
				UpdateProfileData(Profiles[i], Frequency, StartTime, EndTime);
			}

			Data = { Profiles.begin(), Profiles.begin() + NumProfiles };

			QueryReadback->Unmap(0, nullptr);
		}

		NumProfiles = 0;
	}

	void D3D12Profiler::OnEndFrame()
	{
		FrameIndex = (FrameIndex + 1) % FrameLatency;
		Data	   = {};
	}

	UINT D3D12Profiler::StartProfile(
		ID3D12GraphicsCommandList* CommandList,
		const char*				   Name,
		INT						   Depth)
	{
		assert(NumProfiles < MaxProfiles);
		UINT ProfileIdx			  = NumProfiles++;
		Profiles[ProfileIdx].Name = Name;

		ProfileData& ProfileData = Profiles[ProfileIdx];
		assert(ProfileData.QueryStarted == false);
		assert(ProfileData.QueryFinished == false);

		// Insert the start timestamp
		UINT StartQueryIdx = ProfileIdx * 2;
		CommandList->EndQuery(QueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, StartQueryIdx);

		ProfileData.QueryStarted = true;

		ProfileData.Depth = Depth;

		return ProfileIdx;
	}

	void D3D12Profiler::EndProfile(
		ID3D12GraphicsCommandList* CommandList,
		UINT					   Index)
	{
		assert(Index < NumProfiles);

		ProfileData& ProfileData = Profiles[Index];
		assert(ProfileData.QueryStarted == true);
		assert(ProfileData.QueryFinished == false);

		// Insert the end timestamp
		UINT StartIndex = Index * 2 + 0;
		UINT EndIndex	= Index * 2 + 1;
		CommandList->EndQuery(QueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, EndIndex);

		// Resolve the data
		UINT64 AlignedDestinationBufferOffset = (FrameIndex * MaxProfiles * 2 + StartIndex) * sizeof(UINT64);
		CommandList->ResolveQueryData(QueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, StartIndex, 2, QueryReadback.Get(), AlignedDestinationBufferOffset);

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
} // namespace RHI
