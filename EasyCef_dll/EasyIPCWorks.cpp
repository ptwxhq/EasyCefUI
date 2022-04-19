#include "pch.h"
#include "EasyIPCWorks.h"
#include "EasyIPC.h"
#include "include/base/cef_callback.h"
#include "include/wrapper/cef_closure_task.h"

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

void EasyIPCWorks::UIWork(std::shared_ptr<EasyIPCWorks::BRDataPack> pData, bool bNeedUIThread)
{
	if (bNeedUIThread)
	{
		//比如在主线程调用invokedJSMethod，然后js再调用invokeMethod的话会导致直接发送失败，需要下面的特化处理
		auto pIPC = IsBrowser() ? static_cast<EasyIPCBase*>(&EasyIPCServer::GetInstance()) : static_cast<EasyIPCBase*>(&EasyIPCClient::GetInstance());

		//LOG(INFO) << GetCurrentProcessId() << "] UIWork IsMainThreadBlocking:(" << pIPC->IsMainThreadBlocking();
		if (pIPC->IsMainThreadBlocking())
		{
			auto pWait = std::make_shared<std::promise<void>>();

			pIPC->SetOnceMainThreadBlockingWorkCall([this, pData, pWait] {
				UIWork(pData, false);
				pWait->set_value();
				});

			if (pIPC->TriggerBlockingWorkEvent())
			{
				pWait->get_future().wait_for(std::chrono::milliseconds(15000));
			}

			//LOG(INFO) << GetCurrentProcessId() << "] EasyIPCwork specia end:(" << pData->ReturnVal;

			return;
		}
	}

	auto NeedTID = IsBrowser() ? TID_UI : TID_RENDERER;
	if (bNeedUIThread && !CefCurrentlyOn(NeedTID))
	{
		// Execute on the UI thread.
		bool bPostSucc = CefPostTask(NeedTID, CEF_FUNC_BIND(&EasyIPCWorks::DoWork, this, pData));

		if (!bPostSucc)
		{
			pData->Signal.set_value();
		}

		return;
	}

	DoWork(pData);
}
