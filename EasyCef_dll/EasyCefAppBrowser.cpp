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

	auto request_context = CefRequestContext::GetGlobalContext();

	SetRequestDefaultSettings(request_context);

	EasyIPCServer::GetInstance().SetMainThread(GetCurrentThreadId());
	EasyIPCServer::GetInstance().SetWorkCall(std::bind(&EasyBrowserWorks::CommWork, &EasyBrowserWorks::GetInstance(), std::placeholders::_1, std::placeholders::_2));


	//黑暗模式
	auto nDarkMode = GetPrivateProfileIntW(L"Settings", L"DarkMode", 1, g_BrowserGlobalVar.BrowserSettingsPath.c_str());
	SetAllowDarkMode(nDarkMode);
}

void EasyCefAppBrowser::OnBeforeCommandLineProcessing(const CefString& process_type, CefRefPtr<CefCommandLine> command_line)
{
	std::vector<std::string> vecDisableFeatures;

	if (process_type.empty())
	{
#if CEF_VERSION_MAJOR <= 87

		command_line->AppendSwitch("enable-system-flash");
		command_line->AppendSwitch("ppapi-in-process");

		if (!g_BrowserGlobalVar.FlashPluginPath.empty())
		{
			command_line->AppendSwitchWithValue("ppapi-flash-path", g_BrowserGlobalVar.FlashPluginPath);
		}
#endif

		command_line->AppendSwitchWithValue("js-flags", "--expose-gc");

		//目前无法处理多屏选择的情况
		command_line->AppendSwitch("use-fake-ui-for-media-stream");
	}


	auto bUseGpu = GetPrivateProfileIntW(L"Settings", L"GPU", 1, g_BrowserGlobalVar.BrowserSettingsPath.c_str());
	if (bUseGpu == 0)
		command_line->AppendSwitch("disable-gpu");

	auto nSecurity = GetPrivateProfileIntW(L"Settings", L"Security", 0, g_BrowserGlobalVar.BrowserSettingsPath.c_str());

	if (nSecurity == 0)
	{
		//下面的安全设置将来的版本中可能会被去掉
		//iframe跨域cookie
		vecDisableFeatures.push_back("SameSiteByDefaultCookies");
		vecDisableFeatures.push_back("OutOfBlinkCors");
		command_line->AppendSwitch("disable-web-security");
		command_line->AppendSwitch("disable-site-isolation-trials");
	}

	if (g_BrowserGlobalVar.DarkModeType == PreferredAppMode::ForceDark)
	{
		command_line->AppendSwitch("force-dark-mode");
	}
	else if (g_BrowserGlobalVar.DarkModeType == PreferredAppMode::ForceLight)
	{
		//目前看只有强制设置黑色的，接下来就是走系统页面自选，没有简单的非破坏处理方式，或许可以设置高对比度+hook api
	}


	std::string strDisableFeatures;

	for (auto& it : vecDisableFeatures)
	{
		strDisableFeatures += it;
		strDisableFeatures += ",";
	}

	if (!strDisableFeatures.empty())
	{
		strDisableFeatures.pop_back();

		command_line->AppendSwitchWithValue("disable-features", strDisableFeatures);
	}


}


void EasyCefAppBrowser::OnRegisterCustomSchemes(CefRawPtr<CefSchemeRegistrar> registrar)
{
	EasyRegisterCustomSchemes(registrar);
}
