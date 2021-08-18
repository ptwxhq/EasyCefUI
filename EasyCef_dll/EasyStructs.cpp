#include "pch.h"
#include "EasyStructs.h"

BrowserGlobalVar g_BrowserGlobalVar;

const char* ExtraKeyNameIsUIBrowser = "IsUIRender";
const char* ExtraKeyNameIsManagedPopup = "IsManagedPopup";


void GetLocalPaths()
{
	auto strFileName = std::make_unique<WCHAR[]>(MAXSHORT);
	strFileName[0] = 0;
	GetModuleFileNameW(nullptr/*g_BrowserGlobalVar.hInstance*/, strFileName.get(), MAXSHORT);

	g_BrowserGlobalVar.FilePath = strFileName.get();

	auto ps = g_BrowserGlobalVar.FilePath.rfind('\\');

	g_BrowserGlobalVar.ExecName = &strFileName[ps + 1];

	g_BrowserGlobalVar.FileDir = g_BrowserGlobalVar.FilePath.substr(0, ps + 1);


	if (g_BrowserGlobalVar.hDllInstance && g_BrowserGlobalVar.hInstance != g_BrowserGlobalVar.hDllInstance)
	{
		strFileName[0] = 0;
		GetModuleFileNameW(g_BrowserGlobalVar.hDllInstance, strFileName.get(), MAXSHORT);
	}

	g_BrowserGlobalVar.DllPath = strFileName.get();

}