#pragma once
#include <tuple>

template<typename T, typename TTuple>
struct AttributeContainer
{
	inline static const char* Name		 = RegisterClassName<T>();
	inline static TTuple	  Attributes = RegisterClassAttributes<T>();
};

template<typename... TArgs>
auto Attributes(TArgs&&... Args)
{
	return std::make_tuple(std::forward<TArgs>(Args)...);
}

template<typename TClass>
const char* RegisterClassName();

template<typename TClass>
auto RegisterClassAttributes();

template<typename TClass>
const char* GetClass()
{
	return AttributeContainer<TClass, decltype(RegisterClassAttributes<TClass>())>::Name;
}

template<typename TClass>
auto& GetAttributes()
{
	return AttributeContainer<TClass, decltype(RegisterClassAttributes<TClass>())>::Attributes;
}

template<typename TClass>
constexpr std::size_t GetNumAttributes()
{
	return std::tuple_size<decltype(RegisterClassAttributes<TClass>())>::value;
}

template<typename TClass, typename Functor>
void ForEachAttribute(Functor&& F)
{
	std::apply(
		[&](auto&&... x)
		{
			(..., F(x));
		},
		GetAttributes<TClass>());
}

template<typename TClass, typename T>
class Attribute
{
public:
	using Ptr = T TClass::*;

	Attribute(const char* Name, Ptr pPtr)
		: Name(Name)
		, pPtr(pPtr)
	{
	}

	[[nodiscard]] const char* GetName() const noexcept { return Name; }

	// Used exclusively by decltype
	T GetType() const noexcept { return T(); }

	const T& Get(const TClass& Class) const noexcept { return Class.*pPtr; }

	T& Get(TClass& Class) noexcept { return Class.*pPtr; }

	void Set(TClass& Class, T&& Value) { Class.*pPtr = std::forward<T>(Value); }

private:
	const char* Name;
	Ptr			pPtr;
};

#define CLASS_ATTRIBUTE(Class, x) Attribute(#x, &Class::x)

#define REGISTER_CLASS_ATTRIBUTES(Class, Name, ...)                                                                    \
	template<>                                                                                                         \
	inline const char* RegisterClassName<Class>()                                                                      \
	{                                                                                                                  \
		return Name;                                                                                                   \
	}                                                                                                                  \
	template<>                                                                                                         \
	inline auto RegisterClassAttributes<Class>()                                                                       \
	{                                                                                                                  \
		return Attributes(__VA_ARGS__);                                                                                \
	}
