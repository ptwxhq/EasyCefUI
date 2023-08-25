#include "pch.h"
#include "EasyBrowserWorks.h"
#include "EasyWebViewMgr.h"
#include "WebViewControl.h"
#include "LegacyImplement.h"
#include "EasySchemes.h"


namespace BrowserSyncWorkFunctions {

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

	void dbgmode(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefRefPtr<CefListValue>& args)
	{
		if (g_BrowserGlobalVar.Debug)
		{
#if 1
			
#endif
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
		
		if (g_BrowserGlobalVar.funcCallDOMCompleteStatus)
		{
			g_BrowserGlobalVar.funcCallDOMCompleteStatus(hWnd, FrameName.c_str(), FrameUrl.c_str(), MainUrl.c_str());
		}
	}

	void __ContinueUnsecure__(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefRefPtr<CefListValue>& args)
	{
		auto url = args->GetString(0).ToWString();
		if (!url.empty())
		{
			auto domain = DomainPackInfo::GetFormatedDomain(url.c_str());
			g_BrowserGlobalVar.listAllowUnsecureDomains.insert(domain);
			frame->LoadURL(url);
		}
	}
	
	
}

namespace BrowserAsyncWorkJSKeys {

	void __nc_setalledge__(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefRefPtr<CefListValue>& args)
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
			for (const auto& it : keyList)
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


	//集中用于获取属性值，然后下面的才有效
	REG_SYNCWORK_FUNCTION(__V8AccessorAttribKeys__);
	REG_SYNCWORK_JSKEY(appDataPath);
	



	//异步
	REG_ASYNCWORK_FUNCTION(__DOMContentLoaded__);
	REG_ASYNCWORK_FUNCTION(__V8AccessorSetAttribKeys__);
	REG_ASYNCWORK_FUNCTION(dbgmode);
	REG_ASYNCWORK_JSKEY(__nc_setalledge__);

	

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

bool EasyBrowserWorks::DoJSKeySyncWork(const std::string& name, CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefString& retval)
{
	auto it = m_mapSyncJsKeyFuncs.find(name);
	if (it == m_mapSyncJsKeyFuncs.end())
		return false;

	it->second(browser, frame, retval);
	
	return true;
}

void EasyBrowserWorks::DoJSKeyAsyncWork(const std::string& name, CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefRefPtr<CefListValue>& args)
{
	auto it = m_mapAsyncJsKeyFuncs.find(name);
	if (it == m_mapAsyncJsKeyFuncs.end())
		return;

	it->second(browser, frame, args);

}

bool EasyBrowserWorks::RegisterUserJSFunction(const std::string& name, jscall_UserFunction funUser, bool bSync, void* context, int RegType)
{
	if (!(RegType & 3))
	{
		assert("RegType value invaild");
		return false;
	}

	RegType &= 0b0111;


	//已注册相同类型才可覆盖
	const bool bExistUserSync = m_mapSyncUserFuncs.find(name) != m_mapSyncUserFuncs.end();
	const bool bExistUserAsync = m_mapAsyncUserFuncs.find(name) != m_mapAsyncUserFuncs.end();

	if (bExistUserSync)
	{
		if (!bSync)
		{
			return false;
		}
	}
	else if (bExistUserAsync)
	{
		if (bSync)
		{
			return false;
		}
	}
	else
	{
		//如果没有，那么map里找到的时候不可注册，否则可以，防止内置的被覆盖
		if (m_mapAsyncFuncs.find(name) != m_mapAsyncFuncs.end() || m_mapSyncFuncs.find(name) != m_mapSyncFuncs.end())
		{
			return false;
		}
	}


	if (bSync)
	{
		if (funUser)
		{
			auto fun = [funUser, context](CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefRefPtr<CefListValue>& args, CefString& retval) -> bool {

				auto item = EasyWebViewMgr::GetInstance().GetItemBrowserById(browser->GetIdentifier());
				if (!item)
					return false;

				HWND hWnd = item->GetHWND();

				CefRefPtr<CefValue> json = CefValue::Create();
				json->SetList(args);
				auto strArgs = CefWriteJSON(json, JSON_WRITER_DEFAULT).ToString();

				char* pRet = nullptr;
				int ret = funUser(hWnd, strArgs.c_str(), &pRet, context);
				if (pRet)
				{
					retval = pRet;
					delete[] pRet;
				}

				return ret == 0;

			};

			m_mapSyncUserFuncs.insert(std::make_pair(name, RegType));
			m_mapSyncFuncs.insert(std::make_pair(name, fun));
		}
		else
		{
			m_mapSyncUserFuncs.erase(name);
			m_mapSyncFuncs.erase(name);
		}

	}
	else
	{
		if (funUser)
		{
			auto fun = [funUser, context](CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefRefPtr<CefListValue>& args) {

				auto item = EasyWebViewMgr::GetInstance().GetItemBrowserById(browser->GetIdentifier());
				if (!item)
					return;

				HWND hWnd = item->GetHWND();

				CefRefPtr<CefValue> json = CefValue::Create();
				json->SetList(args);
				auto strArgs = CefWriteJSON(json, JSON_WRITER_DEFAULT).ToString();
				funUser(hWnd, strArgs.c_str(), nullptr, context);

			};

			m_mapAsyncUserFuncs.insert(std::make_pair(name, RegType));
			m_mapAsyncFuncs.insert(std::make_pair(name, fun));
		}
		else
		{
			m_mapAsyncUserFuncs.erase(name);
			m_mapAsyncFuncs.erase(name);
		}
	
	}

	
	return true;
}

CefRefPtr<CefDictionaryValue> EasyBrowserWorks::GetUserJSFunction(bool bSync)
{
	auto listfunc = CefDictionaryValue::Create();
	if (bSync)
	{
		for (const auto& it : m_mapSyncUserFuncs)
		{
			listfunc->SetInt(it.first, it.second & ~4);
		}
	}
	else
	{
		for (const auto& it : m_mapAsyncUserFuncs)
		{
			listfunc->SetInt(it.first, it.second & ~4);
		}
	}

	return listfunc;
}

bool EasyBrowserWorks::CheckNeedUI(const std::string& name)
{
	auto it = m_mapSyncUserFuncs.find(name);
	if (it != m_mapSyncUserFuncs.end()) {
		return !!(it->second & 4);
	}

	return false;
}
