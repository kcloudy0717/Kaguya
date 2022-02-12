#pragma once
#include "Delegate.h"
#include <string_view>

class IConsoleObject
{
public:
	IConsoleObject(std::string_view Name);
	virtual ~IConsoleObject() = default;

	[[nodiscard]] virtual bool		  GetBool() const	= 0;
	[[nodiscard]] virtual int		  GetInt() const	= 0;
	[[nodiscard]] virtual float		  GetFloat() const	= 0;
	[[nodiscard]] virtual std::string GetString() const = 0;

	virtual void Set(const std::string& Value) = 0;

	void SetBool(bool Value) { Set(Value ? std::string("true") : std::string("false")); }
	void SetInt(int Value) { Set(std::to_string(Value)); }
	void SetFloat(float Value) { Set(std::to_string(Value)); }
	void SetString(const std::string& Value) { Set(Value); }

protected:
	std::string_view Name;
};

template<typename T>
class ConsoleVariable : public IConsoleObject
{
public:
	ConsoleVariable(std::string_view Name, std::string_view Description, const T& DefaultValue)
		: IConsoleObject(Name)
		, Description(Description)
		, Value(DefaultValue)
	{
	}

	operator auto() { return Get(); }

	T Get()
	{
		if constexpr (std::is_same_v<T, bool>)
		{
			return GetBool();
		}
		else if constexpr (std::is_same_v<T, int>)
		{
			return GetInt();
		}
		else if constexpr (std::is_same_v<T, float>)
		{
			return GetFloat();
		}
		else if constexpr (std::is_same_v<T, std::string>)
		{
			return GetString();
		}
		else
		{
			return T();
		}
	}

	[[nodiscard]] bool GetBool() const override
	{
		if constexpr (std::is_same_v<T, bool>)
		{
			return Value;
		}
		else if constexpr (std::is_same_v<T, int>)
		{
			return Value != 0;
		}
		else if constexpr (std::is_same_v<T, float>)
		{
			return Value != 0.0f;
		}
		else if constexpr (std::is_same_v<T, std::string>)
		{
			bool		BoolValue;
			std::string Copy = Value;
			std::ranges::transform(Copy.begin(), Copy.end(), Copy.begin(), tolower);
			std::istringstream Stream(Copy);
			Stream >> std::boolalpha >> BoolValue;
			return BoolValue;
		}
		else
		{
			return T();
		}
	}
	[[nodiscard]] int GetInt() const override
	{
		if constexpr (std::is_same_v<T, bool>)
		{
			return Value ? 1 : 0;
		}
		else if constexpr (std::is_same_v<T, int>)
		{
			return Value;
		}
		else if constexpr (std::is_same_v<T, float>)
		{
			return static_cast<int>(Value);
		}
		else if constexpr (std::is_same_v<T, std::string>)
		{
			return std::stoi(Value);
		}
		else
		{
			return T();
		}
	}
	[[nodiscard]] float GetFloat() const override
	{
		if constexpr (std::is_same_v<T, bool>)
		{
			return Value ? 1.0f : 0.0f;
		}
		else if constexpr (std::is_same_v<T, int>)
		{
			return static_cast<float>(Value);
		}
		else if constexpr (std::is_same_v<T, float>)
		{
			return Value;
		}
		else if constexpr (std::is_same_v<T, std::string>)
		{
			return std::stof(Value);
		}
		else
		{
			return T();
		}
	}
	[[nodiscard]] std::string GetString() const override
	{
		if constexpr (std::is_same_v<T, bool>)
		{
			return Value ? "true" : "false";
		}
		else if constexpr (std::is_same_v<T, int>)
		{
			return std::to_string(Value);
		}
		else if constexpr (std::is_same_v<T, float>)
		{
			return std::to_string(Value);
		}
		else if constexpr (std::is_same_v<T, std::string>)
		{
			return Value;
		}
		else
		{
			return T();
		}
	}

	void Set(const std::string& Value) override
	{
		if constexpr (std::is_same_v<T, bool>)
		{
			std::string Copy = Value;
			std::ranges::transform(Copy.begin(), Copy.end(), Copy.begin(), tolower);
			std::istringstream Stream(Copy);
			Stream >> std::boolalpha >> this->Value;
		}
		else if constexpr (std::is_same_v<T, int>)
		{
			this->Value = std::stoi(Value);
		}
		else if constexpr (std::is_same_v<T, float>)
		{
			this->Value = std::stof(Value);
		}
		else if constexpr (std::is_same_v<T, std::string>)
		{
			this->Value = Value;
		}
	}

private:
	std::string_view Description;
	T				 Value;
};

class ConsoleCommand : public IConsoleObject
{
public:
	using TArgs			  = const std::vector<std::string_view>&;
	using CommandCallback = Delegate<void(TArgs)>;

	ConsoleCommand(std::string_view Name, CommandCallback&& Callback)
		: IConsoleObject(Name)
		, Callback(std::move(Callback))
	{
	}

	bool Execute(TArgs Args) const
	{
		try
		{
			Callback(Args);
			return true;
		}
		catch (...)
		{
			return false;
		}
	}

	[[nodiscard]] bool		  GetBool() const override { return {}; }
	[[nodiscard]] int		  GetInt() const override { return {}; }
	[[nodiscard]] float		  GetFloat() const override { return {}; }
	[[nodiscard]] std::string GetString() const override { return {}; }

	void Set(const std::string& Value) override {}

private:
	CommandCallback Callback;
};

class IConsole
{
public:
	static IConsole* GetConsole();

	virtual ~IConsole() = default;

	virtual void Register(std::string_view Name, IConsoleObject* Object) = 0;
};
