#include "Console.h"

template<typename T>
class ConsoleVariable : public IConsoleVariable
{
public:
	ConsoleVariable()
	{
		Name		= "<unknown>";
		Description = "<unknown>";
		Value		= {};
	}

	bool		GetBool() const override;
	int			GetInt() const override;
	float		GetFloat() const override;
	std::string GetString() const override;

	void Set(std::string_view Value) override;

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
void ConsoleVariable<bool>::Set(std::string_view Value)
{
	std::string s = { Value.data(), Value.size() };
	std::transform(s.begin(), s.end(), s.begin(), tolower);
	std::istringstream is(s);
	is >> std::boolalpha >> this->Value;
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
	return float(Value);
}
template<>
std::string ConsoleVariable<int>::GetString() const
{
	return std::to_string(Value);
}
template<>
void ConsoleVariable<int>::Set(std::string_view Value)
{
	this->Value = std::stoi(std::string{ Value.begin(), Value.end() });
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
	return int(Value);
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
void ConsoleVariable<float>::Set(std::string_view Value)
{
	this->Value = std::stof(std::string{ Value.begin(), Value.end() });
}

// string
template<>
bool ConsoleVariable<std::string>::GetBool() const
{
	std::string s = Value;
	std::transform(s.begin(), s.end(), s.begin(), tolower);
	std::istringstream is(s);
	bool			   b;
	is >> std::boolalpha >> b;
	return b;
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
void ConsoleVariable<std::string>::Set(std::string_view Value)
{
	this->Value = std::string{ Value.begin(), Value.end() };
}

template<typename T>
class ConsoleVariableRegistry
{
public:
	ConsoleVariableRegistry(size_t Size) { Registry.reserve(Size); }

	ConsoleVariable<T>* Add(std::string_view Name, std::string_view Description, const T& DefaultValue)
	{
		UINT64 Hash = CityHash64(Name.data(), Name.size());

		ConsoleVariable<T>* CVar = &Registry[Hash];
		CVar->Name				 = Name;
		CVar->Description		 = Description;
		CVar->Value				 = DefaultValue;
		return CVar;
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

class Console : public IConsole
{
public:
	enum
	{
		NumBools   = 1024,
		NumInts	   = 1024,
		NumFloats  = 1024,
		NumStrings = 64
	};

	Console()
		: Bools(NumBools)
		, Ints(NumInts)
		, Floats(NumFloats)
		, Strings(NumStrings)
	{
	}

	IConsoleVariable* RegisterVariable(std::string_view Name, std::string_view Description, const bool& DefaultValue)
		override
	{
		return Bools.Add(Name, Description, DefaultValue);
	}

	IConsoleVariable* RegisterVariable(std::string_view Name, std::string_view Description, const int& DefaultValue)
		override
	{
		return Ints.Add(Name, Description, DefaultValue);
	}

	IConsoleVariable* RegisterVariable(std::string_view Name, std::string_view Description, const float& DefaultValue)
		override
	{
		return Floats.Add(Name, Description, DefaultValue);
	}

	IConsoleVariable* RegisterVariable(
		std::string_view   Name,
		std::string_view   Description,
		const std::string& DefaultValue) override
	{
		return Strings.Add(Name, Description, DefaultValue);
	}

	AutoConsoleVariable<bool> FindBoolCVar(std::string_view Name) override { return Bools.Find(Name); }

	AutoConsoleVariable<int> FindIntCVar(std::string_view Name) override { return Ints.Find(Name); }

	AutoConsoleVariable<float> FindFloatCVar(std::string_view Name) override { return Floats.Find(Name); }

	AutoConsoleVariable<std::string> FindStringCVar(std::string_view Name) override { return Strings.Find(Name); }

private:
	ConsoleVariableRegistry<bool>		 Bools;
	ConsoleVariableRegistry<int>		 Ints;
	ConsoleVariableRegistry<float>		 Floats;
	ConsoleVariableRegistry<std::string> Strings;
};

IConsole& IConsole::Instance()
{
	static Console Singleton;
	return Singleton;
}
