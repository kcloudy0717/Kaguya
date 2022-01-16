#include "Ray.h"

Vec3f Ray::operator()(float T) const noexcept
{
	return Origin + Direction * T;
}
