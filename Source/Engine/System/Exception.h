#pragma once
#include <stdexcept>
#include <string_view>

class Exception : public std::exception
{
public:
	Exception(std::string_view File, int Line);

	virtual const char* GetErrorType() const noexcept;
	virtual std::string GetError() const;

	const char* what() const noexcept final;

private:
	std::string GetExceptionOrigin() const;

	std::string GetErrorString() const;

private:
	std::string_view File;
	int				 Line;

	mutable std::string Error;
};
