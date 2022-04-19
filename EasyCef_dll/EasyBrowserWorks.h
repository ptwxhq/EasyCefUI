#pragma once

#include "EasyIPCWorks.h"

class EasyBrowserWorks : public EasyIPCWorks
{
	MYDISALLOW_COPY_AND_ASSIGN(EasyBrowserWorks);
	EasyBrowserWorks();

	typedef void(*HandleJSKeyCallback)(CefRefPtr<CefBrowser>, CefRefPtr<CefFrame>, CefString&);
	typedef std::unordered_map<std::string, HandleJSKeyCallback> SyncJSKeyMap;

	typedef void(*HandleJSKeySetCallback)(CefRefPtr<CefBrowser>, CefRefPtr<CefFrame>, const CefRefPtr<CefListValue>&);
	typedef std::unordered_map<std::string, HandleJSKeySetCallback> ASyncJSKeyMap;

	SyncJSKeyMap m_mapSyncJsKeyFuncs;
	ASyncJSKeyMap m_mapAsyncJsKeyFuncs;

	void DoWork(std::shared_ptr<EasyIPCWorks::BRDataPack> pData) override;

public:
	static EasyBrowserWorks& GetInstance();

	bool IsBrowser() override { return true; }

	bool DoJSKeySyncWork(const std::string name, CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefString& retval);
	void DoJSKeyAsyncWork(const std::string name, CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefRefPtr<CefListValue>& args);
};

