#include "Console.h"

template<typename T>
class CConsoleVariable final : public IConsoleObject
{
public:
	CConsoleVariable()
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
bool CConsoleVariable<bool>::GetBool() const
{
	return Value;
}
template<>
int CConsoleVariable<bool>::GetInt() const
{
	return Value ? 1 : 0;
}
template<>
float CConsoleVariable<bool>::GetFloat() const
{
	return Value ? 1.0f : 0.0f;
}
template<>
std::string CConsoleVariable<bool>::GetString() const
{
	return Value ? "true" : "false";
}
template<>
void CConsoleVariable<bool>::Set(const std::string& Value)
{
	std::string Copy = Value;
	std::ranges::transform(Copy.begin(), Copy.end(), Copy.begin(), tolower);
	std::istringstream Stream(Copy);
	Stream >> std::boolalpha >> this->Value;
}

// int
template<>
bool CConsoleVariable<int>::GetBool() const
{
	return Value != 0;
}
template<>
int CConsoleVariable<int>::GetInt() const
{
	return Value;
}
template<>
float CConsoleVariable<int>::GetFloat() const
{
	return static_cast<float>(Value);
}
template<>
std::string CConsoleVariable<int>::GetString() const
{
	return std::to_string(Value);
}
template<>
void CConsoleVariable<int>::Set(const std::string& Value)
{
	this->Value = std::stoi(Value);
}

// float
template<>
bool CConsoleVariable<float>::GetBool() const
{
	return Value != 0.0f;
}
template<>
int CConsoleVariable<float>::GetInt() const
{
	return static_cast<int>(Value);
}
template<>
float CConsoleVariable<float>::GetFloat() const
{
	return Value;
}
template<>
std::string CConsoleVariable<float>::GetString() const
{
	return std::to_string(Value);
}
template<>
void CConsoleVariable<float>::Set(const std::string& Value)
{
	this->Value = std::stof(Value);
}

// string
template<>
bool CConsoleVariable<std::string>::GetBool() const
{
	bool		BoolValue;
	std::string Copy = Value;
	std::ranges::transform(Copy.begin(), Copy.end(), Copy.begin(), tolower);
	std::istringstream Stream(Copy);
	Stream >> std::boolalpha >> BoolValue;
	return BoolValue;
}
template<>
int CConsoleVariable<std::string>::GetInt() const
{
	return std::stoi(Value);
}
template<>
float CConsoleVariable<std::string>::GetFloat() const
{
	return std::stof(Value);
}
template<>
std::string CConsoleVariable<std::string>::GetString() const
{
	return Value;
}
template<>
void CConsoleVariable<std::string>::Set(const std::string& Value)
{
	this->Value = Value;
}

template<typename T>
class ConsoleVariableRegistry
{
public:
	ConsoleVariableRegistry(size_t Size) { Registry.reserve(Size); }

	CConsoleVariable<T>* Add(std::string_view Name, std::string_view Description, const T& DefaultValue)
	{
		UINT64 Hash = CityHash64(Name.data(), Name.size());

		CConsoleVariable<T>* Variable = &Registry[Hash];
		Variable->Name				  = Name;
		Variable->Description		  = Description;
		Variable->Value				  = DefaultValue;
		return Variable;
	}

	CConsoleVariable<T>* Find(std::string_view Name)
	{
		UINT64 Hash = CityHash64(Name.data(), Name.size());

		if (auto iter = Registry.find(Hash); iter != Registry.end())
		{
			return &iter->second;
		}
		return nullptr;
	}

private:
	std::unordered_map<UINT64, CConsoleVariable<T>> Registry;
};

class CConsole final : public IConsole
{
public:
	static constexpr size_t NumBools   = 1024;
	static constexpr size_t NumInts	   = 1024;
	static constexpr size_t NumFloats  = 1024;
	static constexpr size_t NumStrings = 64;

	IConsoleObject* Register(std::string_view Name, std::string_view Description, const bool& DefaultValue) override
	{
		return Bools.Add(Name, Description, DefaultValue);
	}

	IConsoleObject* Register(std::string_view Name, std::string_view Description, const int& DefaultValue) override
	{
		return Ints.Add(Name, Description, DefaultValue);
	}

	IConsoleObject* Register(std::string_view Name, std::string_view Description, const float& DefaultValue) override
	{
		return Floats.Add(Name, Description, DefaultValue);
	}

	IConsoleObject* Register(std::string_view Name, std::string_view Description, const std::string& DefaultValue)
		override
	{
		return Strings.Add(Name, Description, DefaultValue);
	}

	ConsoleVariable<bool>		 FindBoolCVar(std::string_view Name) override { return Bools.Find(Name); }
	ConsoleVariable<int>		 FindIntCVar(std::string_view Name) override { return Ints.Find(Name); }
	ConsoleVariable<float>		 FindFloatCVar(std::string_view Name) override { return Floats.Find(Name); }
	ConsoleVariable<std::string> FindStringCVar(std::string_view Name) override { return Strings.Find(Name); }

private:
	ConsoleVariableRegistry<bool>		 Bools{ NumBools };
	ConsoleVariableRegistry<int>		 Ints{ NumInts };
	ConsoleVariableRegistry<float>		 Floats{ NumFloats };
	ConsoleVariableRegistry<std::string> Strings{ NumStrings };
};

IConsole& IConsole::Instance()
{
	static CConsole Console;
	return Console;
}
