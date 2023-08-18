// EasyDemo.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include "D:\GitHub\EasyCefUI\EasyCef_dll\EasyExport.h"
#include <string>
#include <map>

#ifdef _DEBUG
#pragma comment(lib, "EasyCef_d.lib")
#else
#pragma comment(lib, "EasyCef.lib")
#endif // DEBUG


std::map<HWND, long> g_winSign;

int user_sync(HWND hWnd, LPCSTR jsonParams, char** strRet, void* context)
{

	if (EASYCEF::InvokeJSFunction(hWnd, "invokeFromNative", nullptr, nullptr, 0, "sib", "string parm", 999, true))
	{

	}
	

	return 0;
}

int user_async(HWND hWnd, LPCSTR jsonParams, char**, void* context)
{
	

	return 0;
}

void BeforeContextMenu(HWND hWnd, int x, int y, bool bIsEdit, EASYCEF::FuncAddContextMenu call, void* callneed)
{
	if (bIsEdit)
	{
		//addrightbutton

		call(callneed, -1, nullptr, true, true);
		call(callneed, 30004, L"测试文本", true, true);


	}
	else
	{
		call(callneed, -1, nullptr, true, true);
		call(callneed, 30005, L"普通按钮", true, true);
	}

}

bool DoContextMenu(HWND hWnd, int command)
{
	switch (command)
	{
	case 30004:
		MessageBox(hWnd, L"30004", 0, 0);
		return true;

	case 30005:
		MessageBox(hWnd, L"30005", 0, 0);
		return true;

	default:
		break;
	}

	return false;
}

bool BeforeDownload(HWND hWnd, unsigned int nId, LPCWSTR lpszUrl, LPCWSTR lpszSuggestFileName)
{
	std::cout << "BeforeDownload -> " << nId << "  " << lpszUrl << " | " << lpszSuggestFileName << "\n";
	return false;
}

bool DownloadStatus(unsigned int nId, long long nDownloadedSize, long long nTotalSize)
{
	std::cout << "DownloadStatus -> " << nId << "  " << nDownloadedSize << "/" << nTotalSize << "\n";
	bool bContinue = true;
	return bContinue;
}


int main()
{
	std::cout << "Hello World!\n";

	//Init---------------------------------------------
	EASYCEF::SetCachePath(L"");

	EasyCefFunctionFlag eff;
	eff.bUIImeFollow = true;
	EASYCEF::SetFunctionFlag(&eff);

	EASYCEF::EasyInitConfig config;
	config.bSupportLayerWindow = true;
	config.strLocal = L"zh-CN";


	EASYCEF::RegisterJSFunction(true, "userjs_sync", user_sync, (void*)1, 3);
	EASYCEF::RegisterJSFunction(false, "userjs_async", user_async,(void*)2, 3);


	EASYCEF::SetAddContextMenuCall(BeforeContextMenu, DoContextMenu);
	EASYCEF::SetDownloadHandler(BeforeDownload, DownloadStatus);

	EASYCEF::InitEasyCef(GetModuleHandle(nullptr), L"EasyRender.exe", &config);

	//CreateWindow----------------------------
	WebViewExtraAttr ExtAttr;
	ExtAttr.alpha = 255;
	ExtAttr.transparent = true;
	ExtAttr.taskbar = true;
	ExtAttr.windowinitstatus = ATTR_ENABLE_DRAG_DROP;
	ExtAttr.dwAddStyle |= WS_MINIMIZEBOX | WS_THICKFRAME | WS_MAXIMIZEBOX;

	WCHAR strCurPath[MAX_PATH];
	GetModuleFileName(nullptr, strCurPath, MAX_PATH);
	*(wcsrchr(strCurPath, '\\') + 1) = 0;

	HWND hMainWindow = EASYCEF::EasyCreateWebUI(nullptr, 100, 100, 900, 800, (strCurPath + std::wstring(L"index.html")).c_str(), &ExtAttr);
	g_winSign[hMainWindow] = 1;

	//Run--------------------------------------
	EASYCEF::RunMsgLoop();

	//Clean------------------------------------
	EASYCEF::ShutEasyCef();

}
