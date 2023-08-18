#pragma once

#include "EasyIPCWorks.h"

class EasyBrowserWorks : public EasyIPCWorks
{
	MYDISALLOW_COPY_AND_ASSIGN(EasyBrowserWorks);
	EasyBrowserWorks();

	using SyncJSKeyMap = std::unordered_map<std::string, std::function<void(CefRefPtr<CefBrowser>, CefRefPtr<CefFrame>, CefString&)>>;

	SyncJSKeyMap m_mapSyncJsKeyFuncs;
	AsyncFunctionMap m_mapAsyncJsKeyFuncs;

	std::unordered_map<std::string, int> m_mapSyncUserFuncs, m_mapAsyncUserFuncs;

	void DoWork(std::shared_ptr<EasyIPCWorks::BRDataPack> pData) override;

public:
	static EasyBrowserWorks& GetInstance();

	bool IsBrowser() override { return true; }

	bool DoJSKeySyncWork(const std::string& name, CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefString& retval);
	void DoJSKeyAsyncWork(const std::string& name, CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefRefPtr<CefListValue>& args);

	typedef int(*jscall_UserFunction)(HWND hWnd, LPCSTR jsonParams, char** strRet, void* context);

	bool RegisterUserJSFunction(const std::string& name, jscall_UserFunction funUser, bool bSync, void* context, int RegType);
	CefRefPtr<CefDictionaryValue> GetUserJSFunction(bool bSync);

	bool CheckNeedUI(const std::string& name) override;
};

