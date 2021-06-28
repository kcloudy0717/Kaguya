#include "pch.h"
#include "Console.h"

template<typename T>
class CConsoleVariable : public ConsoleVariable
{
public:
	CConsoleVariable()
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
void CConsoleVariable<bool>::Set(std::string_view Value)
{
	std::string s = { Value.data(), Value.size() };
	std::transform(s.begin(), s.end(), s.begin(), tolower);
	std::istringstream is(s);
	is >> std::boolalpha >> this->Value;
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
	return float(Value);
}
template<>
std::string CConsoleVariable<int>::GetString() const
{
	return std::to_string(Value);
}
template<>
void CConsoleVariable<int>::Set(std::string_view Value)
{
	this->Value = std::stoi(std::string{ Value.begin(), Value.end() });
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
	return int(Value);
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
void CConsoleVariable<float>::Set(std::string_view Value)
{
	this->Value = std::stof(std::string{ Value.begin(), Value.end() });
}

// string
template<>
bool CConsoleVariable<std::string>::GetBool() const
{
	std::string s = Value;
	std::transform(s.begin(), s.end(), s.begin(), tolower);
	std::istringstream is(s);
	bool			   b;
	is >> std::boolalpha >> b;
	return b;
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
void CConsoleVariable<std::string>::Set(std::string_view Value)
{
	this->Value = std::string{ Value.begin(), Value.end() };
}

template<typename T>
class CConsoleVariableRegistry
{
public:
	CConsoleVariableRegistry(size_t Size) { Registry.reserve(Size); }

	CConsoleVariable<T>* Add(std::string_view Name, std::string_view Description, const T& DefaultValue)
	{
		UINT64 Hash = CityHash64(Name.data(), Name.size());

		CConsoleVariable<T>* CVar = &Registry[Hash];
		CVar->Name				  = Name;
		CVar->Description		  = Description;
		CVar->Value				  = DefaultValue;
		return CVar;
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

class CConsole : public Console
{
public:
	enum
	{
		NumBools   = 1024,
		NumInts	   = 1024,
		NumFloats  = 1024,
		NumStrings = 64
	};

	CConsole()
		: Bools(NumBools)
		, Ints(NumInts)
		, Floats(NumFloats)
		, Strings(NumStrings)
	{
	}

	ConsoleVariable* RegisterVariable(std::string_view Name, std::string_view Description, const bool& DefaultValue)
		override
	{
		return Bools.Add(Name, Description, DefaultValue);
	}

	ConsoleVariable* RegisterVariable(std::string_view Name, std::string_view Description, const int& DefaultValue)
		override
	{
		return Ints.Add(Name, Description, DefaultValue);
	}

	ConsoleVariable* RegisterVariable(std::string_view Name, std::string_view Description, const float& DefaultValue)
		override
	{
		return Floats.Add(Name, Description, DefaultValue);
	}

	ConsoleVariable* RegisterVariable(
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
	CConsoleVariableRegistry<bool>		  Bools;
	CConsoleVariableRegistry<int>		  Ints;
	CConsoleVariableRegistry<float>		  Floats;
	CConsoleVariableRegistry<std::string> Strings;
};

Console& Console::Instance()
{
	static CConsole Singleton;
	return Singleton;
}
