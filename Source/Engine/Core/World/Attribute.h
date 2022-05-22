#pragma once
#include <tuple>

namespace Reflection
{
	template<typename T, typename TTuple>
	struct ReflectionClass
	{
		inline static const char* Name		 = RegisterReflectionClassName<T>();
		inline static TTuple	  Attributes = RegisterReflectionClassAttributes<T>();
	};

	template<typename... TArgs>
	auto Attributes(TArgs&&... Args)
	{
		return std::make_tuple(std::forward<TArgs>(Args)...);
	}

	template<typename TClass>
	const char* RegisterReflectionClassName();

	template<typename TClass>
	auto RegisterReflectionClassAttributes();

	template<typename TClass>
	const char* GetReflectionClassName()
	{
		return ReflectionClass<TClass, decltype(RegisterReflectionClassAttributes<TClass>())>::Name;
	}

	template<typename TClass>
	auto& GetReflectionClassAttributes()
	{
		return ReflectionClass<TClass, decltype(RegisterReflectionClassAttributes<TClass>())>::Attributes;
	}

	template<typename TClass>
	constexpr std::size_t GetNumAttributes()
	{
		return std::tuple_size_v<decltype(RegisterReflectionClassAttributes<TClass>())>;
	}

	template<typename TClass, typename Functor>
	void ForEachAttributeIn(Functor&& F)
	{
		std::apply(
			[&](auto&&... x)
			{
				(..., F(x));
			},
			GetReflectionClassAttributes<TClass>());
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
} // namespace Reflection

#define CLASS_ATTRIBUTE(Class, x) Reflection::Attribute(#x, &Class::x)

#define REGISTER_CLASS_ATTRIBUTES(Class, Name, ...)             \
	namespace Reflection                                        \
	{                                                           \
		template<>                                              \
		inline const char* RegisterReflectionClassName<Class>() \
		{                                                       \
			return Name;                                        \
		}                                                       \
		template<>                                              \
		inline auto RegisterReflectionClassAttributes<Class>()  \
		{                                                       \
			return Attributes(__VA_ARGS__);                     \
		}                                                       \
	}
