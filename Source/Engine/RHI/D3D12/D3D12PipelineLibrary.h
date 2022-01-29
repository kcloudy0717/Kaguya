#pragma once
#include "D3D12Core.h"

namespace RHI
{
	class D3D12PipelineLibrary : public D3D12DeviceChild
	{
	public:
		explicit D3D12PipelineLibrary(
			D3D12Device*				 Parent,
			const std::filesystem::path& Path);
		~D3D12PipelineLibrary();

		[[nodiscard]] ID3D12PipelineLibrary1* GetLibrary1() const noexcept { return PipelineLibrary1.Get(); }

		void InvalidateDiskCache() { ShouldInvalidateDiskCache = TRUE; }

	private:
		std::filesystem::path		Path;
		FileStream					Stream;
		MemoryMappedFile			MappedFile;
		MemoryMappedView			MappedView;
		Arc<ID3D12PipelineLibrary1> PipelineLibrary1;
		BOOL						ShouldInvalidateDiskCache = FALSE;
	};
} // namespace RHI
