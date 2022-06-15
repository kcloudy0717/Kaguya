// Normally argument "DispatchArgument" is parsed through a constant buffer. However, if for some reason it is a
// static value, some DXC compiler versions will be unable to compile the code.
// If that's the case for you, flip DXC_STATIC_DISPATCH_GRID_DIM definition from 0 to 1.
#define DXC_STATIC_DISPATCH_GRID_DIM 0

// Divide the 2D-Dispatch_Grid into tiles of dimension [N, DipatchGridDim.y]
// "CTA" (Cooperative Thread Array) == Thread Group in DirectX terminology
uint2 ThreadGroupTilingX(
	const uint2 DispatchArgument, // Arguments of the Dispatch call (typically from a ConstantBuffer)
	const uint2 NumThreads,		  // Already known in HLSL, eg:[numthreads(8, 8, 1)] -> uint2(8, 8)
	const uint	MaxTileWidth,	  // User parameter (N). Recommended values: 8, 16 or 32.
	const uint2 GroupThreadID,	  // SV_GroupThreadID
	const uint2 GroupID			  // SV_GroupID
)
{
	// A perfect tile is one with dimensions = [maxTileWidth, DispatchArgument.y]
	const uint Number_of_CTAs_in_a_perfect_tile = MaxTileWidth * DispatchArgument.y;

	// Possible number of perfect tiles
	const uint Number_of_perfect_tiles = DispatchArgument.x / MaxTileWidth;

	// Total number of CTAs present in the perfect tiles
	const uint Total_CTAs_in_all_perfect_tiles = Number_of_perfect_tiles * MaxTileWidth * DispatchArgument.y;
	const uint vThreadGroupIDFlattened		   = DispatchArgument.x * GroupID.y + GroupID.x;

	// Tile_ID_of_current_CTA : current CTA to TILE-ID mapping.
	const uint Tile_ID_of_current_CTA			= vThreadGroupIDFlattened / Number_of_CTAs_in_a_perfect_tile;
	const uint Local_CTA_ID_within_current_tile = vThreadGroupIDFlattened % Number_of_CTAs_in_a_perfect_tile;
	uint	   Local_CTA_ID_y_within_current_tile;
	uint	   Local_CTA_ID_x_within_current_tile;

	if (Total_CTAs_in_all_perfect_tiles <= vThreadGroupIDFlattened)
	{
		// Path taken only if the last tile has imperfect dimensions and CTAs from the last tile are launched.
		uint X_dimension_of_last_tile = DispatchArgument.x % MaxTileWidth;
#if DXC_STATIC_DISPATCH_GRID_DIM
		X_dimension_of_last_tile = max(1, X_dimension_of_last_tile);
#endif
		Local_CTA_ID_y_within_current_tile = Local_CTA_ID_within_current_tile / X_dimension_of_last_tile;
		Local_CTA_ID_x_within_current_tile = Local_CTA_ID_within_current_tile % X_dimension_of_last_tile;
	}
	else
	{
		Local_CTA_ID_y_within_current_tile = Local_CTA_ID_within_current_tile / MaxTileWidth;
		Local_CTA_ID_x_within_current_tile = Local_CTA_ID_within_current_tile % MaxTileWidth;
	}

	const uint Swizzled_vThreadGroupIDFlattened =
		Tile_ID_of_current_CTA * MaxTileWidth +
		Local_CTA_ID_y_within_current_tile * DispatchArgument.x +
		Local_CTA_ID_x_within_current_tile;

	uint2 SwizzledvThreadGroupID;
	SwizzledvThreadGroupID.y = Swizzled_vThreadGroupIDFlattened / DispatchArgument.x;
	SwizzledvThreadGroupID.x = Swizzled_vThreadGroupIDFlattened % DispatchArgument.x;

	uint2 SwizzledvThreadID;
	SwizzledvThreadID.x = NumThreads.x * SwizzledvThreadGroupID.x + GroupThreadID.x;
	SwizzledvThreadID.y = NumThreads.y * SwizzledvThreadGroupID.y + GroupThreadID.y;

	return SwizzledvThreadID.xy;
}
