#include "Console.h"
#include <city.h>

IConsoleObject::IConsoleObject(std::string_view Name)
	: Name(Name)
{
	IConsole::GetConsole()->Register(Name, this);
}

class CConsole final : public IConsole
{
public:
	void Register(std::string_view Name, IConsoleObject* Object) override
	{
		UINT64 Hash	   = CityHash64(Name.data(), Name.size());
		Registry[Hash] = Object;
	}

private:
	std::unordered_map<UINT64, IConsoleObject*> Registry;
};

IConsole* IConsole::GetConsole()
{
	static CConsole Instance;
	return &Instance;
}
