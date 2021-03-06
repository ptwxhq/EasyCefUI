#pragma once


#include <unordered_map>
#include <future>


class EasyIPCWorks : public CefRefCount
{
public:
	struct BRDataPack
	{
		bool DataInvalid = false;
		int BrowserId = -1;
		int64 FrameId = -1;
		std::string Name;
		std::string ReturnVal;
		CefRefPtr<CefListValue> Args;

		std::promise<void> Signal;
	};

	

	bool DoSyncWork(const std::string name, CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefRefPtr<CefListValue>& args, CefString& retval);
	bool DoAsyncWork(const std::string name, CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefRefPtr<CefListValue>& args);

	virtual void CommWork(const std::string& input, std::string& output);

	virtual bool IsBrowser() = 0;

protected:
	typedef bool(*HandleSyncCallback)(CefRefPtr<CefBrowser>, CefRefPtr<CefFrame>, const CefRefPtr<CefListValue>&, CefString&);
	typedef std::unordered_map<std::string, HandleSyncCallback> SyncFunctionMap;

	typedef void(*HandleAsyncCallback)(CefRefPtr<CefBrowser>, CefRefPtr<CefFrame>, const CefRefPtr<CefListValue>&);
	typedef std::unordered_map<std::string, HandleAsyncCallback> AsyncFunctionMap;

	void UIWork(std::shared_ptr<EasyIPCWorks::BRDataPack> pData, bool bNeedUIThread);
	virtual void DoWork(std::shared_ptr<EasyIPCWorks::BRDataPack> pData) = 0;

	SyncFunctionMap m_mapSyncFuncs;
	AsyncFunctionMap m_mapAsyncFuncs;
};

