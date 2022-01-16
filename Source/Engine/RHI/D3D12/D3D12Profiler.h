#pragma once
#include "D3D12Core.h"

//=================================================================================================
//
//  MJP's DX12 Sample Framework
//  http://mynameismjp.wordpress.com/
//
//  All code licensed under the MIT license
//
//=================================================================================================
// https://github.com/TheRealMJP/DXRPathTracer/blob/master/SampleFramework12/v1.02/Graphics/Profiler.h
// Modified MJP's Profiler to my own use

struct ProfileData
{
	static constexpr UINT64 FilterSize = 64;

	const char* Name = nullptr;

	bool QueryStarted  = false;
	bool QueryFinished = false;

	INT64 StartTime = 0;
	INT64 EndTime	= 0;

	double TimeSamples[FilterSize] = {};
	UINT64 Sample				   = 0;

	double AverageTime;
	double MaxTime;

	INT Depth = 0;
};

struct D3D12EventNode
{
	D3D12EventNode(INT Depth, std::string Name, D3D12EventNode* Parent)
		: Depth(Depth)
		, Name(std::move(Name))
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
		Lut.clear();
	}

	D3D12EventNode* GetChild(const std::string& Name)
	{
		auto iter = Lut.find(Name);
		if (iter != Lut.end())
		{
			return iter->second;
		}

		auto EventNode = new D3D12EventNode(Depth + 1, Name, this);
		Children.push_back(EventNode);
		Lut[Name] = EventNode;
		return EventNode;
	}

	void StartTiming(ID3D12GraphicsCommandList* CommandList);

	void EndTiming(ID3D12GraphicsCommandList* CommandList);

	INT												 Depth;
	std::string										 Name;
	UINT											 Index = UINT_MAX;
	D3D12EventNode*									 Parent;
	std::vector<D3D12EventNode*>					 Children;
	std::unordered_map<std::string, D3D12EventNode*> Lut;
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
	static D3D12EventNode  RootNode;
	static D3D12EventNode* CurrentNode;
};

class D3D12Profiler
{
public:
	static constexpr UINT MaxProfiles = 128;

	explicit D3D12Profiler(
		UINT		  FrameLatency,
		ID3D12Device* Device,
		UINT64		  Frequency);

	void OnBeginFrame();
	void OnEndFrame();

	UINT StartProfile(
		ID3D12GraphicsCommandList* CommandList,
		const char*				   Name,
		INT						   Depth);

	void EndProfile(
		ID3D12GraphicsCommandList* CommandList,
		UINT					   Index);

public:
	// Read only
	static std::span<ProfileData> Data;

private:
	const UINT				 FrameLatency;
	UINT64					 Frequency = 0;
	std::vector<ProfileData> Profiles;
	UINT					 NumProfiles;
	UINT					 FrameIndex;
	ARC<ID3D12QueryHeap>	 QueryHeap;
	ARC<ID3D12Resource>		 QueryReadback;
};

class D3D12ProfileBlock
{
public:
	D3D12ProfileBlock(ID3D12GraphicsCommandList* CommandList, const char* Name);
	~D3D12ProfileBlock();

private:
	ID3D12GraphicsCommandList* CommandList;
};
