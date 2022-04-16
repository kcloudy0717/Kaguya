#pragma once
#include <type_traits>
#include <stdexcept>

namespace SpanInternal
{
	template<typename C>
	concept ContainerConvertible = requires(C Container)
	{
		Container.data();
		Container.size();
	};

	template<typename T>
	concept ConceptMutableView = !std::is_const_v<T>;

	template<typename T>
	concept ConceptConstView = std::is_const_v<T>;

	// A constexpr min function to avoid min/max macro
	constexpr size_t Min(size_t a, size_t b) noexcept { return a < b ? a : b; }
} // namespace SpanInternal

template<typename T>
class Span
{
public:
	using value_type	  = std::remove_cv_t<T>;
	using pointer		  = T*;
	using const_pointer	  = const T*;
	using reference		  = T&;
	using const_reference = const T&;
	using iterator		  = pointer;
	using const_iterator  = const_pointer;

	static constexpr size_t npos = ~static_cast<size_t>(0);

	constexpr Span() noexcept = default;

	constexpr Span(pointer Data, size_t Size)
		: Data(Data)
		, Size(Size)
	{
	}

	template<size_t N>
	constexpr Span(T (&Array)[N]) noexcept
		: Span(Array, N)
	{
	}

	template<typename C>
	explicit Span(C& Container) requires SpanInternal::ContainerConvertible<C> && SpanInternal::ConceptMutableView<T>
		: Span(Container.data(), Container.size())
	{
	}

	template<typename C>
	Span(const C& Container) requires SpanInternal::ContainerConvertible<C> && SpanInternal::ConceptConstView<T>
		: Span(Container.data(), Container.size())
	{
	}

	// Implicit constructor from an initializer list, making it possible to pass a
	// brace-enclosed initializer list to a function expecting a `Span`. Such
	// spans constructed from an initializer list must be of type `Span<const T>`.
	//
	//   void Process(Span<const int> x);
	//   Process({1, 2, 3});
	//
	// Note that as always the array referenced by the span must outlive the span.
	// Since an initializer list constructor acts as if it is fed a temporary
	// array (cf. C++ standard [dcl.init.list]/5), it's safe to use this
	// constructor only when the `std::initializer_list` itself outlives the span.
	// In order to meet this requirement it's sufficient to ensure that neither
	// the span nor a copy of it is used outside of the expression in which it's
	// created:
	//
	//   // Assume that this function uses the array directly, not retaining any
	//   // copy of the span or pointer to any of its elements.
	//   void Process(Span<const int> ints);
	//
	//   // Okay: the std::initializer_list<int> will reference a temporary array
	//   // that isn't destroyed until after the call to Process returns.
	//   Process({ 17, 19 });
	//
	//   // Not okay: the storage used by the std::initializer_list<int> is not
	//   // allowed to be referenced after the first line.
	//   Span<const int> ints = { 17, 19 };
	//   Process(ints);
	//
	//   // Not okay for the same reason as above: even when the elements of the
	//   // initializer list expression are not temporaries the underlying array
	//   // is, so the initializer list must still outlive the span.
	//   const int foo = 17;
	//   Span<const int> ints = { foo };
	//   Process(ints);
	template<typename LazyT = T>
	Span(std::initializer_list<value_type> List) requires SpanInternal::ConceptConstView<LazyT>
		: Span(List.begin(), List.size())
	{
	}

	[[nodiscard]] constexpr T*	   data() const noexcept { return Data; }
	[[nodiscard]] constexpr size_t size() const noexcept { return Size; }
	[[nodiscard]] constexpr bool   empty() const noexcept { return size() == 0; }

	[[nodiscard]] constexpr T*		 begin() const noexcept { return data(); }
	[[nodiscard]] constexpr const T* cbegin() const noexcept { return begin(); }
	[[nodiscard]] constexpr T*		 end() const noexcept { return data() + size(); }
	[[nodiscard]] constexpr const T* cend() const noexcept { return end(); }

	[[nodiscard]] constexpr T& operator[](size_t Index) const noexcept
	{
		assert(Index < size());
		return *(data() + Index);
	}

	[[nodiscard]] constexpr T& at(size_t Index) const
	{
		if (Index < size())
		{
			return *(data() + Index);
		}
		throw std::out_of_range(__FUNCTION__ " out of range");
	}

	[[nodiscard]] constexpr T& front() const noexcept
	{
		assert(size() > 0);
		return *data();
	}

	[[nodiscard]] constexpr T& back() const noexcept
	{
		assert(size() > 0);
		return *(data() + size() - 1);
	}

	[[nodiscard]] constexpr Span Subspan(size_t Offset, size_t Length = npos)
	{
		assert(Offset <= size());
		return Span(data() + Offset, SpanInternal::Min(size() - Offset, Length));
	}

private:
	T*	   Data = nullptr;
	size_t Size = 0;
};

// ExplicitArgumentBarrier is to prevent explicitly template parameters so the function will always deduce the type

template<int&... ExplicitArgumentBarrier, typename T>
constexpr Span<T> MakeSpan(T* Data, size_t Size) noexcept
{
	return Span<T>(Data, Size);
}

template<int&... ExplicitArgumentBarrier, typename T, size_t N>
constexpr Span<T> MakeSpan(T (&Array)[N]) noexcept
{
	return Span<T>(Array, N);
}

template<int&... ExplicitArgumentBarrier, typename T>
Span<T> MakeSpan(T* Begin, T* End) noexcept
{
	assert(Begin <= End);
	return Span<T>(Begin, static_cast<size_t>(End - Begin));
}

template<int&... ExplicitArgumentBarrier, SpanInternal::ContainerConvertible C>
constexpr auto MakeSpan(C& Container) noexcept
{
	return MakeSpan(Container.data(), Container.size());
}
