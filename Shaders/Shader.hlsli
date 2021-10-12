#pragma once

struct CSParams
{
	// Indices for which thread group a CS is executing in.
	uint3 GroupID : SV_GroupID;

	// Global thread id w.r.t to the entire dispatch call
	// Ranging from [0, numthreads * ThreadGroupCountX,ThreadGroupCountY,ThreadGroupCountZ))^3
	uint3 DispatchThreadID : SV_DispatchThreadID;

	// Local thread id w.r.t the current thread group
	// Ranging from [0, numthreads)^3
	uint3 GroupThreadID : SV_GroupThreadID;

	// The "flattened" index of a compute shader thread within a thread group, which turns the multi-dimensional
	// SV_GroupThreadID into a 1D value. SV_GroupIndex varies from 0 to (numthreadsX * numthreadsY * numThreadsZ) – 1.
	uint GroupIndex : SV_GroupIndex;
};

struct MSParams
{
	// Indices for which thread group a MS is executing in.
	uint3 GroupID : SV_GroupID;

	// Global thread id w.r.t to the entire dispatch call
	// Ranging from [0, numthreads * ThreadGroupCountX,ThreadGroupCountY,ThreadGroupCountZ))^3
	uint3 DispatchThreadID : SV_DispatchThreadID;

	// Local thread id w.r.t the current thread group
	// Ranging from [0, numthreads)^3
	uint3 GroupThreadID : SV_GroupThreadID;

	// The "flattened" index of a compute shader thread within a thread group, which turns the multi-dimensional
	// SV_GroupThreadID into a 1D value. SV_GroupIndex varies from 0 to (numthreadsX * numthreadsY * numThreadsZ) – 1.
	uint GroupIndex : SV_GroupIndex;
};
