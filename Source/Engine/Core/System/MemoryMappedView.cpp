#include "MemoryMappedView.h"

MemoryMappedView::~MemoryMappedView()
{
	InternalDestroy();
}

MemoryMappedView::MemoryMappedView(MemoryMappedView&& MemoryMappedView) noexcept
	: View(std::exchange(MemoryMappedView.View, {}))
	, Sentinel(std::exchange(MemoryMappedView.Sentinel, {}))
{
}

MemoryMappedView& MemoryMappedView::operator=(MemoryMappedView&& MemoryMappedView) noexcept
{
	if (this == &MemoryMappedView)
	{
		return *this;
	}

	InternalDestroy();
	View	 = std::exchange(MemoryMappedView.View, {});
	Sentinel = std::exchange(MemoryMappedView.Sentinel, {});

	return *this;
}

void MemoryMappedView::Flush()
{
	BOOL OpResult = FlushViewOfFile(View, 0);
	assert(OpResult);
}

void MemoryMappedView::InternalDestroy()
{
	if (View)
	{
		BOOL OpResult = UnmapViewOfFile(View);
		assert(OpResult);
	}
}
