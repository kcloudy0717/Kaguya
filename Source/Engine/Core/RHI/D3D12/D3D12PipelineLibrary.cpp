#include "D3D12PipelineLibrary.h"

D3D12PipelineLibrary::D3D12PipelineLibrary(
	D3D12Device*				 Parent,
	const std::filesystem::path& Path)
	: D3D12DeviceChild(Parent)
	, Path(Path)
	, Stream(Path, FileMode::OpenOrCreate, FileAccess::ReadWrite)
	, MappedFile(Stream)
	, MappedView(MappedFile.CreateView())
{
	ID3D12Device1* Device1 = Parent->GetD3D12Device1();
	if (Device1)
	{
		SIZE_T BlobLength  = MappedView.Read<SIZE_T>(0);
		BYTE*  LibraryBlob = MappedView.GetView(sizeof(SIZE_T));

		// Create a Pipeline Library from the serialized blob.
		// Note: The provided Library Blob must remain valid for the lifetime of the object returned - for efficiency,
		// the data is not copied.
		HRESULT Result = Device1->CreatePipelineLibrary(
			LibraryBlob,
			BlobLength,
			IID_PPV_ARGS(&PipelineLibrary1));
		switch (Result)
		{
		case DXGI_ERROR_UNSUPPORTED: // The driver doesn't support Pipeline libraries. WDDM2.1 drivers must support it.
			break;

		case E_INVALIDARG:						  // The provided Library is corrupted or unrecognized.
		case D3D12_ERROR_ADAPTER_NOT_FOUND:		  // The provided Library contains data for different hardware (Don't really
												  // need to clear the cache, could have a cache per adapter).
		case D3D12_ERROR_DRIVER_VERSION_MISMATCH: // The provided Library contains data from an old driver or runtime.
												  // We need to re-create it.
			VERIFY_D3D12_API(Device1->CreatePipelineLibrary(
				nullptr,
				0,
				IID_PPV_ARGS(&PipelineLibrary1)));
			break;

		default:
			VERIFY_D3D12_API(Result);
		}
	}
}

D3D12PipelineLibrary::~D3D12PipelineLibrary()
{
	// If we're not going to invalidate disk cache file, serialize the library to disk.
	if (!ShouldInvalidateDiskCache)
	{
		if (PipelineLibrary1)
		{
			// Important: An ID3D12PipelineLibrary object becomes undefined when the underlying memory, that was used to
			// initalize it, changes.

			assert(PipelineLibrary1->GetSerializedSize() <= UINT64_MAX); // Code below casts to UINT64.
			const UINT64 SerializedSize = static_cast<UINT64>(PipelineLibrary1->GetSerializedSize());
			if (SerializedSize > 0)
			{
				// Grow the file if needed.
				const size_t FileSize = sizeof(SIZE_T) + SerializedSize;
				if (FileSize > MappedFile.GetCurrentFileSize())
				{
					// The file mapping is going to change thus it will invalidate the ID3D12PipelineLibrary object.
					// Serialize the library contents to temporary memory first.
					std::unique_ptr<BYTE[]> pSerializedData = std::make_unique<BYTE[]>(SerializedSize);
					if (pSerializedData)
					{
						if (SUCCEEDED(PipelineLibrary1->Serialize(
								pSerializedData.get(),
								SerializedSize)))
						{
							MappedView.Flush();

							// Now it's safe to grow the mapping.
							MappedFile.GrowMapping(FileSize);
							// Update view
							MappedView = MappedFile.CreateView();

							// Update serialized size and write the serialized blob
							MappedView.Write<SIZE_T>(0, SerializedSize);
							memcpy(MappedView.GetView(sizeof(SIZE_T)), pSerializedData.get(), SerializedSize);
						}
						else
						{
							LOG_WARN("Failed to serialize pipeline library");
						}
					}
				}
				else
				{
					// The mapping didn't change, we can serialize directly to the mapped file.
					// Save the size of the library and the library itself.
					assert(FileSize <= MappedFile.GetCurrentFileSize());

					MappedView.Write<SIZE_T>(0, SerializedSize);
					HRESULT OpResult = PipelineLibrary1->Serialize(MappedView.GetView(sizeof(SIZE_T)), SerializedSize);
					if (FAILED(OpResult))
					{
						LOG_WARN("Failed to serialize pipeline library");
					}
				}

				// PipelineLibrary1 is now undefined because we just wrote to the mapped file, don't use it again.
			}
		}

		MappedView.Flush();
	}
	else
	{
		LOG_INFO("Pipeline library disk cache invalidated");
		Stream.Reset();
		BOOL OpResult = DeleteFile(Path.c_str());
		assert(OpResult);
	}
}
