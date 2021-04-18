#pragma once
#include <winnt.h>
#include <exception>
#include <string>

class Exception : public std::exception
{
public:
	explicit Exception(std::string File, int Line) noexcept;
	virtual ~Exception() = default;

	const char* what() const noexcept override;

	virtual std::string Type() const noexcept;
	virtual std::string Error() const noexcept;
	std::string Origin() const noexcept;
protected:
	mutable std::string m_ErrorMessage;
private:
	std::string m_File;
	int m_Line;
};

class COMException final : public Exception
{
public:
	explicit COMException(std::string File, int Line, HRESULT HR) noexcept;
	~COMException() override = default;

	std::string Type() const noexcept override;
	std::string Error() const noexcept override;
private:
	HRESULT m_HR;
};

#ifndef ThrowIfFailed
#define ThrowIfFailed(HR)								\
do														\
{														\
	HRESULT hr = (HR);									\
	if (FAILED(hr))										\
	{													\
		throw COMException(__FILE__, __LINE__, hr);		\
	}													\
} while(false)
#endif