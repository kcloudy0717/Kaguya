#include "pch.h"
#include "Exception.h"

Exception::Exception(std::string File, int Line) noexcept
	: m_File(std::move(File))
	, m_Line(Line)
{

}

const char* Exception::what() const noexcept
{
	std::ostringstream oss;
	oss << Type() << std::endl
		<< Origin() << std::endl
		<< Error() << std::endl;
	m_ErrorMessage = oss.str();
	return m_ErrorMessage.data();
}

std::string Exception::Type() const noexcept
{
	return "[Engine Exception]";
}

std::string Exception::Error() const noexcept
{
	return "Unspecified Error";
}

std::string Exception::Origin() const noexcept
{
	std::ostringstream oss;
	oss << "[File] " << m_File << std::endl
		<< "[Line] " << m_Line << std::endl;
	return oss.str();
}

COMException::COMException(std::string File, int Line, HRESULT HR) noexcept
	: Exception(std::move(File), Line)
	, m_HR(HR)
{

}

std::string COMException::Type() const noexcept
{
	return "[COM Exception]";
}

std::string COMException::Error() const noexcept
{
	TCHAR* szErrMsg;

	if (FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		nullptr, m_HR, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&szErrMsg, 0, nullptr) != 0)
	{
		using convert_type = std::codecvt_utf8<wchar_t>;
		std::wstring_convert<convert_type, wchar_t> converter;

		std::string errMsg = converter.to_bytes(szErrMsg);
		LocalFree(szErrMsg);
		return errMsg;
	}

	return std::string("Invalid description for HRESULT");
}