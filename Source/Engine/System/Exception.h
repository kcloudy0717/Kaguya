#pragma once
#include <stdexcept>
#include <string_view>

class Exception : public std::exception
{
public:
	Exception(std::string_view File, int Line);

	const char* what() const noexcept final;

protected:
	virtual const char* GetErrorType() const noexcept;
	virtual std::string GetError() const;

private:
	std::string GetExceptionOrigin() const;
	std::string GetErrorString() const;

private:
	std::string_view File;
	int				 Line;

	mutable std::string Error;
};
