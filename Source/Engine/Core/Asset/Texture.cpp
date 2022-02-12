#include "Texture.h"

namespace Asset
{
	void Texture::Release()
	{
		TexImage.Release();
	}
} // namespace Asset
