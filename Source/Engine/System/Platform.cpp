#include "Platform.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <strsafe.h>

void Platform::Exit(std::wstring_view Message)
{
	// https://docs.microsoft.com/en-us/windows/win32/debug/retrieving-the-last-error-code?redirectedfrom=MSDN

	LPCTSTR lpszFunction = Message.data();

	// Retrieve the system error message for the last-error code
	LPVOID lpMsgBuf;
	DWORD  dw = GetLastError();

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0,
		nullptr);

	// Display the error message and exit the process
	LPVOID lpDisplayBuf = LocalAlloc(LMEM_ZEROINIT, (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen(lpszFunction) + 40) * sizeof(TCHAR));
	StringCchPrintf(
		(LPTSTR)lpDisplayBuf,
		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		TEXT("%s failed with error %d: %s"),
		lpszFunction,
		dw,
		lpMsgBuf);
	MessageBox(nullptr, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
	ExitProcess(dw);
}

void Platform::Utf8ToWide(std::string_view Utf8, std::wstring& Wide)
{
	int Utf8Length = static_cast<int>(Utf8.size());
	int Size	   = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, Utf8.data(), Utf8Length, nullptr, 0);
	Wide.resize(Size);
	MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, Utf8.data(), Utf8Length, Wide.data(), Size);
}

void Platform::WideToUtf8(std::wstring_view Wide, std::string& Utf8)
{
	int WideLength = static_cast<int>(Wide.size());
	int Size	   = WideCharToMultiByte(CP_UTF8, 0, Wide.data(), WideLength, nullptr, 0, nullptr, nullptr);
	Utf8.resize(Size);
	WideCharToMultiByte(CP_UTF8, 0, Wide.data(), WideLength, Utf8.data(), Size, nullptr, nullptr);
}
