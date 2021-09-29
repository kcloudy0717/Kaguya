
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

[numthreads(128, 1, 1)]
void CSMain(CSParams Params)
{
	// Each thread of the CS operates on one of the indirect commands.
	uint index = (groupId.x * threadBlockSize) + groupIndex;

	// Don't attempt to access commands that don't exist if more threads are allocated
	// than commands.
	if (index < commandCount)
	{
		// Project the left and right bounds of the triangle into homogenous space.
		float4 left = float4(-xOffset, 0.0f, zOffset, 1.0f) + cbv[index].offset;
		left		= mul(left, cbv[index].projection);
		left /= left.w;

		float4 right = float4(xOffset, 0.0f, zOffset, 1.0f) + cbv[index].offset;
		right		 = mul(right, cbv[index].projection);
		right /= right.w;

		// Only draw triangles that are within the culling space.
		if (-cullOffset < right.x && left.x < cullOffset)
		{
			outputCommands.Append(inputCommands[index]);
		}
	}
}
