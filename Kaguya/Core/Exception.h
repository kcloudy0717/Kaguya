#pragma once
#include <winnt.h>
#include <exception>
#include <string>

inline std::string hresult_to_string(HRESULT hr)
{
	char s_str[64] = {};
	sprintf_s(s_str, "HRESULT of 0x%08X", static_cast<UINT>(hr));
	return std::string(s_str);
}

class hresult_exception : public std::runtime_error
{
public:
	hresult_exception(HRESULT hr) : std::runtime_error(hresult_to_string(hr)), m_hr(hr) {}
	HRESULT Error() const { return m_hr; }
private:
	const HRESULT m_hr;
};
