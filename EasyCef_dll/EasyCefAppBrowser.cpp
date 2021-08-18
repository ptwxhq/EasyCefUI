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

#if CEF_VERSION_MAJOR <= 87
	CefString errstr;
	CefRefPtr<CefRequestContext> request_context = CefRequestContext::GetGlobalContext();
	//非sandbox的cef在首次加载flash的时候会闪个cmd...后面看看要怎么处理比较好或者不处理

	auto val1 = CefValue::Create();
	val1->SetInt(1);
	auto valtrue = CefValue::Create();
	valtrue->SetBool(true);
	request_context->SetPreference("plugins.run_all_flash_in_allow_mode", valtrue, errstr);
	request_context->SetPreference("profile.default_content_setting_values.plugins", val1, errstr);

	request_context->SetPreference("plugins.allow_outdated", valtrue, errstr);
	request_context->SetPreference("webkit.webprefs.plugins_enabled", valtrue, errstr);

#endif
	EasyIPCServer::GetInstance().SetMainThread(GetCurrentThreadId());
	EasyIPCServer::GetInstance().SetWorkCall(std::bind(&EasyBrowserWorks::CommWork, &EasyBrowserWorks::GetInstance(), std::placeholders::_1, std::placeholders::_2));


	//LOG(INFO) << GetCurrentProcessId() << "] EasyCefAppBrowser::OnContextInitialized()" ;

	return;

//	CefRefPtr<CefCommandLine> command_line =
//		CefCommandLine::GetGlobalCommandLine();
//
//
//	std::string url = command_line->GetSwitchValue("url");
//	if (url.empty())
//		url = "http://js.api.test/?";
////		url = "http://localhost/?";
//
//
//	//HWND h = CreateWindowExW(/*WS_EX_LAYERED*/0, L"STATIC", L"test", WS_POPUP|WS_VISIBLE, 100, 100, 800, 500, 0, 0, 0, 0);
//
//
//	CefWindowInfo window_info;
//
//	window_info.SetAsPopup(nullptr, "EasyCefAppBrowser");
//	window_info.x = 100;
//	window_info.y = 100;
//	window_info.width = 1280;
//	window_info.height = 720;
//	window_info.style =  WS_POPUP | WS_VISIBLE /*| WS_SYSMENU| WS_THICKFRAME| WS_MINIMIZEBOX| WS_MAXIMIZEBOX*/;
//	// WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE;
//	/*
//	(WS_OVERLAPPED     | \
//                             WS_CAPTION        | \
//                             WS_SYSMENU        | \
//                             WS_THICKFRAME     | \
//                             WS_MINIMIZEBOX    | \
//                             WS_MAXIMIZEBOX)
//	*/
//
//	//window_info.SetAsChild(h, { 0,0,700,400 });
//	//window_info.ex_style |= WS_EX_COMPOSITED;
//
//	//window_info.SetAsWindowless(nullptr);
//
//
//
//	//HWND h = CreateWindowExW(WS_EX_LAYERED, L"#32770", L"test", WS_POPUP, 100, 100, 800, 500, 0, 0, 0, 0);
//
////	m_MainWindow = new EasyCefWindow;
//
//	RECT rc = { 100,100,800,500 };
////	m_MainWindow->Create(nullptr, rc);
//
//
//
////	window_info.SetAsWindowless(*m_MainWindow);
//
//	CefBrowserSettings browser_settings;
//
//
//	m_clienthandler = new EasyClientHandler;
//
//
//	// CefRefPtr<CefBrowser> CreateBrowserSync
//
//
//	CefRefPtr<CefDictionaryValue> extra_info = CefDictionaryValue::Create();
//
//	//用于处理部分同步工作
//	// 
//	// 目前不需要browser作为client，且由于现在版本一个Browser可能会对应多个render，如有特殊需要使用cef自带消息异步发送即可
//
//	extra_info->SetInt(IpcBrowserServerKeyName, (int)EasyIPCServer::GetInstance().GetHandle());
//	//	extra_info->SetString(IpcBrowserServerKeyName, g_BrowserGlobalVar.IpcServerName);
//	//extra_info->SetString(IpcRenderServerKeyName, ClientName);
//
//	//LOG(INFO) << GetCurrentProcessId() << "] CefBrowserHost::CreateBrowser:" << g_BrowserGlobalVar.IpcServerName << " To:";
//
//
//	//CreateBrowserSync只能在UI线程调用，CreateBrowser任意
//
//
//	/*auto browser =*/ CefBrowserHost::CreateBrowser(window_info, m_clienthandler, url, browser_settings,
//		extra_info, nullptr);
//
////	m_MainWindow->SetMainBrowser(browser);
//
//	//ShowWindow(h, SW_SHOW);


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

		//
	//	command_line->AppendSwitch("allow-outdated-plugins");

	//	command_line->AppendSwitchWithValue("plugin-policy", "allow");
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


}

CefRefPtr<CefClient> EasyCefAppBrowser::GetDefaultClient()
{
	return m_clienthandler;
}

void EasyCefAppBrowser::OnRegisterCustomSchemes(CefRawPtr<CefSchemeRegistrar> registrar)
{
	EasyRegisterCustomSchemes(registrar);
}
