#include "Texture.h"

#pragma region DDS
#pragma pack(push, 1)
constexpr uint32_t DDS_MAGIC = 0x20534444; // "DDS "

struct DDS_PIXELFORMAT
{
	DWORD Size;
	DWORD Flags;
	DWORD FourCC;
	DWORD RGBBitCount;
	DWORD RBitMask;
	DWORD GBitMask;
	DWORD BBitMask;
	DWORD ABitMask;
};

#define DDS_FOURCC		0x00000004 // DDPF_FOURCC
#define DDS_RGB			0x00000040 // DDPF_RGB
#define DDS_RGBA		0x00000041 // DDPF_RGB | DDPF_ALPHAPIXELS
#define DDS_LUMINANCE	0x00020000 // DDPF_LUMINANCE
#define DDS_LUMINANCEA	0x00020001 // DDPF_LUMINANCE | DDPF_ALPHAPIXELS
#define DDS_ALPHAPIXELS 0x00000001 // DDPF_ALPHAPIXELS
#define DDS_ALPHA		0x00000002 // DDPF_ALPHA
#define DDS_PAL8		0x00000020 // DDPF_PALETTEINDEXED8
#define DDS_PAL8A		0x00000021 // DDPF_PALETTEINDEXED8 | DDPF_ALPHAPIXELS
#define DDS_BUMPDUDV	0x00080000 // DDPF_BUMPDUDV

#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3) (static_cast<uint32_t>(static_cast<uint8_t>(ch0)) | (static_cast<uint32_t>(static_cast<uint8_t>(ch1)) << 8) | (static_cast<uint32_t>(static_cast<uint8_t>(ch2)) << 16) | (static_cast<uint32_t>(static_cast<uint8_t>(ch3)) << 24))
#endif /* MAKEFOURCC */

static constexpr DDS_PIXELFORMAT DDSPF_R16_FLOAT		  = { sizeof(DDS_PIXELFORMAT), DDS_FOURCC, 111, 0, 0, 0, 0, 0 };
static constexpr DDS_PIXELFORMAT DDSPF_R16G16_FLOAT		  = { sizeof(DDS_PIXELFORMAT), DDS_FOURCC, 112, 0, 0, 0, 0, 0 };
static constexpr DDS_PIXELFORMAT DDSPF_R16G16B16A16_FLOAT = { sizeof(DDS_PIXELFORMAT), DDS_FOURCC, 113, 0, 0, 0, 0, 0 };
static constexpr DDS_PIXELFORMAT DDSPF_R32_FLOAT		  = { sizeof(DDS_PIXELFORMAT), DDS_FOURCC, 114, 0, 0, 0, 0, 0 };
static constexpr DDS_PIXELFORMAT DDSPF_R32G32_FLOAT		  = { sizeof(DDS_PIXELFORMAT), DDS_FOURCC, 115, 0, 0, 0, 0, 0 };
static constexpr DDS_PIXELFORMAT DDSPF_R32G32B32A32_FLOAT = { sizeof(DDS_PIXELFORMAT), DDS_FOURCC, 115, 0, 0, 0, 0, 0 };

#define DDS_HEADER_FLAGS_TEXTURE	0x00001007 // DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT
#define DDS_HEADER_FLAGS_MIPMAP		0x00020000 // DDSD_MIPMAPCOUNT
#define DDS_HEADER_FLAGS_VOLUME		0x00800000 // DDSD_DEPTH
#define DDS_HEADER_FLAGS_PITCH		0x00000008 // DDSD_PITCH
#define DDS_HEADER_FLAGS_LINEARSIZE 0x00080000 // DDSD_LINEARSIZE

#define DDS_HEIGHT					0x00000002 // DDSD_HEIGHT
#define DDS_WIDTH					0x00000004 // DDSD_WIDTH

#define DDS_SURFACE_FLAGS_TEXTURE	0x00001000 // DDSCAPS_TEXTURE
#define DDS_SURFACE_FLAGS_MIPMAP	0x00400008 // DDSCAPS_COMPLEX | DDSCAPS_MIPMAP
#define DDS_SURFACE_FLAGS_CUBEMAP	0x00000008 // DDSCAPS_COMPLEX

#define DDS_CUBEMAP_POSITIVEX		0x00000600 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEX
#define DDS_CUBEMAP_NEGATIVEX		0x00000a00 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEX
#define DDS_CUBEMAP_POSITIVEY		0x00001200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEY
#define DDS_CUBEMAP_NEGATIVEY		0x00002200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEY
#define DDS_CUBEMAP_POSITIVEZ		0x00004200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEZ
#define DDS_CUBEMAP_NEGATIVEZ		0x00008200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEZ

#define DDS_CUBEMAP_ALLFACES		(DDS_CUBEMAP_POSITIVEX | DDS_CUBEMAP_NEGATIVEX | DDS_CUBEMAP_POSITIVEY | DDS_CUBEMAP_NEGATIVEY | DDS_CUBEMAP_POSITIVEZ | DDS_CUBEMAP_NEGATIVEZ)

#define DDS_CUBEMAP					0x00000200 // DDSCAPS2_CUBEMAP

#define DDS_FLAGS_VOLUME			0x00200000 // DDSCAPS2_VOLUME

// Subset here matches D3D10_RESOURCE_DIMENSION and D3D11_RESOURCE_DIMENSION
enum DDS_RESOURCE_DIMENSION : uint32_t
{
	DDS_DIMENSION_TEXTURE1D = 2,
	DDS_DIMENSION_TEXTURE2D = 3,
	DDS_DIMENSION_TEXTURE3D = 4,
};

// Subset here matches D3D10_RESOURCE_MISC_FLAG and D3D11_RESOURCE_MISC_FLAG
enum DDS_RESOURCE_MISC_FLAG : uint32_t
{
	DDS_RESOURCE_MISC_TEXTURECUBE = 0x4L,
};

enum DDS_MISC_FLAGS2 : uint32_t
{
	DDS_MISC_FLAGS2_ALPHA_MODE_MASK = 0x7L,
};

#ifndef DDS_ALPHA_MODE_DEFINED
#define DDS_ALPHA_MODE_DEFINED
enum DDS_ALPHA_MODE : uint32_t
{
	DDS_ALPHA_MODE_UNKNOWN		 = 0,
	DDS_ALPHA_MODE_STRAIGHT		 = 1,
	DDS_ALPHA_MODE_PREMULTIPLIED = 2,
	DDS_ALPHA_MODE_OPAQUE		 = 3,
	DDS_ALPHA_MODE_CUSTOM		 = 4,
};
#endif

struct DDS_HEADER
{
	DWORD			Size;
	DWORD			Flags;
	DWORD			Height;
	DWORD			Width;
	DWORD			PitchOrLinearSize;
	DWORD			Depth; // only if DDS_HEADER_FLAGS_VOLUME is set in flags
	DWORD			MipMapCount;
	DWORD			Reserved1[11];
	DDS_PIXELFORMAT DDSPF;
	DWORD			Caps;
	DWORD			Caps2;
	DWORD			Caps3;
	DWORD			Caps4;
	DWORD			Reserved2;
};

#pragma pack(pop)

const DDS_PIXELFORMAT* GetDDSPF(DXGI_FORMAT Format)
{
	// clang-format off
	switch (Format)
	{
	case DXGI_FORMAT_R16_FLOAT:				return &DDSPF_R16_FLOAT;
	case DXGI_FORMAT_R16G16_FLOAT:			return &DDSPF_R16G16_FLOAT;
	case DXGI_FORMAT_R16G16B16A16_FLOAT:	return &DDSPF_R16G16B16A16_FLOAT;
	case DXGI_FORMAT_R32_FLOAT:				return &DDSPF_R32_FLOAT;
	case DXGI_FORMAT_R32G32_FLOAT:			return &DDSPF_R32G32_FLOAT;
	case DXGI_FORMAT_R32G32B32A32_FLOAT:	return &DDSPF_R32G32B32A32_FLOAT;
	}
	// clang-format on
	return nullptr;
}
#pragma endregion

namespace Asset
{
	void Texture::SaveToDisk(const std::filesystem::path& Path)
	{
		SaveToDisk(DxTexture, Path);
	}

	void Texture::SaveToDisk(RHI::D3D12Texture& Texture, const std::filesystem::path& Path)
	{
		const std::string Extension = Path.extension().string();
		if (Extension == ".dds")
		{
			SaveToDDS(Texture, Path);
		}
	}

	void Texture::Release()
	{
		TexImage.Release();
	}

	void Texture::SaveToDDS(RHI::D3D12Texture& Texture, const std::filesystem::path& Path)
	{
		RHI::D3D12LinkedDevice*	  Device = Texture.GetParentLinkedDevice();
		const D3D12_RESOURCE_DESC Desc	 = Texture.GetDesc();
		auto					  DDSPF	 = GetDDSPF(Desc.Format);
		if (!DDSPF)
		{
			throw std::exception("Unsupported Format");
		}

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT Layout		  = {};
		UINT							   NumRows		  = 0;
		UINT64							   RowSizeInBytes = 0;
		UINT64							   TotalBytes	  = 0;

		Device->GetDevice()->GetCopyableFootprints(
			&Desc,
			0,
			1,
			0,
			&Layout,
			&NumRows,
			&RowSizeInBytes,
			&TotalBytes);

		RHI::D3D12Buffer ReadbackBuffer = RHI::D3D12Buffer(Device, TotalBytes, 0, D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_FLAG_NONE);

		RHI::D3D12CommandContext& Context = Device->GetCommandContext();
		Context.Open();
		{
			const CD3DX12_TEXTURE_COPY_LOCATION Dst(ReadbackBuffer.GetResource(), Layout);
			const CD3DX12_TEXTURE_COPY_LOCATION Src(Texture.GetResource(), 0);

			Context.TransitionBarrier(&Texture, D3D12_RESOURCE_STATE_COPY_SOURCE);
			Context->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);
		}
		Context.Close();
		Context.Execute(true);

		const DDS_HEADER Header = {
			.Size			   = sizeof(DDS_HEADER),
			.Flags			   = DDS_HEADER_FLAGS_TEXTURE | DDS_HEADER_FLAGS_MIPMAP | DDS_HEADER_FLAGS_PITCH,
			.Height			   = Desc.Height,
			.Width			   = static_cast<DWORD>(Desc.Width),
			.PitchOrLinearSize = static_cast<DWORD>(RowSizeInBytes),

			.MipMapCount = 1,

			.DDSPF = *DDSPF,
			.Caps  = DDS_SURFACE_FLAGS_TEXTURE
		};

		std::unique_ptr<uint8_t[]> Pixels(new (std::nothrow) uint8_t[RowSizeInBytes * NumRows]);

		void*		pMappedMemory = nullptr;
		D3D12_RANGE ReadRange	  = { 0, TotalBytes };
		D3D12_RANGE WriteRange	  = { 0, 0 };
		ReadbackBuffer.GetResource()->Map(0, &ReadRange, &pMappedMemory);
		auto	 SrcPtr = static_cast<const uint8_t*>(pMappedMemory);
		uint8_t* DstPtr = Pixels.get();

		for (size_t i = 0; i < NumRows; ++i)
		{
			memcpy(DstPtr, SrcPtr, RowSizeInBytes);
			SrcPtr += Layout.Footprint.RowPitch;
			DstPtr += RowSizeInBytes;
		}

		ReadbackBuffer.GetResource()->Unmap(0, &WriteRange);

		FileStream	 Stream(Path, FileMode::Create);
		BinaryWriter Writer(Stream);
		Writer << DDS_MAGIC;
		Writer << Header;
		Writer.Write(Pixels.get(), RowSizeInBytes * NumRows);
	}
} // namespace Asset
