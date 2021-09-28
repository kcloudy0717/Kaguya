#pragma once
#include <string_view>

class IConsoleObject
{
public:
	virtual ~IConsoleObject() = default;

	[[nodiscard]] virtual bool		  GetBool() const	= 0;
	[[nodiscard]] virtual int		  GetInt() const	= 0;
	[[nodiscard]] virtual float		  GetFloat() const	= 0;
	[[nodiscard]] virtual std::string GetString() const = 0;

	virtual void Set(const std::string& Value) = 0;

	void Set(bool Value) { Set(Value ? "true" : "false"); }
	void Set(int Value) { Set(std::to_string(Value)); }
	void Set(float Value) { Set(std::to_string(Value)); }
};

template<typename T>
class ConsoleVariable
{
public:
	ConsoleVariable(IConsoleObject* ConsoleVariable)
		: Variable(ConsoleVariable)
	{
	}
	ConsoleVariable(std::string_view Name, std::string_view Description, const T& DefaultValue) {}

	operator T();

	IConsoleObject&		  operator*() { return *Variable; }
	const IConsoleObject& operator*() const { return *Variable; }
	IConsoleObject*		  operator->() { return Variable; }
	const IConsoleObject* operator->() const { return Variable; }

private:
	IConsoleObject* Variable;
};

class IConsole
{
public:
	virtual ~IConsole() = default;

	static IConsole& Instance();

	virtual IConsoleObject* Register(std::string_view Name, std::string_view Description, const bool& DefaultValue) = 0;

	virtual IConsoleObject* Register(std::string_view Name, std::string_view Description, const int& DefaultValue) = 0;

	virtual IConsoleObject* Register(
		std::string_view Name,
		std::string_view Description,
		const float&	 DefaultValue) = 0;

	virtual IConsoleObject* Register(
		std::string_view   Name,
		std::string_view   Description,
		const std::string& DefaultValue) = 0;

	virtual ConsoleVariable<bool>		 FindBoolCVar(std::string_view Name)   = 0;
	virtual ConsoleVariable<int>		 FindIntCVar(std::string_view Name)	   = 0;
	virtual ConsoleVariable<float>		 FindFloatCVar(std::string_view Name)  = 0;
	virtual ConsoleVariable<std::string> FindStringCVar(std::string_view Name) = 0;
};

template<>
inline ConsoleVariable<bool>::ConsoleVariable(
	std::string_view Name,
	std::string_view Description,
	const bool&		 DefaultValue)
	: Variable(IConsole::Instance().Register(Name, Description, DefaultValue))
{
}
template<>
inline ConsoleVariable<bool>::operator bool()
{
	return Variable->GetBool();
}

template<>
inline ConsoleVariable<int>::ConsoleVariable(
	std::string_view Name,
	std::string_view Description,
	const int&		 DefaultValue)
	: Variable(IConsole::Instance().Register(Name, Description, DefaultValue))
{
}
template<>
inline ConsoleVariable<int>::operator int()
{
	return Variable->GetInt();
}

template<>
inline ConsoleVariable<float>::ConsoleVariable(
	std::string_view Name,
	std::string_view Description,
	const float&	 DefaultValue)
	: Variable(IConsole::Instance().Register(Name, Description, DefaultValue))
{
}
template<>
inline ConsoleVariable<float>::operator float()
{
	return Variable->GetFloat();
}

template<>
inline ConsoleVariable<std::string>::ConsoleVariable(
	std::string_view   Name,
	std::string_view   Description,
	const std::string& DefaultValue)
	: Variable(IConsole::Instance().Register(Name, Description, DefaultValue))
{
}
template<>
inline ConsoleVariable<std::string>::operator std::string()
{
	return Variable->GetString();
}
