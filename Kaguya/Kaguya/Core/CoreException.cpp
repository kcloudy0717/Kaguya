#include "CoreException.h"
#include <sstream>

CoreException::CoreException(const char* File, int Line)
	: File(File)
	, Line(Line)
{
}

const char* CoreException::what() const noexcept
{
	Error = GetErrorString();
	return Error.data();
}

const char* CoreException::GetErrorType() const noexcept
{
	return "[Core]";
}

std::string CoreException::GetError() const
{
	return "Unknown Exception";
}

std::string CoreException::GetExceptionOrigin() const
{
	std::ostringstream oss;
	oss << "[File] " << File << std::endl << "[Line] " << Line;
	return oss.str();
}

std::string CoreException::GetErrorString() const
{
	std::ostringstream oss;
	oss << GetErrorType() << std::endl << GetExceptionOrigin() << std::endl << GetError();
	return oss.str();
}
