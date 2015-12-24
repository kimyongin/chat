#include <stdio.h>
#include <tchar.h>
#include <iostream>
#include "../Chat.Server.Backend/Api.h"
#include "MySinkBackend.h"
#include <Windows.h>

int _tmain(int argc, _TCHAR* argv[])
{
	TInit();

	DWORD pHttpServerObj = HTTPServerCreate("13101", "Chat.Server.Frontend");
	HTTPServerStart(pHttpServerObj);

	while (1);

	HTTPServerDestroy(pHttpServerObj);

	TRelease();

	return 0;
}

