#include "pch.h"
#include "EasyIPCWorks.h"

bool EasyIPCWorks::DoSyncWork(const std::string name, CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefRefPtr<CefListValue>& args, CefString& retval)
{
	auto it = m_mapSyncFuncs.find(name);
	if (it == m_mapSyncFuncs.end())
		return false;

	return it->second(browser, frame, args, retval);
}

bool EasyIPCWorks::DoAsyncWork(const std::string name, CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefRefPtr<CefListValue>& args)
{
	auto it = m_mapAsyncFuncs.find(name);
	if (it == m_mapAsyncFuncs.end())
		return false;

	it->second(browser, frame, args);

	return true;
}

void EasyIPCWorks::CommWork(const std::string& input, std::string& output)
{
	auto pData = std::make_shared< EasyIPCWorks::BRDataPack>();

	if (!QuickGetIpcParms(input, pData->BrowserId, pData->FrameId, pData->Name, pData->Args))
	{
		LOG(WARNING) << GetCurrentProcessId() << "] QuickGetIpcParms failed" << input;

		return;
	}

	//LOG(INFO) << GetCurrentProcessId() << "] EasyIPCWorks::UIWork name: " << pData->Name << " is browser:"<< IsBrowser();

	
	auto it = m_mapSyncFuncs.find(pData->Name);
	//可以考虑进一步过滤
	bool bSync = it != m_mapSyncFuncs.end();
	bool bNeedUI = false;
	if (bSync)
	{
		if (IsBrowser())
		{
			//只放几个必须得同步的。看了下mirage里面browser只有invokeMethod有必要
			//这个的主要原有是因为存量旧代码可能会在内部创建窗口之类的
			bNeedUI = pData->Name == "invokeMethod";
		}
		else
		{
			bNeedUI = true;
		}
		
	}

	UIWork(pData, bNeedUI);

	if (bNeedUI)
	{
		auto waitres = pData->Signal.get_future().wait_for(std::chrono::milliseconds(14000));
		if (std::future_status::ready == waitres)
		{
			output = std::move(pData->ReturnVal);
		}
		else if (std::future_status::timeout == waitres)
		{
			pData->DataInvalid = true;
			LOG(WARNING) << GetCurrentProcessId() << "] EasyIPCWorks::UIWork timeout " << pData->Name << " data:" << input;
		}
	}
	else
	{
		output = std::move(pData->ReturnVal);
	}


	//LOG(INFO) << GetCurrentProcessId() << "] SetWorkCall:f->t (" << pData << ") " << input << "out:" << output;

}
