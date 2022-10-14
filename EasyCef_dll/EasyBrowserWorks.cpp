#include "pch.h"
#include "EasyBrowserWorks.h"
#include "EasyWebViewMgr.h"
#include "WebViewControl.h"
#include "LegacyImplement.h"


inline void WriteJSON(CefRefPtr<CefValue>& node, CefString& retval)
{
	retval = CefWriteJSON(node, JSON_WRITER_DEFAULT);
}


namespace BrowserSyncWorkFunctions {

	bool crossInvokeWebMethod(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefRefPtr<CefListValue>& args, CefString& retval)
	{
		auto item = EasyWebViewMgr::GetInstance().GetItemBrowserById(browser->GetIdentifier());
		if (!item)
			return false;

		HWND hWnd = item->GetHWND();
		if (WebkitEcho::getUIFunMap() && WebkitEcho::getUIFunMap()->crossInvokeWebMethod && IsWindow(hWnd))
		{
			int sign = args->GetInt(0);
			std::wstring module = args->GetString(1).ToWString();
			std::wstring method = args->GetString(2).ToWString();
			std::wstring parm = args->GetString(3).ToWString();
			bool tojson = args->GetBool(4);

			retval = WebkitEcho::getUIFunMap()->crossInvokeWebMethod(hWnd, sign, module.c_str(), method.c_str(), parm.c_str(), tojson);
			return true;
			
		}

		return false;
	}

	bool crossInvokeWebMethod2(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefRefPtr<CefListValue>& args, CefString& retval)
	{
		auto item = EasyWebViewMgr::GetInstance().GetItemBrowserById(browser->GetIdentifier());
		if (!item)
			return false;

		HWND hWnd = item->GetHWND();
		if (WebkitEcho::getUIFunMap() && WebkitEcho::getUIFunMap()->crossInvokeWebMethod2 && IsWindow(hWnd))
		{
			std::wstring randstr = args->GetString(0).ToWString();
			int sign = args->GetInt(1);
			std::wstring frame = args->GetString(2).ToWString();
			std::wstring module = args->GetString(3).ToWString();
			std::wstring method = args->GetString(4).ToWString();
			std::wstring parm = args->GetString(5).ToWString();
			bool tojson = args->GetBool(6);

			retval = WebkitEcho::getUIFunMap()->crossInvokeWebMethod2((HWND)randstr.c_str(), sign, frame.c_str(), module.c_str(), method.c_str(), parm.c_str(), tojson);
			return true;

		}

		return false;
	}

	bool invokeMethod(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefRefPtr<CefListValue>& args, CefString& retval)
	{
		//LOG(INFO) << GetCurrentProcessId() << "] invokeMethod: in";

		auto item = EasyWebViewMgr::GetInstance().GetItemBrowserById(browser->GetIdentifier());
		if (!item)
			return false;

		HWND hWnd = item->GetHWND();

		if (!WebkitEcho::getUIFunMap() || !IsWindow(hWnd))
			return false;

		std::wstring module = args->GetString(0).ToWString();
		std::wstring method = args->GetString(1).ToWString();
		std::wstring parm = args->GetString(2).ToWString();
		int extra = args->GetInt(3);

		//LOG(INFO) << GetCurrentProcessId() << "] invokeMethod:" << module << "|" << method;


		if (item->IsUIControl())
		{
			if (WebkitEcho::getUIFunMap()->invokeMethod)
			{
				retval = WebkitEcho::getUIFunMap()->invokeMethod(hWnd, module.c_str(), method.c_str(), parm.c_str(), extra);
				return true;
			}
		}
		else
		{
			if (WebkitEcho::getFunMap()->webkitInvokeMethod)
			{
				retval = WebkitEcho::getFunMap()->webkitInvokeMethod(browser->GetIdentifier(), module.c_str(), method.c_str(), parm.c_str(), extra);
				return true;
			}
		}

		return false;
	}

	bool winProty(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefRefPtr<CefListValue>& args, CefString& retval)
	{
		auto item = EasyWebViewMgr::GetInstance().GetItemBrowserById(browser->GetIdentifier());
		if (!item)
			return false;

		HWND hWnd = item->GetHWND();
		if (WebkitEcho::getUIFunMap() && WebkitEcho::getUIFunMap()->winProty && IsWindow(hWnd))
		{
			retval = WebkitEcho::getUIFunMap()->winProty(hWnd);
			return true;
		}

		return false;
	}

	//仅供以下setProfile/getProfile使用
	static std::unordered_map<size_t, std::wstring> g_tempprofile;
	bool setProfile(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefRefPtr<CefListValue>& args, CefString& retval)
	{
		auto key = std::hash<std::wstring>{}(args->GetString(0).ToWString());
		
		g_tempprofile[key] = args->GetString(1).ToWString();

		return true;
	}

	bool getProfile(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefRefPtr<CefListValue>& args, CefString& retval)
	{
		auto ret = CefValue::Create();
		auto key = std::hash<std::wstring>{}(args->GetString(0).ToWString());

		auto it = g_tempprofile.find(key);
		if (it == g_tempprofile.end())
		{
			ret->SetString("");
		}
		else
		{
			ret->SetString(it->second);
		}

		WriteJSON(ret, retval);

		return true;
	}

	bool getSoftwareAttribute(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefRefPtr<CefListValue>& args, CefString& retval)
	{
		if (WebkitEcho::getUIFunMap() && WebkitEcho::getUIFunMap()->softAttr)
		{
			int idx = args->GetInt(0);
			auto ret = CefValue::Create();
			ret->SetString(WebkitEcho::getUIFunMap()->softAttr(idx));
			WriteJSON(ret, retval);
			return true;
		}

		return false;
	}

	bool __V8AccessorAttribKeys__(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefRefPtr<CefListValue>& args, CefString& retval)
	{
		return EasyBrowserWorks::GetInstance().DoJSKeySyncWork(args->GetString(0).ToString(), browser, frame, retval);
	}
}

namespace BrowserSyncWorkJSKeys
{
	void appDataPath(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefString& retval)
	{
		retval = g_BrowserGlobalVar.CachePath;
	}


}


namespace BrowserAsyncWorkFunctions {

	void setWindowPos(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefRefPtr<CefListValue>& args)
	{
		auto item = EasyWebViewMgr::GetInstance().GetItemBrowserById(browser->GetIdentifier());
		if (!item)
			return;

		HWND hWnd = item->GetHWND();
		if (WebkitEcho::getUIFunMap() && WebkitEcho::getUIFunMap()->setWindowPos && IsWindow(hWnd))
		{
			int order = args->GetInt(0);
			int x = args->GetInt(1);
			int y = args->GetInt(2);
			int width = args->GetInt(3);
			int height = args->GetInt(4);
			int flag = args->GetInt(5);

			WebkitEcho::getUIFunMap()->setWindowPos(hWnd, order, x, y, width, height, flag);
		}
	}

#define DEF_TRANS_VALUE true
	struct JSCreateWindowParam
	{
		bool trans = DEF_TRANS_VALUE;
		int alpha = 255;
		int x = 0;
		int y = 0;
		int width = 0;
		int height = 0;
		int min_cx = 0;
		int min_cy = 0;
		int max_cx = 0;
		int max_cy = 0;
		int ulStyle = 0;
		int parentSign = 0;
		int extra = 0;
		std::wstring skin;
	};

	bool ParseCreateWindowParam(const CefString& strParam, JSCreateWindowParam* objParam)
	{
		auto values = CefParseJSON(strParam, JSON_PARSER_RFC);
		if (!values)
			return false;

		auto root = values->GetDictionary();

		if (!root)
			return false;

#define GETKEYVALVE_INT(key) if (root->HasKey(#key))\
		objParam->key = root->GetInt(#key)


		if (root->HasKey("trans"))
		{
			objParam->trans = root->GetBool("trans");
		}

		if (root->HasKey("skin"))
		{
			objParam->skin = root->GetString("skin").ToWString();
		}

		GETKEYVALVE_INT(x);
		GETKEYVALVE_INT(y);
		GETKEYVALVE_INT(width);
		GETKEYVALVE_INT(height);
		GETKEYVALVE_INT(min_cx);
		GETKEYVALVE_INT(min_cy);
		GETKEYVALVE_INT(max_cx);
		GETKEYVALVE_INT(max_cy);
		GETKEYVALVE_INT(alpha);
		GETKEYVALVE_INT(ulStyle);
		GETKEYVALVE_INT(extra);
		GETKEYVALVE_INT(parentSign);

		return true;
	}


	void createWindow(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefRefPtr<CefListValue>& args)
	{
		auto item = EasyWebViewMgr::GetInstance().GetItemBrowserById(browser->GetIdentifier());
		if (!item)
			return;

		HWND hWnd = item->GetHWND();
		if (WebkitEcho::getUIFunMap() && WebkitEcho::getUIFunMap()->createWindow && IsWindow(hWnd))
		{
			if (args->GetSize() == 1)
			{
				JSCreateWindowParam parm;
				if (ParseCreateWindowParam(args->GetString(0), &parm))
				{
					WebkitEcho::getUIFunMap()->createWindow(hWnd, parm.x, parm.y, parm.width, parm.height, parm.min_cx, parm.min_cy, parm.max_cx, parm.max_cy, parm.skin.c_str(), parm.alpha, parm.ulStyle, parm.trans, parm.extra);
				}
			}
			else
			{
				//无法获取原始字符串？参数都解开了，那就这样吧
				WebkitEcho::getUIFunMap()->createWindow(hWnd, args->GetInt(0), args->GetInt(1), args->GetInt(2), args->GetInt(3), args->GetInt(4), args->GetInt(5), args->GetInt(6), args->GetInt(7), args->GetString(8).ToWString().c_str(), args->GetInt(9), args->GetInt(10), DEF_TRANS_VALUE, args->GetInt(11));
			}
			
		}
	}

	void createModalWindow(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefRefPtr<CefListValue>& args)
	{
		auto item = EasyWebViewMgr::GetInstance().GetItemBrowserById(browser->GetIdentifier());
		if (!item)
			return;

		HWND hWnd = item->GetHWND();
		if (WebkitEcho::getUIFunMap() && WebkitEcho::getUIFunMap()->createModalWindow && IsWindow(hWnd))
		{
			if (args->GetSize() == 1)
			{
				JSCreateWindowParam parm;
				if (ParseCreateWindowParam(args->GetString(0), &parm))
				{
					WebkitEcho::getUIFunMap()->createModalWindow(hWnd, parm.x, parm.y, parm.width, parm.height, parm.min_cx, parm.min_cy, parm.max_cx, parm.max_cy, parm.skin.c_str(), parm.alpha, parm.ulStyle, parm.trans, parm.extra);
				}
			}
			else
			{
				//无法获取原始字符串？参数都解开了，那就这样吧
				WebkitEcho::getUIFunMap()->createModalWindow(hWnd, args->GetInt(0), args->GetInt(1), args->GetInt(2), args->GetInt(3), args->GetInt(4), args->GetInt(5), args->GetInt(6), args->GetInt(7), args->GetString(8).ToWString().c_str(), args->GetInt(9), args->GetInt(10), DEF_TRANS_VALUE, args->GetInt(11));
			}
		}
	}

	void createModalWindow2(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefRefPtr<CefListValue>& args)
	{
		auto item = EasyWebViewMgr::GetInstance().GetItemBrowserById(browser->GetIdentifier());
		if (!item)
			return;

		HWND hWnd = item->GetHWND();
		if (WebkitEcho::getUIFunMap() && WebkitEcho::getUIFunMap()->createModalWindow2 && IsWindow(hWnd))
		{
			if (args->GetSize() == 1)
			{
				JSCreateWindowParam parm;
				if (ParseCreateWindowParam(args->GetString(0), &parm))
				{
					WebkitEcho::getUIFunMap()->createModalWindow2(hWnd, parm.x, parm.y, parm.width, parm.height, parm.min_cx, parm.min_cy, parm.max_cx, parm.max_cy, parm.skin.c_str(), parm.alpha, parm.ulStyle, parm.trans, parm.extra, parm.parentSign);
				}
			}
			else
			{
				//无法获取原始字符串？参数都解开了，那就这样吧
				WebkitEcho::getUIFunMap()->createModalWindow2(hWnd, args->GetInt(0), args->GetInt(1), args->GetInt(2), args->GetInt(3), args->GetInt(4), args->GetInt(5), args->GetInt(6), args->GetInt(7), args->GetString(8).ToWString().c_str(), args->GetInt(9), args->GetInt(10), DEF_TRANS_VALUE, args->GetInt(11), args->GetInt(12));
			}
		}

	}

	void setAlpha(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefRefPtr<CefListValue>& args)
	{
		CefRefPtr<WebViewUIControl> item = dynamic_cast<WebViewUIControl*>(EasyWebViewMgr::GetInstance().GetItemBrowserById(browser->GetIdentifier()).get());

		if (!item)
			return;

		int alpha = args->GetInt(0);

		item->SetAlpha(alpha);
	}


	


	void closeWindow(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefRefPtr<CefListValue>& args)
	{
		auto item = EasyWebViewMgr::GetInstance().GetItemBrowserById(browser->GetIdentifier());
		if (!item)
			return;

		HWND hWnd = item->GetHWND();
		if (WebkitEcho::getUIFunMap() && WebkitEcho::getUIFunMap()->closeWindow && IsWindow(hWnd))
		{
			WebkitEcho::getUIFunMap()->closeWindow(hWnd);
		}
	}

	void asyncCrossInvokeWebMethod(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefRefPtr<CefListValue>& args)
	{
		auto item = EasyWebViewMgr::GetInstance().GetItemBrowserById(browser->GetIdentifier());
		if (!item)
			return;

		HWND hWnd = item->GetHWND();
		if (WebkitEcho::getUIFunMap() && WebkitEcho::getUIFunMap()->asyncCrossInvokeWebMethod && IsWindow(hWnd))
		{
			int sign = args->GetInt(0);
			std::wstring module = args->GetString(1).ToWString();
			std::wstring method = args->GetString(2).ToWString();
			std::wstring parm = args->GetString(3).ToWString();
			bool tojson = args->GetBool(4);

			WebkitEcho::getUIFunMap()->asyncCrossInvokeWebMethod(hWnd, sign, module.c_str(), method.c_str(), parm.c_str(), tojson);
		}
	}

	void asyncCrossInvokeWebMethod2(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefRefPtr<CefListValue>& args)
	{
		auto item = EasyWebViewMgr::GetInstance().GetItemBrowserById(browser->GetIdentifier());
		if (!item)
			return;

		HWND hWnd = item->GetHWND();
		if (WebkitEcho::getUIFunMap() && WebkitEcho::getUIFunMap()->asyncCrossInvokeWebMethod2 && IsWindow(hWnd))
		{
			int sign = args->GetInt(0);
			std::wstring frame = args->GetString(1).ToWString();
			std::wstring module = args->GetString(2).ToWString();
			std::wstring method = args->GetString(3).ToWString();
			std::wstring parm = args->GetString(4).ToWString();
			bool tojson = args->GetBool(5);

			WebkitEcho::getUIFunMap()->asyncCrossInvokeWebMethod2(hWnd, sign, frame.c_str(), module.c_str(), method.c_str(), parm.c_str(), tojson);
		}
	}

	void asyncCallMethod(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefRefPtr<CefListValue>& args)
	{
		auto item = EasyWebViewMgr::GetInstance().GetItemBrowserById(browser->GetIdentifier());
		if (!item)
			return;
		
		HWND hWnd = item->GetHWND();

		if (!WebkitEcho::getUIFunMap() || !IsWindow(hWnd))
			return;

		std::wstring module = args->GetString(0).ToWString();
		std::wstring method = args->GetString(1).ToWString();
		std::wstring parm = args->GetString(2).ToWString();
		int extra = args->GetInt(3);

		std::string cb_module;
		std::string cb_method;
		CefString cb_parm;

		if (args->GetSize() > 5)
		{
			cb_module = args->GetString(4).ToString();
			cb_method = args->GetString(5).ToString();
		}

		if (item->IsUIControl())
		{
			if (WebkitEcho::getUIFunMap()->invokeMethod)
			{
				cb_parm = WebkitEcho::getUIFunMap()->invokeMethod(hWnd, module.c_str(), method.c_str(), parm.c_str(), extra);
			}
		}
		else
		{
			if (WebkitEcho::getFunMap()->webkitAsyncCallMethod)
			{
				cb_parm = WebkitEcho::getFunMap()->webkitAsyncCallMethod(browser->GetIdentifier(), module.c_str(), method.c_str(), parm.c_str(), extra);
			}
		}

		if (!cb_module.empty() && !cb_method.empty())
		{
			auto strParam = ArrangeJsonString(cb_parm.ToString());
			auto fmt = std::format("window.invokeMethod('{}', '{}', {}, {})", cb_module, cb_method, strParam, "true");
			frame->ExecuteJavaScript(fmt, "", 0);
		}
	}

	void __V8AccessorSetAttribKeys__(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefRefPtr<CefListValue>& args)
	{
		EasyBrowserWorks::GetInstance().DoJSKeyAsyncWork(args->GetString(0).ToString(), browser, frame, args);
	}

	void __DOMContentLoaded__(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefRefPtr<CefListValue>& args)
	{
		//原先好像onDocLoaded把同步去掉了，那直接异步处理就好了

		auto item = EasyWebViewMgr::GetInstance().GetItemBrowserById(browser->GetIdentifier());
		if (!item)
			return;

		HWND hWnd = item->GetHWND();


		std::wstring FrameUrl = frame->GetURL().ToWString();
		std::wstring FrameName = frame->GetName().ToWString();

		auto MainFrame = browser->GetMainFrame();


		std::wstring MainUrl = MainFrame ? MainFrame->GetURL().ToWString() : L"";
		
		std::wstring strToInject;

		if (item->IsUIControl())
		{
			if (WebkitEcho::getUIFunMap() && WebkitEcho::getUIFunMap()->injectJS)
			{
				strToInject = WebkitEcho::getUIFunMap()->injectJS(hWnd, FrameUrl.c_str(), MainUrl.c_str(), FrameName.c_str());
			}
		}
		else
		{
			if (WebkitEcho::getFunMap())
			{
				if (WebkitEcho::getFunMap()->webkitDocLoaded)
				{
					WebkitEcho::getFunMap()->webkitDocLoaded(browser->GetIdentifier(), FrameUrl.c_str(), FrameName.c_str(), frame->IsMain());
				}

				if (WebkitEcho::getFunMap()->webkitInjectJS)
				{
					strToInject = WebkitEcho::getFunMap()->webkitInjectJS(browser->GetIdentifier(), FrameUrl.c_str(), MainUrl.c_str(), FrameName.c_str());
				}
			}
		
		}

		if (!strToInject.empty())
		{
			frame->ExecuteJavaScript(strToInject, "", 0);
		}


	}

	
}

namespace BrowserAsyncWorkJSKeys {

	void nc_setalledge(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefRefPtr<CefListValue>& args)
	{
		static std::unordered_map<std::string, EasyUIWindowBase::HT_INFO> mapInfo;
		if (mapInfo.empty())
		{
			mapInfo.insert(std::make_pair("top", EasyUIWindowBase::E_HTTOP));
			mapInfo.insert(std::make_pair("left", EasyUIWindowBase::E_HTLEFT));
			mapInfo.insert(std::make_pair("right", EasyUIWindowBase::E_HTRIGHT));
			mapInfo.insert(std::make_pair("bottom", EasyUIWindowBase::E_HTBOTTOM));
			mapInfo.insert(std::make_pair("topleft", EasyUIWindowBase::E_HTTOPLEFT));
			mapInfo.insert(std::make_pair("topright", EasyUIWindowBase::E_HTTOPRIGHT));
			mapInfo.insert(std::make_pair("bottomleft", EasyUIWindowBase::E_HTBOTTOMLEFT));
			mapInfo.insert(std::make_pair("bottomright", EasyUIWindowBase::E_HTBOTTOMRIGHT));

			mapInfo.insert(std::make_pair("maxbutton", EasyUIWindowBase::E_HTMAXBUTTON));
		}

		CefRefPtr<WebViewUIControl> item = dynamic_cast<WebViewUIControl*>(EasyWebViewMgr::GetInstance().GetItemBrowserById(browser->GetIdentifier()).get());
		if (!item || !item->IsUIControl())
			return;

		auto testVal = args->GetValue(1);
		if (testVal->GetType() == VTYPE_DICTIONARY)
		{
			auto dict = args->GetDictionary(1);
			if (!dict || dict->GetSize() == 0)
				return;

			CefDictionaryValue::KeyList keyList;
			dict->GetKeys(keyList);
			for (auto it : keyList)
			{
				auto value = dict->GetValue(it);

				auto type = mapInfo.find(it);

				if (value->GetType() == VTYPE_LIST)
				{
					auto list = dict->GetList(it);
					std::vector<RECT> vecRc;
					for (size_t i = 0; i < list->GetSize(); i++)
					{
						auto obj = list->GetDictionary(i);
						if (obj)
						{
							RECT rc{ obj->GetInt("left"),obj->GetInt("top") ,obj->GetInt("right") ,obj->GetInt("bottom") };
							if (rc.left || rc.top || rc.right || rc.bottom)
							{
								vecRc.push_back(rc);
							}
						}
					}

					item->SetEdgeNcAera(type->second, vecRc);
				}
			}
		}

		
	}
}



EasyBrowserWorks::EasyBrowserWorks()
{
#define REG_SYNCWORK_FUNCTION(fnName) \
	m_mapSyncFuncs.insert(std::make_pair(std::string(#fnName), BrowserSyncWorkFunctions::fnName))

#define REG_ASYNCWORK_FUNCTION(fnName) \
	m_mapAsyncFuncs.insert(std::make_pair(std::string(#fnName), BrowserAsyncWorkFunctions::fnName))

#define REG_SYNCWORK_JSKEY(fnName) \
	m_mapSyncJsKeyFuncs.insert(std::make_pair(std::string(#fnName), BrowserSyncWorkJSKeys::fnName))

#define REG_ASYNCWORK_JSKEY(fnName) \
	m_mapAsyncJsKeyFuncs.insert(std::make_pair(std::string(#fnName), BrowserAsyncWorkJSKeys::fnName))


	REG_SYNCWORK_FUNCTION(crossInvokeWebMethod);
	REG_SYNCWORK_FUNCTION(crossInvokeWebMethod2);
	REG_SYNCWORK_FUNCTION(invokeMethod);
	REG_SYNCWORK_FUNCTION(winProty);
	REG_SYNCWORK_FUNCTION(setProfile);
	REG_SYNCWORK_FUNCTION(getProfile);
	REG_SYNCWORK_FUNCTION(getSoftwareAttribute);

	//集中用于获取属性值，然后下面的才有效
	REG_SYNCWORK_FUNCTION(__V8AccessorAttribKeys__);

	REG_SYNCWORK_JSKEY(appDataPath);
	



	//异步
	REG_ASYNCWORK_FUNCTION(createWindow);
	REG_ASYNCWORK_FUNCTION(createModalWindow);
	REG_ASYNCWORK_FUNCTION(createModalWindow2);
	REG_ASYNCWORK_FUNCTION(setAlpha);
	REG_ASYNCWORK_FUNCTION(setWindowPos);

	REG_ASYNCWORK_FUNCTION(closeWindow);
	REG_ASYNCWORK_FUNCTION(asyncCrossInvokeWebMethod);
	REG_ASYNCWORK_FUNCTION(asyncCrossInvokeWebMethod2);
	REG_ASYNCWORK_FUNCTION(asyncCallMethod);

	REG_ASYNCWORK_FUNCTION(__DOMContentLoaded__);

	REG_ASYNCWORK_FUNCTION(__V8AccessorSetAttribKeys__);



	REG_ASYNCWORK_JSKEY(nc_setalledge);

	

}

EasyBrowserWorks& EasyBrowserWorks::GetInstance()
{
	static EasyBrowserWorks obj;
	return obj;
}

void EasyBrowserWorks::DoWork(std::shared_ptr<EasyIPCWorks::BRDataPack> pData)
{

	if (pData->DataInvalid)
		return;

	auto item = EasyWebViewMgr::GetInstance().GetItemBrowserById(pData->BrowserId);
	if (item)
	{
		CefString ReturnVal;
		if (DoSyncWork(pData->Name, item->GetBrowser(), item->GetBrowser()->GetFrame(pData->FrameId), pData->Args, ReturnVal))
		{
			pData->ReturnVal = ReturnVal.ToString();
		}
	}

	pData->Signal.set_value();
}

bool EasyBrowserWorks::DoJSKeySyncWork(const std::string name, CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefString& retval)
{
	auto it = m_mapSyncJsKeyFuncs.find(name);
	if (it == m_mapSyncJsKeyFuncs.end())
		return false;

	it->second(browser, frame, retval);
	
	return true;
}

void EasyBrowserWorks::DoJSKeyAsyncWork(const std::string name, CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefRefPtr<CefListValue>& args)
{
	auto it = m_mapAsyncJsKeyFuncs.find(name);
	if (it == m_mapAsyncJsKeyFuncs.end())
		return;

	it->second(browser, frame, args);

}
