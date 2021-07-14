#pragma once
#include <tuple>

template<typename T, typename TTuple>
struct AttributeContainer
{
	inline static const char* Name		 = RegisterClassName<T>();
	inline static TTuple	  Attributes = RegisterClassAttributes<T>();
};

template<typename... TArgs>
inline auto Attributes(TArgs&&... Args)
{
	return std::make_tuple(std::forward<TArgs>(Args)...);
}

template<typename TClass>
inline const char* RegisterClassName();

template<typename TClass>
inline auto RegisterClassAttributes();

template<typename TClass>
inline const char* GetClass()
{
	return AttributeContainer<TClass, decltype(RegisterClassAttributes<TClass>())>::Name;
}

template<typename TClass>
inline const auto& GetAttributes()
{
	return AttributeContainer<TClass, decltype(RegisterClassAttributes<TClass>())>::Attributes;
}

template<typename TClass>
inline constexpr std::size_t GetNumAttributes()
{
	return std::tuple_size<decltype(RegisterClassAttributes<TClass>())>::value;
}

template<typename TClass, typename Functor>
inline void ForEachAttribute(Functor&& F)
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

	const char* GetName() const noexcept { return Name; }

	const T& Get(const TClass& Class) const noexcept { return Class.*pPtr; }

	T& Get(TClass& Class) noexcept { return Class.*pPtr; }

private:
	const char* Name;
	Ptr			pPtr;
};

#define CLASS_ATTRIBUTE(Class, x) Attribute(#x, &Class::x)

#define REGISTER_CLASS_ATTRIBUTES(Class, ...)                                                                          \
	template<>                                                                                                         \
	inline const char* RegisterClassName<Class>()                                                                      \
	{                                                                                                                  \
		return #Class;                                                                                                 \
	}                                                                                                                  \
	template<>                                                                                                         \
	inline auto RegisterClassAttributes<Class>()                                                                       \
	{                                                                                                                  \
		CLASS_ATTRIBUTE(Tag, Name);                                                                                    \
		return Attributes(__VA_ARGS__);                                                                                \
	}
