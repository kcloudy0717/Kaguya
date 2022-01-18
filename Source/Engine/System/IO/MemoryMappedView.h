#pragma once
#include "SystemCore.h"

class MemoryMappedView
{
public:
	explicit MemoryMappedView(
		std::byte* View,
		u64		   SizeInBytes);
	MemoryMappedView(MemoryMappedView&& MemoryMappedView) noexcept;
	MemoryMappedView& operator=(MemoryMappedView&& MemoryMappedView) noexcept;
	~MemoryMappedView();

	MemoryMappedView(const MemoryMappedView&) = delete;
	MemoryMappedView& operator=(const MemoryMappedView&) = delete;

	[[nodiscard]] bool IsMapped() const noexcept { return View != nullptr; }

	void Flush();

	[[nodiscard]] std::byte* GetView(u64 ViewOffset) const noexcept;

	template<typename T>
	T Read(u64 ViewOffset)
	{
		static_assert(std::is_trivial_v<T>, "typename T is not trivial");

		return *reinterpret_cast<T*>(GetView(ViewOffset));
	}

	template<typename T>
	void Write(u64 ViewOffset, const T& Data)
	{
		static_assert(std::is_trivial_v<T>, "typename T is not trivial");

		std::byte* DstAddress = GetView(ViewOffset);
		memcpy(DstAddress, &Data, sizeof(T));
	}

private:
	void InternalDestroy();

private:
	std::byte* View		= nullptr;
	std::byte* Sentinel = nullptr;
};
