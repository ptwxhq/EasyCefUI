#include "pch.h"
#include "Export.h"
#include "EasyCefAppBrowser.h"
#include "EasyCefAppRender.h"
#include "EasySchemes.h"
#include "EasyIPC.h"
#include "EasyWebViewMgr.h"
#include "WebViewControl.h"
#include "EasyReqRespModify.h"
#include "extlib/pack.h"



#if defined(CEF_USE_SANDBOX)
#include "include/cef_sandbox_win.h"

CefScopedSandboxInfo g_scoped_sandbox;

#pragma comment(lib, "cef_sandbox.lib")
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Version.lib")
#pragma comment(lib, "Dbghelp.lib")
#pragma comment(lib, "Winmm.lib")
#pragma comment(lib, "Powrprof.lib")
#pragma comment(lib, "Setupapi.lib")
#pragma comment(lib, "Propsys.lib")
#pragma comment(lib, "delayimp.lib")

#endif

void GetLocalPaths();

namespace EASYCEF {

bool RegisterPackDomain(LPCWSTR lpszDomain, LPCWSTR lpszFilePath)
{
	return DomainPackInfo::GetInstance().RegisterPackDomain(lpszDomain, lpszFilePath);

}

void UnregisterPackDomain(LPCWSTR lpszDomain)
{
	DomainPackInfo::GetInstance().UnregisterPackDomain(lpszDomain);
}

//bool AddCrossOriginWhitelistEntry(LPCWSTR source_origin, LPCWSTR target_protocol, LPCWSTR target_domain, bool allow_target_subdomains)
//{
//	return CefAddCrossOriginWhitelistEntry(source_origin, target_protocol, target_domain, allow_target_subdomains);
//}
//
//bool RemoveCrossOriginWhitelistEntry(LPCWSTR source_origin, LPCWSTR target_protocol, LPCWSTR target_domain, bool allow_target_subdomains)
//{
//	return CefRemoveCrossOriginWhitelistEntry(source_origin, target_protocol, target_domain, allow_target_subdomains);
//}

void SetSpeedUpWork(SpeedUpWork func)
{
	g_BrowserGlobalVar.funcSpeedupCallback = reinterpret_cast<void*>(func);
}

void SetCloseHandler(CloseHandler func)
{
	g_BrowserGlobalVar.funcCloseCallback = reinterpret_cast<void*>(func);
}

void RunMsgLoop()
{
	CefRunMessageLoop();
}

void QuitMsgLoop()
{
	//先清理所有item，避免异常残留
	EasyWebViewMgr::GetInstance().RemoveAllItems();
	//关闭服务
	EasyIPCServer::GetInstance().Stop();

	CefQuitMessageLoop();
}

void ShutEasyCef()
{
	CleanLoadedPacks(nullptr);
	CefShutdown();
}

int InitEasyCef(HINSTANCE hInstance, LPCWSTR lpRender, PEASYINITCONFIG pConf)
{
	static bool bInited = false;
	if (bInited)
	{
		return 1;
	}

	bInited = true;

	g_BrowserGlobalVar.hInstance = hInstance;

	DWORD dwParentPid = 0;
	GetParentProcessInfo(&dwParentPid, nullptr);

	{
		using fnRtlGetNtVersionNumbers = void (WINAPI*)(LPDWORD major, LPDWORD minor, LPDWORD build);
		auto RtlGetNtVersionNumbers = reinterpret_cast<fnRtlGetNtVersionNumbers>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlGetNtVersionNumbers"));
		if (RtlGetNtVersionNumbers)
		{
			RtlGetNtVersionNumbers(&g_BrowserGlobalVar.WindowsVerMajor, &g_BrowserGlobalVar.WindowsVerMinor, &g_BrowserGlobalVar.WindowsVerBuild);
			g_BrowserGlobalVar.WindowsVerBuild &= ~0xF0000000;
		}
	}

	GetLocalPaths();

	CefEnableHighDPISupport();

	auto strDebugConfigPath = g_BrowserGlobalVar.FileDir + L"debug.dbg";
	g_BrowserGlobalVar.Debug = GetPrivateProfileIntW(L"Debug", L"Debug", 0, strDebugConfigPath.c_str()) == 1;

#if defined(CEF_USE_SANDBOX)
	g_BrowserGlobalVar.sandbox_info = g_scoped_sandbox.sandbox_info();
#endif

	CefMainArgs main_args(hInstance);

	CefRefPtr<CefCommandLine> command_line = CefCommandLine::CreateCommandLine();
	command_line->InitFromString(::GetCommandLineW());

	CefRefPtr<CefApp> app;

	int ProcessType = 0;
	if (pConf)
	{
		ProcessType = pConf->ProcessType;
	}

	if (ProcessType == 0)
	{
		if (!command_line->HasSwitch("type"))
		{
			ProcessType = 1;
		}
		else
		{
			const std::string& processType = command_line->GetSwitchValue("type");
			if (processType == "renderer")
			{
				ProcessType = 2;
			}
			else
			{
				ProcessType = 3;
			}
		}
	}


	if (ProcessType == 1)
	{
		app = new EasyCefAppBrowser;
		g_BrowserGlobalVar.IsBrowserProcess = true;
	}
	else
	{
		if (ProcessType == 2)
		{
			app = new EasyCefAppRender;
		}

		if (dwParentPid)
		{
			std::thread([dwParentPid] {
				//5秒超时强退
				HANDLE hParent = OpenProcess(SYNCHRONIZE, FALSE, dwParentPid);

				DWORD dwRes = WaitForSingleObject(hParent, INFINITE);
				CloseHandle(hParent);
				if (WAIT_OBJECT_0 == dwRes)
				{
					std::this_thread::sleep_for(std::chrono::seconds(5));
					OutputDebugStringA("ghost process timeout...");
					TerminateProcess(GetCurrentProcess(), -1);
				}

				}).detach();

		}
		
	}

	int exit_code = CefExecuteProcess(main_args, app, g_BrowserGlobalVar.sandbox_info);
	if (exit_code >= 0) {
		return exit_code;
	}

	CefSettings settings;

#if !defined(CEF_USE_SANDBOX)
	settings.no_sandbox = true;
#endif

	if (pConf)
	{
		if (pConf->bSupportLayerWindow)
		{
			settings.windowless_rendering_enabled = true;
			g_BrowserGlobalVar.SupportLayerWindow = true;
		}

		if (pConf->strLocal && pConf->strLocal[0])
		{
			CefString(&settings.locale).FromWString(pConf->strLocal);
		}

		if (pConf->strUserAgent && pConf->strUserAgent[0])
		{
			CefString(&settings.user_agent).FromWString(pConf->strUserAgent);
		}

		if (pConf->strWebViewClassName && pConf->strWebViewClassName[0])
		{
			g_BrowserGlobalVar.WebViewClassName = pConf->strWebViewClassName;
		}

		if (pConf->strUILoadingTitle && pConf->strUILoadingTitle[0])
		{
			g_BrowserGlobalVar.UILoadingWindowTitle = pConf->strUILoadingTitle;
		}
	}

#if !defined(CEF_USE_SANDBOX)
	//其余情况将使用自身同名进程
	if (lpRender && wcslen(lpRender))
	{
		std::wstring strRenderPath;
		if (PathIsRelativeW(lpRender))
		{
			strRenderPath = g_BrowserGlobalVar.FileDir + lpRender;
		}
		else
		{
			strRenderPath = lpRender;
		}

		//if (_waccess(strRenderPath.c_str(), 0) == 0) 
		//已经设置了如果文件丢失打不开界面就好，如果直接使用自身更容易出问题
		{
			//设置渲染进程exe
			CefString(&settings.browser_subprocess_path).FromWString(strRenderPath);
		}
	}
#endif

	//缓存路径
	if (g_BrowserGlobalVar.CachePath.empty())
	{
		g_BrowserGlobalVar.CachePath = GetDefAppDataFolder();
	}

	CefString(&settings.cache_path).FromWString(g_BrowserGlobalVar.CachePath);

	settings.log_severity = (cef_log_severity_t)GetPrivateProfileIntW(L"Debug", L"Log", LOGSEVERITY_DISABLE, strDebugConfigPath.c_str());

	EasyIPCServer::GetInstance().ThdRun();

	CefInitialize(main_args, settings, app.get(), g_BrowserGlobalVar.sandbox_info);

	RegEasyCefSchemes();

	SetCloseHandler(QuitMsgLoop);



	return exit_code;
}

void EasyCreateWebControl(HWND hParent, int x, int y, int width, int height, LPCWSTR url, LPCWSTR cookiepath, const WebViewExtraAttr* pExt)
{
	RECT rc = { x, y, x + width, y + height };
	VERIFY(EasyWebViewMgr::GetInstance().CreateWebViewControl(hParent, rc, url, cookiepath, pExt));
}
HWND EasyCreateWebUI(HWND hParent, int x, int y, int width, int height, LPCWSTR url, const WebViewExtraAttr* pExt)
{
	if (pExt && pExt->transparent)
	{
		if (!g_BrowserGlobalVar.SupportLayerWindow)
			return nullptr;
	}

	RECT rc = { x, y, x + width, y + height };

	auto handle = EasyWebViewMgr::GetInstance().CreateWebViewUI(hParent, rc, url, 0, pExt);


	if (handle)
	{
		auto item = EasyWebViewMgr::GetInstance().GetItemBrowserByHandle(handle);

		if (item)
		{
			return item->GetHWND();
		}

	}

	return nullptr;
}

void SetFunctionFlag(EasyCefFunctionFlag* pFlag)
{
	if (pFlag)
	{
		memcpy(&g_BrowserGlobalVar.FunctionFlag, pFlag, sizeof(EasyCefFunctionFlag));
	}
	else
	{
		EasyCefFunctionFlag def;
		memcpy(&g_BrowserGlobalVar.FunctionFlag, &def, sizeof(EasyCefFunctionFlag));
	}
}

void SetFlashPluginPath(LPCWSTR lpszPath)
{
	if (lpszPath && lpszPath[0] && _waccess(lpszPath, 0) == 0)
	{
		g_BrowserGlobalVar.FlashPluginPath = lpszPath;
	}
}

unsigned AddReqRspRule(const EasyReqRspRule* pRule)
{
	EasyReplaceRule	r1;
	r1.bAddIfNotExist = pRule->bAddIfNotExist;
	r1.bContinueSearch = pRule->bContinueSearch;
	r1.bReplaceCaseInsensitive = pRule->bReplaceCaseInsensitive;
	r1.ModifyType = static_cast<RULE_MODIFY_TYPE>(pRule->ModifyType);
	r1.strHeadField = pRule->strHeadField ? pRule->strHeadField : "";
	r1.strReplace = pRule->strReplace ? pRule->strReplace : "";
	r1.SetUrlMatchType(pRule->strUrlMathInfo ? pRule->strUrlMathInfo : "", static_cast<RULE_MATCH_TYPE>(pRule->MatchType));
	r1.SetSearchContents(pRule->strSearch ? pRule->strSearch : "", pRule->bReplaceUseRegex);

	auto id = EasyReqRespModifyMgr::GetInstance().AddRule(r1);

	return id;
}


bool SetReqRspRule(unsigned id, const EasyReqRspRule* pRule)
{
	auto rule = EasyReqRespModifyMgr::GetInstance().FindRule(id);
	if (rule)
	{
		EasyReplaceRule& r1 = *rule;
		r1.bAddIfNotExist = pRule->bAddIfNotExist;
		r1.bContinueSearch = pRule->bContinueSearch;
		r1.bReplaceCaseInsensitive = pRule->bReplaceCaseInsensitive;
		r1.ModifyType = static_cast<RULE_MODIFY_TYPE>(pRule->ModifyType);
		r1.strHeadField = pRule->strHeadField ? pRule->strHeadField : "";
		r1.strReplace = pRule->strReplace ? pRule->strReplace : "";
		r1.SetUrlMatchType(pRule->strUrlMathInfo ? pRule->strUrlMathInfo : "", static_cast<RULE_MATCH_TYPE>(pRule->MatchType));
		r1.SetSearchContents(pRule->strSearch ? pRule->strSearch : "", pRule->bReplaceUseRegex);
		return true;
	}
	return false;
}

bool SetReqRspOrder(unsigned id, long offset, int origin)
{
	return EasyReqRespModifyMgr::GetInstance().SetOrderInfo(id, offset, origin);
}
bool DelReqRspRule(unsigned id)
{
	return EasyReqRespModifyMgr::GetInstance().DelRule(id);
}

bool GetReqRspRule(unsigned id, EasyReqRspRule* pRule)
{
	auto rule = EasyReqRespModifyMgr::GetInstance().FindRule(id);
	if (rule)
	{
		pRule->bAddIfNotExist = rule->bAddIfNotExist;
		pRule->bContinueSearch = rule->bContinueSearch;
		pRule->bReplaceCaseInsensitive = rule->bReplaceCaseInsensitive;
		pRule->ModifyType = static_cast<EasyReqRspRule::RULE_MODIFY_TYPE>(rule->ModifyType);
		pRule->strHeadField = rule->strHeadField.c_str();
		pRule->strReplace = rule->strReplace.c_str();
		pRule->strUrlMathInfo = rule->GetMatchInfo().c_str();
		pRule->bReplaceUseRegex = rule->GetIsReplaceUseRegex();
		pRule->strSearch = rule->GetSearchInfo().c_str();
		pRule->MatchType = static_cast<EasyReqRspRule::RULE_MATCH_TYPE>(rule->GetMatchType());
		return true;
	}

	return false;
}


}