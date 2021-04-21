#include <Windows.h>
#include <string>
#include <vector>
#include "Resizer.h"

int wmain(int argc, wchar_t* argv[])
{
	std::vector<std::wstring> params(argv, argv + argc);
	std::wstring filePath;

	if (params.size() < 2)
	{
		//wprintf(L"Usage: %s <filepath>\n", params[0].c_str());
		//return 1;
		filePath = L"sample.png";
	}

    CoInitialize(NULL);

	Resizer resizer;
	resizer.Init();
    resizer.ReadFile(filePath);
    resizer.DrawFrame();

    CoUninitialize();
	return 0;
}