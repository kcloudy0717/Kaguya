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

namespace RHI
{
	class D3D12CommandQueue;
	class D3D12Profiler;

	struct ProfileData
	{
		static constexpr UINT64 FilterSize = 64;

		std::string_view Name;

		bool QueryStarted  = false;
		bool QueryFinished = false;

		INT64 StartTime = 0;
		INT64 EndTime	= 0;

		double TimeSamples[FilterSize] = {};
		UINT64 Sample				   = 0;

		double AverageTime;
		double MaxTime;

		INT	   Depth	 = 0;
		UINT64 Frequency = 0;
	};

	struct D3D12EventNode
	{
		D3D12EventNode() noexcept = default;
		explicit D3D12EventNode(D3D12Profiler* Profiler, INT Depth, std::string_view Name, D3D12EventNode* Parent)
			: Profiler(Profiler)
			, Depth(Depth)
			, Name(Name)
			, Parent(Parent)
		{
		}

		D3D12EventNode* GetChild(std::string_view Name)
		{
			if (auto Iterator = Lut.find(Name);
				Iterator != Lut.end())
			{
				return Iterator->second;
			}

			auto& Child = Children.emplace_back(std::make_unique<D3D12EventNode>(Profiler, Depth + 1, Name, this));
			Lut[Name]	= Child.get();
			return Child.get();
		}

		void StartTiming(D3D12CommandQueue* CommandQueue, ID3D12GraphicsCommandList* CommandList);
		void EndTiming(ID3D12GraphicsCommandList* CommandList);

		D3D12Profiler*										  Profiler;
		INT													  Depth;
		std::string_view									  Name;
		UINT												  Index = UINT_MAX;
		D3D12EventNode*										  Parent;
		std::vector<std::unique_ptr<D3D12EventNode>>		  Children;
		std::unordered_map<std::string_view, D3D12EventNode*> Lut;
	};

	class D3D12Profiler : public D3D12LinkedDeviceChild
	{
	public:
		static constexpr UINT MaxProfiles = 1024;

		D3D12Profiler() noexcept = default;
		explicit D3D12Profiler(
			D3D12LinkedDevice* Parent,
			UINT			   FrameLatency);

		D3D12Profiler(D3D12Profiler&&) noexcept = default;
		D3D12Profiler& operator=(D3D12Profiler&&) noexcept = default;

		D3D12Profiler(const D3D12Profiler&) noexcept = delete;
		D3D12Profiler& operator=(const D3D12Profiler&) noexcept = delete;

		void OnBeginFrame();
		void OnEndFrame();

		void PushEventNode(D3D12CommandQueue* CommandQueue, std::string_view Name, ID3D12GraphicsCommandList* CommandList);
		void PopEventNode(ID3D12GraphicsCommandList* CommandList);

	public:
		// Read only
		std::span<ProfileData> Data;

	private:
		UINT StartProfile(ID3D12GraphicsCommandList* CommandList, std::string_view Name, INT Depth, UINT64 Frequency);
		void EndProfile(ID3D12GraphicsCommandList* CommandList, UINT Index);

	private:
		UINT					 FrameLatency;
		std::vector<ProfileData> Profiles;
		UINT					 NumProfiles;
		UINT					 FrameIndex;
		Arc<ID3D12QueryHeap>	 TimestampQueryHeap;
		Arc<ID3D12Resource>		 TimestampQueryReadback;

		// Event Graph
		friend struct D3D12EventNode;
		D3D12EventNode	RootNode;
		D3D12EventNode* CurrentNode;
	};

	class D3D12ProfileBlock
	{
	public:
		D3D12ProfileBlock(
			D3D12Profiler*			   Profiler,
			D3D12CommandQueue*		   CommandQueue,
			ID3D12GraphicsCommandList* CommandList,
			std::string_view		   Name);
		~D3D12ProfileBlock();

	private:
		D3D12Profiler*			   Profiler;
		ID3D12GraphicsCommandList* CommandList;
	};
} // namespace RHI
