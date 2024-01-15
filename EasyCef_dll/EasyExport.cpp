#include "pch.h"
#include "EasyExport.h"
#include "EasyCefAppBrowser.h"
#include "EasyCefAppRender.h"
#include "EasySchemes.h"
#include "EasyIPC.h"
#include "EasyWebViewMgr.h"
#include "WebViewControl.h"
#include "EasyReqRespModify.h"
#include "EasySchemes.h"
#include "EasyBrowserWorks.h"



#if defined(CEF_USE_SANDBOX)
#include "include/cef_sandbox_win.h"

CefScopedSandboxInfo g_scoped_sandbox;

#pragma comment(lib, "cef_sandbox.lib")
#pragma comment(lib, "Version.lib")
#pragma comment(lib, "Dbghelp.lib")
#pragma comment(lib, "Winmm.lib")
#pragma comment(lib, "Powrprof.lib")
#pragma comment(lib, "Setupapi.lib")
#pragma comment(lib, "Propsys.lib")
#pragma comment(lib, "delayimp.lib")

#endif

void GetLocalPaths();
extern EasyMemoryFileMgr g_MemFileMgr;

int InnerGetProcessType()
{
	static int ProcessType = 0;
	if (ProcessType != 0)
		return ProcessType;


	CefRefPtr<CefCommandLine> command_line = CefCommandLine::CreateCommandLine();
	command_line->InitFromString(::GetCommandLineW());

	const char kProcessType[] = "type";

	if (command_line->HasSwitch(kProcessType))
	{
		const char* sProcessTypes[] = { "utility",	"utility-sub-type",
			"renderer",  "gpu-process", "ppapi",  "plugin"
		};

		auto type = command_line->GetSwitchValue(kProcessType).ToString();

		for (size_t i = 2; i < _countof(sProcessTypes); i++)
		{
			if (type == sProcessTypes[i])
			{
				ProcessType = i;
				return ProcessType;
			}
		}

		if (type == sProcessTypes[0])
		{
			auto subType = command_line->GetSwitchValue(sProcessTypes[1]).ToString();
			if (subType == "audio.mojom.AudioService")
			{
				ProcessType = 11;
			}
			else if (subType == "network.mojom.NetworkService")
			{
				ProcessType = 12;
			}
			else
			{
				ProcessType = 10;
			}
		}
		else
		{
			ProcessType = -1;
		}
	}
	else
	{
		ProcessType = 1;
	}


	return ProcessType;
}

namespace EASYCEF {

bool RegisterPackDomain(LPCWSTR lpszDomain, LPCWSTR lpszFilePath)
{
	return DomainPackInfo::GetInstance().RegisterPackDomain(lpszDomain, lpszFilePath);

}

void UnregisterPackDomain(LPCWSTR lpszDomain)
{
	DomainPackInfo::GetInstance().UnregisterPackDomain(lpszDomain);
}

void SetSpeedUpWork(SpeedUpWork func)
{
	g_BrowserGlobalVar.funcSpeedupCallback = func;
}

void SetHostResolverWork(SetHostResolver func)
{
	g_BrowserGlobalVar.funcSetHostResolverWork = func;
}

void SetLocalHost(LPCSTR lpszHost, LPCSTR lpszIp)
{
	if (lpszHost && lpszHost[0])
	{
		auto pstr = "";
		if (lpszIp && lpszIp[0])
		{
			pstr = lpszIp;
		}

		EasyBrowserWorks::GetInstance().SetLocalHost(lpszHost, pstr);
	}
}


void SetCloseHandler(CloseHandler func)
{
	g_BrowserGlobalVar.funcCloseCallback = func;
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
	CefShutdown();
}

int GetProcessType()
{
	return InnerGetProcessType();
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

#if CEF_VERSION_MAJOR < 112

	if (g_BrowserGlobalVar.FunctionFlag.bEnableHignDpi)
		CefEnableHighDPISupport();
#endif

	auto strDebugConfigPath = g_BrowserGlobalVar.FileDir + L"debug.dbg";
	g_BrowserGlobalVar.Debug = GetPrivateProfileIntW(L"Debug", L"Debug", 0, strDebugConfigPath.c_str()) == 1;

#if defined(CEF_USE_SANDBOX)
	g_BrowserGlobalVar.sandbox_info = g_scoped_sandbox.sandbox_info();
#endif

	CefMainArgs main_args(hInstance);

	CefRefPtr<CefApp> app;

	int ProcessType = GetProcessType();

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
		else
		{
			app = new EasyCefAppOther;
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

	RegEasyCefSchemes(CefRequestContext::GetGlobalContext());

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

void SetCachePath(LPCWSTR lpszPath)
{
	if (lpszPath && lpszPath[0])
		g_BrowserGlobalVar.CachePath = lpszPath;
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

bool SetXPackWorkCall(XPackExtractWork funWork, XPackFreeData funFree)
{
	if (funWork && funFree)
	{
		g_BrowserGlobalVar.funcXpackExtract = funWork;
		g_BrowserGlobalVar.funcXpackFreeData = funFree;
		return true;
	}

	return false;
}

float GetWindowScaleFactor(HWND hwnd)
{
	return ::GetWindowScaleFactor(hwnd);
}



bool AddMemoryFile(const void* pData, unsigned int nDataLen, size_t* id, LPCWSTR lpszDomain, LPCWSTR lpszExtName)
{
	size_t nid = 0;
	auto ret = g_MemFileMgr.AddMemoryFile(pData, nDataLen, nid, lpszDomain, lpszExtName);
	if (ret && id)
	{
		*id = nid;
	}

	return ret;
}

void DelMemoryFile(size_t id)
{
	g_MemFileMgr.DelMemoryFile(id);
}

bool GetMemoryFileUrl(size_t id, LPWSTR lpszUrl, unsigned int nInLen, unsigned int* nOutLen)
{
	CefString szUrl;
	if (g_MemFileMgr.GetMemoryFileUrl(id, szUrl))
	{
		auto ws = szUrl.ToWString();
		if (lpszUrl)
		{
			auto nLast = std::min(nInLen, ws.size());
			wcsncpy_s(lpszUrl, nInLen, ws.c_str(), nLast);
			lpszUrl[nLast] = 0;
		}

		if (nOutLen)
		{
			*nOutLen = ws.size() + 1;
		}

		return true;
	}

	return false;
}

bool GetMemoryFile(size_t id, void* pData, unsigned int* nLen)
{
	if (!nLen)
		return false;

	bool bSucc = false;
	std::string sData;
	if (g_MemFileMgr.GetData(id, sData))
	{
		if (pData)
		{
			if (*nLen >= sData.size())
			{
				memcpy(pData, sData.data(), sData.size());
				bSucc = true;
			}
		}
	}

	*nLen = sData.size();

	return bSucc;
}

bool GetMemoryByUrl(LPCWSTR lpszUrl, void* pData, unsigned int* nLen)
{
	if (!nLen)
		return false;

	DomainPackInfo::Uri url_parts(lpszUrl);
	std::string data;
	if (g_MemFileMgr.GetDataByUrl(url_parts.Formated(), data))
	{
		
		if (*nLen < data.size())
		{
			return false;
		}

		*nLen = data.size();

		memcpy(pData, data.data(), data.size());

		return true;
	}

	return false;
}

void ForceWebUIPaint(HWND hWnd)
{
	auto item = EasyWebViewMgr::GetInstance().GetItemByHwnd(hWnd);
	if (item)
	{
		auto browser = item->GetBrowser();
		if (browser)
		{
			auto host = browser->GetHost();
			if (host)
			{
				if (item->IsTransparentUI())
				{
					host->WasResized();
					host->Invalidate(PET_VIEW);
				}
				else
				{
					RECT rcSize;
					if (GetClientRect(GetParent(item->GetHWND()), &rcSize))
					{
						SetWindowPos(item->GetHWND(), nullptr, 0, 0, rcSize.right, rcSize.bottom, SWP_NOZORDER | SWP_NOMOVE | SWP_ASYNCWINDOWPOS);
					}
				}
			}
			
		
		}
	}
}

bool RegisterJSFunction(bool bSync, LPCSTR lpszFunctionName, jscall_UserFunction func, void* context, int RegType)
{
	if (func && lpszFunctionName && lpszFunctionName[0])
	{
		return EasyBrowserWorks::GetInstance().RegisterUserJSFunction(lpszFunctionName, func, bSync, context, RegType);
	}

	return false;
}

char* CreateEasyString(size_t nLen)
{
	if (nLen)
	{
		return new char[nLen];
	}

	return nullptr;
}

void FreeEasyString(const char* str)
{
	if (str)
	{
		delete[] str;
	}
}

bool GetUserFuncContext(UserFuncContext* data)
{
	return EasyBrowserWorks::GetInstance().GetBrowserFrameContext(data);
}

bool SetAddContextMenuCall(CallBeforeContextMenu func, CallDoMenuCommand fundo)
{
	if (func && fundo)
	{
		g_BrowserGlobalVar.funcBeforeContextMenu = func;
		g_BrowserGlobalVar.funcDoMenuCommand = fundo;
		return true;
	}

	return false;
}

void SetDOMCompleteStatusCallback(CallDOMCompleteStatus func)
{
	g_BrowserGlobalVar.funcCallDOMCompleteStatus = func;
}

void SetNativeCompleteStatusCallback(CallNativeCompleteStatus func)
{
	g_BrowserGlobalVar.funcCallNativeCompleteStatus = func;
}

void SetPopNewUrlCallback(CallPopNewUrl func)
{
	g_BrowserGlobalVar.funcPopNewUrlCallback = func;
}

void SetLoadErrorHandler(LoadErrorHandler func)
{
	g_BrowserGlobalVar.funcLoadErrorCallback = func;
}

void SetDownloadHandler(BeforeDownloadHandler func, DownloadStatusHandler funcStatus)
{
	g_BrowserGlobalVar.funcBeforeDownloadCallback = func;
	g_BrowserGlobalVar.funcDownloadStatusCallback = funcStatus;
}

void SetFrameLoadStatusCallback(FrameLoadStatus func)
{
	g_BrowserGlobalVar.funcFrameLoadStatusCallback = (void(*)(HWND, int, int, void*))func;
}

void ExecuteJavaScript(HWND hWnd, LPCWSTR lpszFrameName, LPCWSTR lpszJSCode)
{
	auto item = EasyWebViewMgr::GetInstance().GetItemByHwnd(hWnd);
	if (!item || !item->GetBrowser())
		return;

	auto browser = item->GetBrowser();

	std::vector<int64_t> ids;

	if (lpszFrameName && lpszFrameName[0])
	{
		auto frame = item->GetBrowser()->GetFrame(lpszFrameName);
		ids.push_back(frame->GetIdentifier());
	}
	else
	{
		browser->GetFrameIdentifiers(ids);
	}


	for (auto it : ids)
	{
		auto frame = browser->GetFrame(it);
		if (frame)
		{
			frame->ExecuteJavaScript(lpszJSCode, "", 0);
		}
	}
}

bool ExecuteJavaScriptByFid(HWND hWnd, long long nFrameId, LPCSTR lpszJSCode)
{
	auto item = EasyWebViewMgr::GetInstance().GetItemByHwnd(hWnd);
	if (!item || !item->GetBrowser())
		return false;

	auto browser = item->GetBrowser();

	auto frame = browser->GetFrame(nFrameId);
	if (frame)
	{
		frame->ExecuteJavaScript(lpszJSCode, "", 0);
		return true;
	}

	return false;
}

static bool InvokeJSFunction(HWND hWnd, bool bSync, LPCSTR lpszJSFunctionName, LPCSTR lpszFrameName, std::string* pstrRet, DWORD dwTimeout, LPCSTR parmfmt, va_list args)
{
	auto item = EasyWebViewMgr::GetInstance().GetItemByHwnd(hWnd);
	if (!item || !item->GetBrowser())
		return false;

	const char* pCur = parmfmt;

	auto list = CefListValue::Create();
	list->SetString(0, lpszJSFunctionName);

	int nCount = 1;
	bool bCheckOk = true;
	while (*pCur && bCheckOk)
	{
		switch (*pCur)
		{
		case 's':
			{
				auto s = va_arg(args, const char*);
				if (!s)s = "";
				list->SetString(nCount, s);
			}
			break;
		case 'w':
			{
				auto s = va_arg(args, const wchar_t*);
				if (!s)s = L"";
				list->SetString(nCount, s);
			}
			break;
		case 'i':
			list->SetInt(nCount, va_arg(args, int));
			break;
		case 'f':
			list->SetDouble(nCount, va_arg(args, double));
			break;
		case 'b':
			list->SetBool(nCount, va_arg(args, int));
			break;
		case 'u':
			{
				va_arg(args, int);
				list->SetNull(nCount);
			}
			break;
		default:
			bCheckOk = false;
			break;
		}

		++pCur;
		++nCount;
	}

	va_end(args);

	if (!bCheckOk)
		return false;


	CefRefPtr<CefFrame> frame;

	int64_t frameid = -1;
	if (lpszFrameName && lpszFrameName[0])
	{
		frame = item->GetBrowser()->GetFrame(lpszFrameName);
		frameid = frame->GetIdentifier();
	}
	else
	{
		frame = item->GetBrowser()->GetMainFrame();
	}

	if (bSync)
	{
		auto& ipcSvr = EasyIPCServer::GetInstance();
		auto hipcli = ipcSvr.GetClientHandle(item->GetBrowser()->GetIdentifier());
		if (!hipcli)
			return false;

		auto send = QuickMakeIpcParms(item->GetBrowser()->GetIdentifier(), frameid, dwTimeout ? GetTimeNowMS(dwTimeout) : 0, "__InvokedJSMethod__", list);
		std::string ret;
		OutputDebugStringA(send.c_str());
		if (ipcSvr.SendData(hipcli, send, ret, dwTimeout))
		{
			if (pstrRet)
			{
				*pstrRet = std::move(ret);
			}
			return true;
		}
	}
	else
	{

		std::string jsRun = lpszJSFunctionName;
		jsRun += '(';
		for (size_t i = 1; i < list->GetSize(); i++)
		{
			auto val = list->GetValue(i);
			switch (val->GetType())
			{

			case VTYPE_NULL:
				jsRun += "null";
				break;
			case VTYPE_BOOL:
				jsRun += val->GetBool() ? "true" : "false";
				break;
			case VTYPE_INT:
				jsRun += std::to_string(val->GetInt());
				break;
			case VTYPE_DOUBLE:
				jsRun += std::to_string(val->GetDouble());
				break;
			case VTYPE_STRING:
				//需要转义一下
				jsRun += ArrangeJsonString(val->GetString());
				break;
			default:
				assert(0);
				break;
			}

			jsRun += ',';
		}

		if (jsRun.back() == ',')
		{
			jsRun.back() = ')';
		}
		else
		{
			jsRun += ')';
		}

		frame->ExecuteJavaScript(jsRun, "", 0);
		return true;
	}




	return false;
}

bool InvokeJSFunction(HWND hWnd, LPCSTR lpszJSFunctionName, LPCSTR lpszFrameName, const char** ppStrRet, DWORD dwTimeout, LPCSTR parmfmt, ...)
{
	thread_local std::string strRet;
	va_list args;
	va_start(args, parmfmt);
	bool bRet = InvokeJSFunction(hWnd, true, lpszJSFunctionName, lpszFrameName, &strRet, dwTimeout, parmfmt, args);

	if (ppStrRet)
	{
		*ppStrRet = strRet.c_str();
	}

	return bRet;
}

bool InvokeJSFunctionAsync(HWND hWnd, LPCSTR lpszJSFunctionName, LPCSTR lpszFrameName, LPCSTR parmfmt, ...)
{
	va_list args;
	va_start(args, parmfmt);
	return InvokeJSFunction(hWnd, false, lpszJSFunctionName, lpszFrameName, nullptr, /*0*/15000, parmfmt, args);
}

void AdjustRenderProcessSpeed(HWND hWnd, double dbSpeed)
{
	//目前这个就简单异步传个信息给render，让render自己去设置，省得一些处理
	auto item = EasyWebViewMgr::GetInstance().GetItemByHwnd(hWnd);
	if (!item || !item->GetBrowser())
		return;

	CefRefPtr<CefProcessMessage> msg = CefProcessMessage::Create("__AdjustRenderSpeed__");
	auto agrs = msg->GetArgumentList();
	agrs->SetSize(1);
	agrs->SetDouble(0, dbSpeed);
	item->GetBrowser()->GetMainFrame()->SendProcessMessage(PID_RENDERER, msg);
}

void CleanCookies()
{
	class RemoveAllCookie : public CefCookieVisitor
	{
		IMPLEMENT_REFCOUNTING(RemoveAllCookie);
	public:
		bool Visit(const CefCookie& cookie,
			int count,
			int total,
			bool& deleteCookie) override
		{
			deleteCookie = true;
			return true;
		};
	};

	CefCookieManager::GetGlobalManager(nullptr)->VisitAllCookies(new RemoveAllCookie);
}

bool EasyLoadUrl(HWND hWnd, LPCWSTR lpszUrl)
{
	auto item = EasyWebViewMgr::GetInstance().GetItemByHwnd(hWnd);
	if (item)
	{
		return item->LoadUrl(lpszUrl);
	}

	return false;
}

void EasyCloseAllWeb()
{
	EasyWebViewMgr::GetInstance().RemoveAllItems();
}

void SetWindowAlpha(HWND hWnd, unsigned char val)
{
	CefRefPtr<WebViewUIControl> item = dynamic_cast<WebViewUIControl*>(EasyWebViewMgr::GetInstance().GetItemByHwnd(hWnd).get());
	if (!item)
		return;

	item->SetAlpha(val);
}

bool WebControlDoWork(HWND hWnd, WEBCONTROLWORK dowork)
{
	auto item = EasyWebViewMgr::GetInstance().GetItemByHwnd(hWnd);
	if (item)
	{
		switch (dowork)
		{
		case EASYCEF::WC_GOBACK:
			item->GoBack();
			break;
		case EASYCEF::WC_GOFOWARD:
			item->GoForward();
			break;
		case EASYCEF::WC_RELOAD:
			item->Reload();
			break;
		case EASYCEF::WC_RELOADIGNORECACHE:
			item->ReloadIgnoreCache();
			break;
		case EASYCEF::WC_STOP:
			item->StopLoad();
			break;
		case EASYCEF::WC_MUTEAUDIO:
			item->MuteAudio(true);
			break;
		case EASYCEF::WC_UNMUTEAUDIO:
			item->MuteAudio(false);
			break;
		default:
			return false;
		}

		return true;
	}


	return false;
}

bool GetViewZoomLevel(HWND hWnd, double& level)
{
	auto item = EasyWebViewMgr::GetInstance().GetItemByHwnd(hWnd);
	if (item)
	{
		level = item->GetZoomLevel();
		return true;
	}

	return false;
}

bool SetViewZoomLevel(HWND hWnd, double level)
{
	auto item = EasyWebViewMgr::GetInstance().GetItemByHwnd(hWnd);
	if (item)
	{
		item->SetZoomLevel(level);
		return true;
	}

	return false;
}

void SetWebControlCallbacks(WebControlCallbacks *callbacks)
{
	g_BrowserGlobalVar.funcWebControlCreated = callbacks->WebCreated;
	g_BrowserGlobalVar.funcWebControLoadingState = callbacks->WebLoadingState;
	g_BrowserGlobalVar.funcWebControlUrlChange = callbacks->WebUrlChange;
	g_BrowserGlobalVar.funcWebControlTitleChange = callbacks->WebTitleChange;
	g_BrowserGlobalVar.funcWebControlLoadBegin = callbacks->WebLoadBegin;
	g_BrowserGlobalVar.funcWebControlLoadEnd = callbacks->WebLoadEnd;
	g_BrowserGlobalVar.funcWebControlFavIconChange = callbacks->WebFavIconChange;
	g_BrowserGlobalVar.funcWebControlBeforeClose = callbacks->WebBeforeClose;
	g_BrowserGlobalVar.funcWebControlBeforePopup = callbacks->WebBeforePopup;
}


/*


*/

}