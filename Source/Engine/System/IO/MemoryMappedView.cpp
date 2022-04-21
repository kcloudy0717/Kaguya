#include "MemoryMappedView.h"

MemoryMappedView::MemoryMappedView(
	std::byte* View,
	u64		   SizeInBytes)
	: View(View)
	, Sentinel(View + SizeInBytes)
{
}

MemoryMappedView::MemoryMappedView(MemoryMappedView&& MemoryMappedView) noexcept
	: View(std::exchange(MemoryMappedView.View, {}))
	, Sentinel(std::exchange(MemoryMappedView.Sentinel, {}))
{
}

MemoryMappedView& MemoryMappedView::operator=(MemoryMappedView&& MemoryMappedView) noexcept
{
	if (this != &MemoryMappedView)
	{
		InternalDestroy();
		View	 = std::exchange(MemoryMappedView.View, {});
		Sentinel = std::exchange(MemoryMappedView.Sentinel, {});
	}
	return *this;
}

MemoryMappedView::~MemoryMappedView()
{
	InternalDestroy();
}

void MemoryMappedView::Flush()
{
	BOOL OpResult = FlushViewOfFile(View, 0);
	assert(OpResult);
}

std::byte* MemoryMappedView::GetView(u64 ViewOffset) const noexcept
{
	assert(View + ViewOffset <= Sentinel);
	return View + ViewOffset;
}

void MemoryMappedView::InternalDestroy()
{
	if (View)
	{
		BOOL OpResult = UnmapViewOfFile(View);
		View		  = nullptr;
		Sentinel	  = nullptr;
		assert(OpResult);
	}
}
