#include <Windows.h>
#include <string>
#include <vector>
#include <memory>
#include "Resizer.h"

int wmain(int argc, wchar_t* argv[])
{
	std::vector<std::wstring> params(argv, argv + argc);
	SIZE targetSize = { 1920, 1080 };

	if (params.size() == 3)
	{
		try
		{
			targetSize.cx = std::stoi(argv[1]);
			targetSize.cy = std::stoi(argv[2]);
		}
		catch (const std::exception&)
		{
			wprintf(L"Could not convert to int.\n");
		}
	}

	if (FAILED(CoInitializeEx(nullptr, COINIT_MULTITHREADED)))
		return -1;

	std::unique_ptr<Resizer> pResizer = std::make_unique<Resizer>();
	pResizer->InitDx();
	pResizer->Prepare(targetSize);
    pResizer->ReadFile(L"sample.png");
    pResizer->Draw();
	pResizer->SaveFile(L"out.png");

    CoUninitialize();
	return 0;
}