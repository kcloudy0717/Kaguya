#pragma once
#include "RenderGraphCommon.h"

class RenderGraph;

template<typename T, RgResourceType Type>
class RgRegistry
{
public:
	template<typename... TArgs>
	[[nodiscard]] auto Add(TArgs&&... Args) -> RgResourceHandle
	{
		RgResourceHandle Handle = {};
		Handle.Type				= Type;
		Handle.State			= 1;
		Handle.Version			= 0;
		Handle.Id				= Array.size();
		Array.emplace_back(std::forward<TArgs>(Args)...);
		return Handle;
	}

	[[nodiscard]] auto GetResource(RgResourceHandle Handle) -> T*
	{
		assert(Handle.Type == Type);
		return &Array[Handle.Id];
	}

protected:
	std::vector<T> Array;
};

using RootSignatureRegistry			  = RgRegistry<std::unique_ptr<D3D12RootSignature>, RgResourceType::RootSignature>;
using PipelineStateRegistry			  = RgRegistry<std::unique_ptr<D3D12PipelineState>, RgResourceType::PipelineState>;
using RaytracingPipelineStateRegistry = RgRegistry<std::unique_ptr<D3D12RaytracingPipelineState>, RgResourceType::RaytracingPipelineState>;

class RenderGraphRegistry
{
public:
	[[nodiscard]] auto CreateRootSignature(std::unique_ptr<D3D12RootSignature>&& RootSignature) -> RgResourceHandle;
	[[nodiscard]] auto CreatePipelineState(std::unique_ptr<D3D12PipelineState>&& PipelineState) -> RgResourceHandle;
	[[nodiscard]] auto CreateRaytracingPipelineState(std::unique_ptr<D3D12RaytracingPipelineState>&& RaytracingPipelineState) -> RgResourceHandle;

	D3D12RootSignature*			  GetRootSignature(RgResourceHandle Handle);
	D3D12PipelineState*			  GetPipelineState(RgResourceHandle Handle);
	D3D12RaytracingPipelineState* GetRaytracingPipelineState(RgResourceHandle Handle);

	void RealizeResources(RenderGraph* Graph, D3D12Device* Device);

	D3D12Texture* GetImportedResource(RgResourceHandle Handle);

	template<typename T>
	[[nodiscard]] auto Get(RgResourceHandle Handle) -> T*
	{
		auto& Container = GetContainer<T>();
		assert(Handle.IsValid());
		assert(Handle.Type == RgResourceTraits<T>::Enum);
		assert(Handle.Id < Container.size());
		return &Container[Handle.Id];
	}

private:
	template<typename T>
	[[nodiscard]] auto GetContainer() -> std::vector<typename RgResourceTraits<T>::ApiType>&
	{
		if constexpr (std::is_same_v<T, D3D12Buffer>)
		{
			return Buffers;
		}
		else if constexpr (std::is_same_v<T, D3D12Texture>)
		{
			return Textures;
		}
		else if constexpr (std::is_same_v<T, D3D12RenderTarget>)
		{
			return RenderTargets;
		}
		else if constexpr (std::is_same_v<T, D3D12ShaderResourceView>)
		{
			return ShaderResourceViews;
		}
		else if constexpr (std::is_same_v<T, D3D12UnorderedAccessView>)
		{
			return UnorderedAccessViews;
		}
	}

private:
	RenderGraph*					Graph = nullptr;
	RootSignatureRegistry			RootSignatureRegistry;
	PipelineStateRegistry			PipelineStateRegistry;
	RaytracingPipelineStateRegistry RaytracingPipelineStateRegistry;

	std::vector<D3D12Buffer>			  Buffers;
	std::vector<D3D12Texture>			  Textures;
	std::vector<D3D12RenderTarget>		  RenderTargets;
	std::vector<D3D12ShaderResourceView>  ShaderResourceViews;
	std::vector<D3D12UnorderedAccessView> UnorderedAccessViews;
};
