#include "Converter.h"
#include <Windows.h>

std::string Converter::ansi_to_utf8(const std::string& source)
{
	// ansi to utf-16
	INT buffer1_length = 
	::MultiByteToWideChar(CP_ACP, 0, source.c_str(), source.length(), NULL, NULL);
	LPWSTR buffer1 = new WCHAR[buffer1_length + 1]; 
	memset(buffer1, 0, sizeof(WCHAR)* (buffer1_length + 1));
	::MultiByteToWideChar(CP_ACP, 0, source.c_str(), source.length(), buffer1, buffer1_length + 1);

	// utf-16 to utf-8
	INT buffer2_length = 
	::WideCharToMultiByte(CP_UTF8, 0, buffer1, buffer1_length, NULL, 0, NULL, NULL);
	LPSTR buffer2 = new CHAR[buffer2_length + 1]; 
	memset(buffer2, 0, sizeof(CHAR)* (buffer2_length + 1));
	::WideCharToMultiByte(CP_UTF8, 0, buffer1, buffer1_length, buffer2, buffer2_length + 1, NULL, NULL);

	std::string result(buffer2);
	delete[] buffer1;
	delete[] buffer2;

	return result;
}