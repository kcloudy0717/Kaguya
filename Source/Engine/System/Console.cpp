#include "Console.h"
#include "Hash.h"

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
		u64 Hash	   = Hash::Hash64(Name.data(), Name.size());
		Registry[Hash] = Object;
	}

private:
	std::unordered_map<u64, IConsoleObject*> Registry;
};

IConsole* IConsole::GetConsole()
{
	static CConsole Instance;
	return &Instance;
}
