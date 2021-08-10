#pragma once
#include "D3D12Common.h"

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

	INT Depth = 0;

	void Update(UINT64 Index, UINT64 GpuFrequency, const UINT64* pFrameQueryData);
};

struct D3D12EventNode
{
	D3D12EventNode(INT Depth, const std::string& Name, D3D12EventNode* Parent)
		: Depth(Depth)
		, Name(Name)
		, Parent(Parent)
	{
	}
	~D3D12EventNode()
	{
		for (auto Child : Children)
		{
			delete Child;
		}
		Children.clear();
		LUT.clear();
	}

	D3D12EventNode* GetChild(const std::string& Name)
	{
		auto iter = LUT.find(Name);
		if (iter != LUT.end())
		{
			return iter->second;
		}

		D3D12EventNode* pEventNode = new D3D12EventNode(Depth + 1, Name, this);
		Children.push_back(pEventNode);
		LUT[Name] = pEventNode;
		return pEventNode;
	}

	void StartTiming(ID3D12GraphicsCommandList* CommandList);

	void EndTiming(ID3D12GraphicsCommandList* CommandList);

	INT												 Depth;
	std::string										 Name;
	UINT64											 Index = UINT64_MAX;
	D3D12EventNode*									 Parent;
	std::vector<D3D12EventNode*>					 Children;
	std::unordered_map<std::string, D3D12EventNode*> LUT;
};

class D3D12EventGraph
{
public:
	static void PushEventNode(const std::string& Name, ID3D12GraphicsCommandList* CommandList)
	{
		CurrentNode = CurrentNode->GetChild(Name);
		CurrentNode->StartTiming(CommandList);
	}

	static void PopEventNode(ID3D12GraphicsCommandList* CommandList)
	{
		CurrentNode->EndTiming(CommandList);
		CurrentNode = CurrentNode->Parent;
	}

private:
	inline static D3D12EventNode RootNode = D3D12EventNode(-1, "", nullptr);

	inline static D3D12EventNode* CurrentNode = &RootNode;
};

class D3D12Profiler
{
public:
	static constexpr UINT64 MaxProfiles = 64;

	D3D12Profiler(UINT FrameLatency);

	void Initialize(ID3D12Device* pDevice, UINT64 Frequency);

	void OnBeginFrame()
	{
		UINT64* QueryData = nullptr;
		if (SUCCEEDED(QueryReadback->Map(0, nullptr, reinterpret_cast<void**>(&QueryData))))
		{
			const UINT64* FrameQueryData = QueryData + (FrameIndex * MaxProfiles * 2);

			for (UINT64 i = 0; i < NumProfiles; ++i)
			{
				Profiles[i].Update(i, Frequency, FrameQueryData);
			}

			QueryReadback->Unmap(0, nullptr);

			Data = { Profiles.begin(), Profiles.begin() + NumProfiles };
		}

		NumProfiles = 0;
	}
	void OnEndFrame()
	{
		FrameIndex = (FrameIndex + 1) % FrameLatency;
		Data	   = {};
	}

	UINT64 StartProfile(ID3D12GraphicsCommandList* CommandList, const char* Name, INT Depth);
	void   EndProfile(ID3D12GraphicsCommandList* CommandList, UINT64 Index);

public:
	inline static std::span<ProfileData> Data;

private:
	const UINT FrameLatency;
	UINT64	   Frequency;

	std::vector<ProfileData> Profiles;
	UINT64					 NumProfiles = 0;

	UINT									FrameIndex;
	Microsoft::WRL::ComPtr<ID3D12QueryHeap> QueryHeap;
	Microsoft::WRL::ComPtr<ID3D12Resource>	QueryReadback;
};

class D3D12ProfileBlock
{
public:
	D3D12ProfileBlock(ID3D12GraphicsCommandList* CommandList, const char* Name);
	~D3D12ProfileBlock();

private:
	ID3D12GraphicsCommandList* CommandList;
};
