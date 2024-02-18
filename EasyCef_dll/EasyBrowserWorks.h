#pragma once

#include "EasyIPCWorks.h"
#include "EasyIPC.h"
#include "EasyExport.h"

class EasyBrowserWorks : public EasyIPCWorks
{
public:

	struct BrowserFrameContext
	{
		bool IsMainFrame = false;
		int BrowserId = -1;
		int64_t FrameId = -1;
		int64_t MainFrameId = -1;
		std::wstring FrameName;
		std::wstring FrameUrl;
		std::wstring MainUrl;
	};
	static void InnerBrowserFrameContext2User(const BrowserFrameContext* context, EASYCEF::UserFuncContext* user);
	static void SetBrowserFrameContext(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, BrowserFrameContext* context);

	static EasyBrowserWorks& GetInstance();

	bool IsBrowser() override { return true; }

	bool DoJSKeySyncWork(const std::string& name, CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefString& retval);
	void DoJSKeyAsyncWork(const std::string& name, CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefRefPtr<CefListValue>& args);


	bool RegisterUserJSFunction(const std::string& name, EASYCEF::jscall_UserFunction funUser, bool bSync, void* context, int RegType);
	CefRefPtr<CefDictionaryValue> GetUserJSFunction(bool bSync);

	void SetLocalHost(const std::string& host, const std::string& ip);
	std::string GetLocalHosts(const std::string& host, const std::string& ip);

	bool GetBrowserFrameContext(EASYCEF::UserFuncContext* data);

	bool CheckNeedUI(const std::string& name) override;

	void DisconnectNetworkUtility();

private:
	MYDISALLOW_COPY_AND_ASSIGN(EasyBrowserWorks);
	EasyBrowserWorks();

	using SyncJSKeyMap = std::unordered_map<std::string, std::function<void(CefRefPtr<CefBrowser>, CefRefPtr<CefFrame>, CefString&)>>;

	SyncJSKeyMap m_mapSyncJsKeyFuncs;
	AsyncFunctionMap m_mapAsyncJsKeyFuncs;

	std::unordered_map<std::string, int> m_mapSyncUserFuncs, m_mapAsyncUserFuncs;
	std::unordered_map<std::string, std::string> m_mapLocalHost;



	std::unordered_map<size_t, BrowserFrameContext> m_mapBrowserFrameContext;

	EasyIPCBase::IPCHandle m_hNetworkUtility = nullptr;

	void DoWork(std::shared_ptr<EasyIPCWorks::BRDataPack> pData) override;

	size_t SetBrowserFrameContext(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame);
	void RemoveBrowserFrameContext(size_t id);

	void SetNetworkUtilityHandle(EasyIPCBase::IPCHandle h) {
		m_hNetworkUtility = h;
	}
};

