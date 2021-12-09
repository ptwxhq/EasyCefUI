#include "pch.h"
#include "EasyCefAppBrowser.h"

#include "include/cef_browser.h"
#include "include/wrapper/cef_helpers.h"

#include "EasyIPC.h"
#include "EasySchemes.h"
#include "EasyBrowserWorks.h"



void EasyCefAppBrowser::OnContextInitialized()
{
	CEF_REQUIRE_UI_THREAD();

	CefRefPtr<CefRequestContext> request_context = CefRequestContext::GetGlobalContext();
	
	SetRequestDefaultSettings(request_context);

	EasyIPCServer::GetInstance().SetMainThread(GetCurrentThreadId());
	EasyIPCServer::GetInstance().SetWorkCall(std::bind(&EasyBrowserWorks::CommWork, &EasyBrowserWorks::GetInstance(), std::placeholders::_1, std::placeholders::_2));


	//黑暗模式
	auto nDarkMode = GetPrivateProfileIntW(L"Settings", L"DarkMode", 1, g_BrowserGlobalVar.BrowserSettingsPath.c_str());
	SetAllowDarkMode(nDarkMode);
}

void EasyCefAppBrowser::OnBeforeCommandLineProcessing(const CefString& process_type, CefRefPtr<CefCommandLine> command_line)
{
#if CEF_VERSION_MAJOR <= 87
	if (process_type.empty())
	{
		command_line->AppendSwitch("enable-system-flash");
		command_line->AppendSwitch("ppapi-in-process");

		if (!g_BrowserGlobalVar.FlashPluginPath.empty())
		{
			command_line->AppendSwitchWithValue("ppapi-flash-path", g_BrowserGlobalVar.FlashPluginPath);
		}
	}

#endif

	if (process_type.empty())
	{
		command_line->AppendSwitchWithValue("js-flags", "--expose-gc");
	}

	//if (g_BrowserGlobalVar.Debug)
	{
		command_line->AppendSwitch("disable-web-security");
	}

//	command_line->AppendSwitch("disable-web-security");
//	command_line->AppendSwitch("disable-site-isolation-trials");

	//下面的安全设置将来的版本中可能会被去掉

	//iframe跨域cookie
	command_line->AppendSwitchWithValue("disable-features", "SameSiteByDefaultCookies,OutOfBlinkCors");

	command_line->AppendSwitch("disable-web-security");
	command_line->AppendSwitch("disable-site-isolation-trials");



	auto bUseGpu = GetPrivateProfileIntW(L"Settings", L"GPU", 1, g_BrowserGlobalVar.BrowserSettingsPath.c_str());
	if (bUseGpu == 0)
		command_line->AppendSwitch("disable-gpu");

}


void EasyCefAppBrowser::OnRegisterCustomSchemes(CefRawPtr<CefSchemeRegistrar> registrar)
{
	EasyRegisterCustomSchemes(registrar);
}
