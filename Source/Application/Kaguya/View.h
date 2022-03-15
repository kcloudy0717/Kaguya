#pragma once
#include <RHI/RHI.h>
#include <Core/World/World.h>
#include "RaytracingAccelerationStructure.h"

struct View
{
	unsigned int Width, Height;
};

struct WorldRenderView
{
	WorldRenderView(RHI::D3D12LinkedDevice* Device)
	{
		Lights = RHI::D3D12Buffer(
			Device,
			sizeof(Hlsl::Light) * World::LightLimit,
			sizeof(Hlsl::Light),
			D3D12_HEAP_TYPE_UPLOAD,
			D3D12_RESOURCE_FLAG_NONE);

		Materials = RHI::D3D12Buffer(
			Device,
			sizeof(Hlsl::Material) * World::MaterialLimit,
			sizeof(Hlsl::Material),
			D3D12_HEAP_TYPE_UPLOAD,
			D3D12_RESOURCE_FLAG_NONE);

		Meshes = RHI::D3D12Buffer(
			Device,
			sizeof(Hlsl::Mesh) * World::MeshLimit,
			sizeof(Hlsl::Mesh),
			D3D12_HEAP_TYPE_UPLOAD,
			D3D12_RESOURCE_FLAG_NONE);

		pLights	  = Lights.GetCpuVirtualAddress<Hlsl::Light>();
		pMaterial = Materials.GetCpuVirtualAddress<Hlsl::Material>();
		pMeshes	  = Meshes.GetCpuVirtualAddress<Hlsl::Mesh>();
	}

	void Update(World* World, /*Optional*/ RaytracingAccelerationStructure* RaytracingAccelerationStructure)
	{
		NumMaterials = NumLights = NumMeshes = 0;
		if (RaytracingAccelerationStructure)
		{
			RaytracingAccelerationStructure->Reset();
		}

		World->Registry.view<CoreComponent, LightComponent>().each(
			[&](CoreComponent& Core, LightComponent& Light)
			{
				pLights[NumLights++] = GetHLSLLightDesc(Core.Transform, Light);
			});
		World->Registry.view<CoreComponent, StaticMeshComponent>().each(
			[&](CoreComponent& Core, StaticMeshComponent& StaticMesh)
			{
				if (StaticMesh.Mesh)
				{
					RHI::D3D12Buffer& VertexBuffer = StaticMesh.Mesh->VertexResource;
					RHI::D3D12Buffer& IndexBuffer  = StaticMesh.Mesh->IndexResource;

					D3D12_DRAW_INDEXED_ARGUMENTS DrawIndexedArguments = {};
					DrawIndexedArguments.IndexCountPerInstance		  = StaticMesh.Mesh->IndexCount;
					DrawIndexedArguments.InstanceCount				  = 1;
					DrawIndexedArguments.StartIndexLocation			  = 0;
					DrawIndexedArguments.BaseVertexLocation			  = 0;
					DrawIndexedArguments.StartInstanceLocation		  = 0;

					Hlsl::Mesh Mesh	  = GetHLSLMeshDesc(Core.Transform);
					Mesh.VertexBuffer = VertexBuffer.GetVertexBufferView();
					Mesh.IndexBuffer  = IndexBuffer.GetIndexBufferView();
					if (StaticMesh.Mesh->Options.GenerateMeshlets)
					{
						Mesh.Meshlets			 = StaticMesh.Mesh->MeshletResource.GetGpuVirtualAddress();
						Mesh.UniqueVertexIndices = StaticMesh.Mesh->UniqueVertexIndexResource.GetGpuVirtualAddress();
						Mesh.PrimitiveIndices	 = StaticMesh.Mesh->PrimitiveIndexResource.GetGpuVirtualAddress();
					}
					Mesh.BoundingBox		  = StaticMesh.Mesh->BoundingBox;
					Mesh.DrawIndexedArguments = DrawIndexedArguments;
					Mesh.MaterialIndex		  = NumMaterials;
					Mesh.NumMeshlets		  = StaticMesh.Mesh->MeshletCount;
					Mesh.VertexView			  = StaticMesh.Mesh->VertexView.GetIndex();
					Mesh.IndexView			  = StaticMesh.Mesh->IndexView.GetIndex();

					pMaterial[NumMaterials] = GetHLSLMaterialDesc(StaticMesh.Material);
					pMeshes[NumMeshes]		= Mesh;
					if (RaytracingAccelerationStructure)
					{
						RaytracingAccelerationStructure->AddInstance(Core.Transform, &StaticMesh);
					}

					++NumMaterials;
					++NumMeshes;
				}
			});
	}

	RHI::D3D12Buffer Lights;
	RHI::D3D12Buffer Materials;
	RHI::D3D12Buffer Meshes;

	u32 NumLights = 0, NumMaterials = 0, NumMeshes = 0;

	Hlsl::Light*	pLights	  = nullptr;
	Hlsl::Material* pMaterial = nullptr;
	Hlsl::Mesh*		pMeshes	  = nullptr;
};
