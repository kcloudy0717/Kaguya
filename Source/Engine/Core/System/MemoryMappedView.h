#pragma once

class MemoryMappedView
{
public:
	MemoryMappedView(const MemoryMappedView&) = delete;
	MemoryMappedView& operator=(const MemoryMappedView&) = delete;

	explicit MemoryMappedView(BYTE* View, UINT64 SizeInBytes)
		: View(View)
		, Sentinel(View + SizeInBytes)
	{
	}
	~MemoryMappedView();

	MemoryMappedView(MemoryMappedView&& MemoryMappedView) noexcept;
	MemoryMappedView& operator=(MemoryMappedView&& MemoryMappedView) noexcept;

	[[nodiscard]] bool IsMapped() const noexcept { return View != nullptr; }

	void Flush();

	BYTE* GetView(UINT64 ViewOffset) const noexcept { return View + ViewOffset; }

	template<typename T>
	T Read(UINT64 ViewOffset)
	{
		static_assert(std::is_trivial_v<T>, "typename T is not trivial");
		assert(View + ViewOffset <= Sentinel);

		BYTE* Address = View + ViewOffset;
		return *reinterpret_cast<T*>(Address);
	}

	template<typename T>
	void Write(UINT64 ViewOffset, const T& Data)
	{
		static_assert(std::is_trivial_v<T>, "typename T is not trivial");
		assert(View + ViewOffset <= Sentinel);

		BYTE* DstAddress = View + ViewOffset;
		memcpy(DstAddress, &Data, sizeof(T));
	}

private:
	void InternalDestroy();

private:
	BYTE* View	   = nullptr;
	BYTE* Sentinel = nullptr;
};
