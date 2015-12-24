#pragma once

#include <Windows.h>

LONG WINAPI DefinedUnhandledExceptionFilter(__in struct _EXCEPTION_POINTERS *ExceptionInfo);
LONG WINAPI DefinedUnhandledExceptionFilterForDump(__in struct _EXCEPTION_POINTERS *ExceptionInfo);