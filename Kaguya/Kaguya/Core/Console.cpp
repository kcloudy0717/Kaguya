#include "Console.h"

template<typename T>
class ConsoleVariable final : public IConsoleVariable
{
public:
	ConsoleVariable()
	{
		Name		= "<unknown>";
		Description = "<unknown>";
		Value		= {};
	}

	[[nodiscard]] bool		  GetBool() const override;
	[[nodiscard]] int		  GetInt() const override;
	[[nodiscard]] float		  GetFloat() const override;
	[[nodiscard]] std::string GetString() const override;

	void Set(const std::string& Value) override;

	std::string Name;
	std::string Description;
	T			Value;
};

// bool
template<>
bool ConsoleVariable<bool>::GetBool() const
{
	return Value;
}
template<>
int ConsoleVariable<bool>::GetInt() const
{
	return Value ? 1 : 0;
}
template<>
float ConsoleVariable<bool>::GetFloat() const
{
	return Value ? 1.0f : 0.0f;
}
template<>
std::string ConsoleVariable<bool>::GetString() const
{
	return Value ? "true" : "false";
}
template<>
void ConsoleVariable<bool>::Set(const std::string& Value)
{
	std::string Copy = Value;
	std::ranges::transform(Copy.begin(), Copy.end(), Copy.begin(), tolower);
	std::istringstream Stream(Copy);
	Stream >> std::boolalpha >> this->Value;
}

// int
template<>
bool ConsoleVariable<int>::GetBool() const
{
	return Value != 0;
}
template<>
int ConsoleVariable<int>::GetInt() const
{
	return Value;
}
template<>
float ConsoleVariable<int>::GetFloat() const
{
	return static_cast<float>(Value);
}
template<>
std::string ConsoleVariable<int>::GetString() const
{
	return std::to_string(Value);
}
template<>
void ConsoleVariable<int>::Set(const std::string& Value)
{
	this->Value = std::stoi(Value);
}

// float
template<>
bool ConsoleVariable<float>::GetBool() const
{
	return Value != 0.0f;
}
template<>
int ConsoleVariable<float>::GetInt() const
{
	return static_cast<int>(Value);
}
template<>
float ConsoleVariable<float>::GetFloat() const
{
	return Value;
}
template<>
std::string ConsoleVariable<float>::GetString() const
{
	return std::to_string(Value);
}
template<>
void ConsoleVariable<float>::Set(const std::string& Value)
{
	this->Value = std::stof(Value);
}

// string
template<>
bool ConsoleVariable<std::string>::GetBool() const
{
	bool		BoolValue;
	std::string Copy = Value;
	std::ranges::transform(Copy.begin(), Copy.end(), Copy.begin(), tolower);
	std::istringstream Stream(Copy);
	Stream >> std::boolalpha >> BoolValue;
	return BoolValue;
}
template<>
int ConsoleVariable<std::string>::GetInt() const
{
	return std::stoi(Value);
}
template<>
float ConsoleVariable<std::string>::GetFloat() const
{
	return std::stof(Value);
}
template<>
std::string ConsoleVariable<std::string>::GetString() const
{
	return Value;
}
template<>
void ConsoleVariable<std::string>::Set(const std::string& Value)
{
	this->Value = Value;
}

template<typename T>
class ConsoleVariableRegistry
{
public:
	ConsoleVariableRegistry(size_t Size) { Registry.reserve(Size); }

	ConsoleVariable<T>* Add(std::string_view Name, std::string_view Description, const T& DefaultValue)
	{
		UINT64 Hash = CityHash64(Name.data(), Name.size());

		ConsoleVariable<T>* Variable = &Registry[Hash];
		Variable->Name				 = Name;
		Variable->Description		 = Description;
		Variable->Value				 = DefaultValue;
		return Variable;
	}

	ConsoleVariable<T>* Find(std::string_view Name)
	{
		UINT64 Hash = CityHash64(Name.data(), Name.size());

		if (auto iter = Registry.find(Hash); iter != Registry.end())
		{
			return &iter->second;
		}
		return nullptr;
	}

private:
	std::unordered_map<UINT64, ConsoleVariable<T>> Registry;
};

class Console final : public IConsole
{
public:
	static constexpr size_t NumBools   = 1024;
	static constexpr size_t NumInts	   = 1024;
	static constexpr size_t NumFloats  = 1024;
	static constexpr size_t NumStrings = 64;

	IConsoleVariable* Register(std::string_view Name, std::string_view Description, const bool& DefaultValue) override
	{
		return Bools.Add(Name, Description, DefaultValue);
	}

	IConsoleVariable* Register(std::string_view Name, std::string_view Description, const int& DefaultValue) override
	{
		return Ints.Add(Name, Description, DefaultValue);
	}

	IConsoleVariable* Register(std::string_view Name, std::string_view Description, const float& DefaultValue) override
	{
		return Floats.Add(Name, Description, DefaultValue);
	}

	IConsoleVariable* Register(std::string_view Name, std::string_view Description, const std::string& DefaultValue)
		override
	{
		return Strings.Add(Name, Description, DefaultValue);
	}

	AutoConsoleVariable<bool>		 FindBoolCVar(std::string_view Name) override { return Bools.Find(Name); }
	AutoConsoleVariable<int>		 FindIntCVar(std::string_view Name) override { return Ints.Find(Name); }
	AutoConsoleVariable<float>		 FindFloatCVar(std::string_view Name) override { return Floats.Find(Name); }
	AutoConsoleVariable<std::string> FindStringCVar(std::string_view Name) override { return Strings.Find(Name); }

private:
	ConsoleVariableRegistry<bool>		 Bools{ NumBools };
	ConsoleVariableRegistry<int>		 Ints{ NumInts };
	ConsoleVariableRegistry<float>		 Floats{ NumFloats };
	ConsoleVariableRegistry<std::string> Strings{ NumStrings };
};

IConsole& IConsole::Instance()
{
	static Console Console;
	return Console;
}
