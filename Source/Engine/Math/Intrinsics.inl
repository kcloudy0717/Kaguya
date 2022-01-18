namespace xvm
{
	// Internal xvm intrinsics
	XVM_INLINE __m128 _xvm_mm_cross_ps(__m128 v1, __m128 v2)
	{
		// http://threadlocalmutex.com/?p=8
		// (v1 x v2).x = v1.y * v2.z - v1.z * v2.y;
		// (v1 x v2).y = v1.z * v2.x - v1.x * v2.z;
		// (v1 x v2).z = v1.x * v2.y - v1.y * v2.x;
		//
		// moving the 3rd operation to top yields this
		//
		// (v1 x v2).z = v1.x * v2.y - v1.y * v2.x;
		// (v1 x v2).x = v1.y * v2.z - v1.z * v2.y;
		// (v1 x v2).y = v1.z * v2.x - v1.x * v2.z;
		//
		// we can perform swizzling here
		//
		// (v1 x v2).zxy = v1 * v2.yzx - v1.yzx * v2;
		//
		// swizzle zxy on both sides gives us the cross product again
		//
		// (v1 x v2) = (v1 * v2.yzx - v1.yzx * v2).yzx;
		__m128 v1_yzx = XVM_SWIZZLE_PS(v1, _MM_SHUFFLE(3, 0, 2, 1));
		__m128 v2_yzx = XVM_SWIZZLE_PS(v2, _MM_SHUFFLE(3, 0, 2, 1));
		__m128 cross  = _mm_sub_ps(_mm_mul_ps(v1, v2_yzx), _mm_mul_ps(v1_yzx, v2));
		return XVM_SWIZZLE_PS(cross, _MM_SHUFFLE(3, 0, 2, 1));
	}
} // namespace xvm
