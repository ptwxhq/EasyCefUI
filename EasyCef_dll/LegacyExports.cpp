#include "pch.h"
#include "LegacyExports.h"
#include "LegacyImplement.h"


//临时加入，转间接调用后移除：
#include "EasyWebViewMgr.h"
#include "WebViewControl.h"
#include "EasyIPC.h"

#include <format>


bool QueryNodeAttrib(CefRefPtr<WebViewControl> item, int x, int y, std::string name, std::wstring& outVal);
//////////////////////////////////////////////////////
//旧接口实现

namespace wrapQweb {

	void InitQWeb(FunMap* map)
	{
		WebkitEcho::SetUIFunMap(map);
	}


	void CloseWebview(HWND hWnd)
	{
		auto item = EasyWebViewMgr::GetInstance().GetItemByHwnd(hWnd);
		if (item && item->GetBrowser())
		{
			item->GetBrowser()->GetHost()->CloseBrowser(true);
		}
	}

	void CloseAllWebView()
	{
		EasyWebViewMgr::GetInstance().RemoveAllItems();
	}

	bool UILoadUrl(HWND hwnd, LPCWSTR url)
	{
		auto item = EasyWebViewMgr::GetInstance().GetItemByHwnd(hwnd);
		if (item)
		{
			return item->LoadUrl(url);
		}

		return false;
	}

	HWND CreateWebView(int x, int y, int width, int height, LPCWSTR lpResource, int alpha, bool taskbar, bool trans, int winCombination)
	{
		WebViewExtraAttr ExtAttr;
		ExtAttr.alpha = alpha;
		ExtAttr.transparent = trans;

		ExtAttr.taskbar = taskbar;

		ExtAttr.windowinitstatus = winCombination;


		return EASYCEF::EasyCreateWebUI(nullptr, x, y, width, height, lpResource, &ExtAttr);
	}

	HWND CreateInheritWebView(HWND, int x, int y, int width, int height, LPCWSTR lpResource, int alpha, bool taskbar, bool trans, int winCombination)
	{				
		// 未实现，直接转发
		return CreateWebView(x,y, width, height,  lpResource, alpha,  taskbar, trans, winCombination);
	}

	bool QueryNodeAttrib(HWND hwnd, int x, int y, const char* name, WCHAR* outVal, int len)
	{
		auto item = EasyWebViewMgr::GetInstance().GetItemByHwnd(hwnd);
		if (!item || !item->GetBrowser())
			return false;

		std::wstring res;
		if (QueryNodeAttrib(item, x, y, name ? name : "", res))
		{
			if (res.size() > 0 && len > 0 && len > (int)res.size())
			{
				wcscpy_s(outVal, len, res.c_str());
				return true;
			}
		}

		return false;
	}

	void SetFouceWebView(HWND hWnd, bool fouce)
	{  
		auto item = EasyWebViewMgr::GetInstance().GetItemByHwnd(hWnd);
		if (item && item->GetBrowser())
		{
			return item->GetBrowser()->GetHost()->SendFocusEvent(fouce);
		}
	}

	bool invokedJSMethod(HWND hwnd, const char* utf8_module, const char* utf8_method, const char* utf8_parm, JS_RETURN_WSTR *outstr, const char* utf8_frame_name, bool bNoticeJSTrans2JSON)
	{
		auto item = EasyWebViewMgr::GetInstance().GetItemByHwnd(hwnd);
		if (!item || !item->GetBrowser())
			return false;
		auto& ipcSvr = EasyIPCServer::GetInstance();
		auto hipcli = ipcSvr.GetClientHandle(item->GetBrowser()->GetIdentifier());
		if (hipcli)
		{
			CefRefPtr<CefListValue> valueList = CefListValue::Create();

			int64 frameid = -1;

			if (utf8_frame_name)
			{
				auto frame = item->GetBrowser()->GetFrame(utf8_frame_name);
				frameid = frame->GetIdentifier();
			}

			valueList->SetString(0, utf8_module);
			valueList->SetString(1, utf8_method);
			valueList->SetString(2, utf8_parm);

			valueList->SetBool(3, bNoticeJSTrans2JSON);
		

			auto send = QuickMakeIpcParms(item->GetBrowser()->GetIdentifier(), frameid, "invokedJSMethod", valueList);
			std::string ret;
			if (ipcSvr.SendData(hipcli, send, ret))
			{
				CefString tmp(ret);
				std::wstring wtemp = tmp.ToWString();
				*outstr = new WCHAR[wtemp.size() + 1];
				wcscpy_s(*outstr, wtemp.size()+1, wtemp.c_str());
				return true;

			}
		}
		return false;
	}

	bool asyncInvokedJSMethod(HWND hwnd, const char* utf8_module, const char* utf8_method, const char* utf8_parm, const char* utf8_frame_name, bool bNoticeJSTrans2JSON)
	{
		auto item = EasyWebViewMgr::GetInstance().GetItemByHwnd(hwnd);
		if (!item || !item->GetBrowser())
			return false;
		auto& ipcSvr = EasyIPCServer::GetInstance();
		auto hipcli = ipcSvr.GetClientHandle(item->GetBrowser()->GetIdentifier());
		if (hipcli)
		{
			CefRefPtr<CefListValue> valueList = CefListValue::Create();

			CefRefPtr<CefFrame> frame;

			if (utf8_frame_name)
			{
				frame = item->GetBrowser()->GetFrame(utf8_frame_name);
			}
			else
			{
				frame = item->GetBrowser()->GetMainFrame();
			}

			std::string strParam = utf8_parm ? utf8_parm : "";
			if (strParam.size())
			{
				const auto trim = [](const std::string& s)
				{
					const auto check = [](int c) {
						return !(c < -1 || c > 255 || std::isprint(c));
					};

					auto wsfront = std::find_if_not(s.begin(), s.end(), check);
					auto wsback = std::find_if_not(s.rbegin(), s.rend(), check).base();
					return (wsback <= wsfront ? std::string() : std::string(wsfront, wsback));
				};

				strParam = trim(strParam);
			}

			auto fmt = std::format("window.invokeMethod('{}', '{}', '{}', {})", utf8_module, utf8_method, strParam, bNoticeJSTrans2JSON);

			frame->ExecuteJavaScript(fmt, "", 0);

		//	LOG(INFO) << GetCurrentProcessId() << "] InvokedJSMethod:" << fmt;


			return true;
		}


		return false;
	}

	void FreeJSReturnStr(JS_RETURN_WSTR str)
	{
		if (str)
			delete [] str;
	}

	void clearResolveHost()
	{
		//看看怎么实现通过js的chrome.send('clearHostResolverCache');
		/*
		void NetInternalsMessageHandler::OnClearHostResolverCache(
    const base::ListValue* list) {
  GetNetworkContext()->ClearHostCache(nullptr, base::NullCallback());
	}
		*/
		//或者需要导入v8/NetBenchmarking	chrome.benchmarking.clearHostResolverCache
		// src\chrome\renderer\net_benchmarking_extension.cc
		/*
			static void ClearHostResolverCache(
				const v8::FunctionCallbackInfo<v8::Value>& args) {
				GetNetBenchmarking().ClearHostResolverCache();
			}
		
		*/

	}

	/*bool QueryRenderProcessID(HWND, int& pid)
	{
		return false;
	}

	bool QueryPluginsProcessID(HWND, DWORD* Count, DWORD** plugins_process_ids)
	{
		return false;
	}

	void FreeQueryPluginReturnData(DWORD* plugins_process_ids)
	{
	}*/

	bool GetViewZoomLevel(HWND hwnd, double& level)
	{
		auto item = EasyWebViewMgr::GetInstance().GetItemByHwnd(hwnd);
		if (item)
		{
			level = item->GetZoomLevel();
			return true;
		}
		return false;
	}

	bool SetViewZoomLevel(HWND hwnd, double level)
	{
		auto item = EasyWebViewMgr::GetInstance().GetItemByHwnd(hwnd);
		if (item)
		{
			item->SetZoomLevel(level);
			return true;
		}
		return false;
	}

	//bool PrepareSetEnableNpapi(bool bEnable)
	//{
	//	if (cef_version_info(4) > 87)
	//	{
	//		return false;
	//	}
	//	return false;
	//}

	void PrepareSetCachePath(const LPCWSTR path)
	{
		if (path && path[0])
			g_BrowserGlobalVar.CachePath = path;
	}

	void InitEchoFn(EchoMap* map)
	{
		WebkitEcho::SetFunMap(map);
	}

	void CreateWebControl(HWND hwnd, LPCWSTR url, LPCWSTR cookie)
	{	
		//普通窗口
		EASYCEF::EasyCreateWebControl(hwnd, 0, 0, 0, 0, url, cookie, nullptr);
	}

	bool CloseWebControl(HWND hwnd)
	{
		//普通窗口
		auto item = EasyWebViewMgr::GetInstance().GetItemByHwnd(hwnd);
		if (!item || !item->GetBrowser() || item->IsUIControl())
			return false;

		item->CloseBrowser();


		return true;
	}

	bool LoadUrl(HWND hwnd, LPCWSTR url)
	{
		return UILoadUrl(hwnd, url);
	}

	bool GoBack(HWND hwnd)
	{
		auto item = EasyWebViewMgr::GetInstance().GetItemByHwnd(hwnd);
		if (item)
		{
			item->GoBack();
			return true;
		}

		return false;
	}

	bool GoForward(HWND hwnd)
	{
		auto item = EasyWebViewMgr::GetInstance().GetItemByHwnd(hwnd);
		if (item)
		{
			item->GoForward();
			return true;
		}
		return false;
	}

	bool Reload(HWND hwnd)
	{
		auto item = EasyWebViewMgr::GetInstance().GetItemByHwnd(hwnd);
		if (item)
		{
			item->Reload();
			return true;
		}
		return false;
	}

	bool ReloadIgnoreCache(HWND hwnd)
	{
		auto item = EasyWebViewMgr::GetInstance().GetItemByHwnd(hwnd);
		if (item)
		{
			item->ReloadIgnoreCache();
			return true;
		}
		return false;
	}

	bool IsAudioMuted(HWND hwnd)
	{
		auto item = EasyWebViewMgr::GetInstance().GetItemByHwnd(hwnd);
		if (item)
		{
			return item->IsMuteAudio();
		}
		return false;
	}

	void SetAudioMuted(HWND hwnd, bool bMute)
	{
		auto item = EasyWebViewMgr::GetInstance().GetItemByHwnd(hwnd);
		if (item)
		{
			item->MuteAudio(bMute);
		}
	}

	bool Stop(HWND hwnd)
	{
		auto item = EasyWebViewMgr::GetInstance().GetItemByHwnd(hwnd);
		if (item)
		{
			item->StopLoad();
			return true;
		}
		return false;
	}

	void AdjustRenderSpeed(HWND hWnd, double dbSpeed)
	{
		//目前这个就简单异步传个信息给render，让render自己去设置，省得一些处理
		auto item = EasyWebViewMgr::GetInstance().GetItemByHwnd(hWnd);
		if (!item || !item->GetBrowser())
			return;

		CefRefPtr<CefProcessMessage> msg = CefProcessMessage::Create("AdjustRenderSpeed");
		auto agrs = msg->GetArgumentList();
		agrs->SetSize(1);
		agrs->SetDouble(0, dbSpeed);
		item->GetBrowser()->GetMainFrame()->SendProcessMessage(PID_RENDERER, msg);
	}

	void ClearBrowserData(int combType)
	{
		//貌似只有cookie的可以处理，缓存只能自己删文件？

		if (combType & REMOVE_DATA_MASK_COOKIES)
		{
			class RemoveAllCookie : public CefCookieVisitor
			{
				IMPLEMENT_REFCOUNTING(RemoveAllCookie);
			public:
				virtual bool Visit(const CefCookie& cookie,
					int count,
					int total,
					bool& deleteCookie)
				{
					deleteCookie = true;
					return true;
				};
			};

			CefCookieManager::GetGlobalManager(nullptr)->VisitAllCookies(new RemoveAllCookie);
		}
	}

	void InjectJS(HWND hwnd, LPCWSTR js)
	{
		//TODO 待转换为间接调用形式
		auto item = EasyWebViewMgr::GetInstance().GetItemByHwnd(hwnd);
		if (!item || !item->GetBrowser())
			return;

		auto browser = item->GetBrowser();

		std::vector<int64> ids;
		browser->GetFrameIdentifiers(ids);
		for (auto it : ids)
		{
			auto frame = browser->GetFrame(it);
			if (frame)
			{
				frame->ExecuteJavaScript(js, "", 0);
			}
		}
	}

}
