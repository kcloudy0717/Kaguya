namespace Math
{
	inline Vec4f mul(const Vec4f& v, const Matrix4x4& m) noexcept
	{
		//				[a b c d]
		// [x y z w] *	[e f g h]
		//				[i j k l]
		//				[m n o p]
		//
		// x' = x * a + y * e + z * i + w * m
		// y' = x * b + y * f + z * j + w * n
		// z' = x * c + y * g + z * k + w * o
		// w' = x * d + y * h + z * l + w * p
		return Vec4f(
			v.x * m._11 + v.y * m._21 + v.z * m._31 + v.w * m._41,
			v.x * m._12 + v.y * m._22 + v.z * m._32 + v.w * m._42,
			v.x * m._13 + v.y * m._23 + v.z * m._33 + v.w * m._43,
			v.x * m._14 + v.y * m._24 + v.z * m._34 + v.w * m._44);
	}

	inline Matrix4x4 mul(const Matrix4x4& a, const Matrix4x4& b) noexcept
	{
		Vec4f R[4];
		for (size_t i = 0; i < 4; ++i)
		{
			R[i] = mul(a.GetRow(i), b);
		}
		return { R[0], R[1], R[2], R[3] };
	}
} // namespace Math
