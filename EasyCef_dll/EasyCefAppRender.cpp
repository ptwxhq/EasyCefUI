#include "pch.h"
#include "EasyCefAppRender.h"
#include "EasyIPC.h"
#include "EasySchemes.h"
#include "EasyRenderBrowserInfo.h"

#include "NativeV8Handler.h"
#include "NativeV8Accessor.h"

#include "EasyRenderWorks.h"
#include "EasyRenderBrowserInfo.h"


void EasyCefAppRenderBase::OnRegisterCustomSchemes(CefRawPtr<CefSchemeRegistrar> registrar)
{
	EasyRegisterCustomSchemes(registrar);
}

void EasyCefAppRender::OnWebKitInitialized()
{
	CEF_REQUIRE_RENDERER_THREAD();

	EasyIPCClient::GetInstance().SetMainThread(GetCurrentThreadId());
	EasyIPCClient::GetInstance().ThdRun();


	//LOG(INFO) << GetCurrentProcessId() << "]ClientAppRenderer::OnWebKitInitialized";


	const std::string app_code = R"(
var __easycef_app;
if (!__easycef_app)
__easycef_app = {};
(function() {
 __easycef_app.__NotifyDomLoaded = function() {
  native function __DOMContentLoaded__();
 return __DOMContentLoaded__();
};
})();
)";

	CefRegisterExtension("v8/easycef", app_code, new GobalNativeV8Handler);
}

void EasyCefAppRender::OnBrowserCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDictionaryValue> extra_info)
{

	bool bIsPopup = browser->IsPopup();

	bool bCheck = extra_info->GetBool(ExtraKeyNames[IsManagedPop]);

	//LOG(INFO) << GetCurrentProcessId() << "] EasyCefAppRender::OnBrowserCreated:" << browser << " is pop " << bIsPopup << " pass:" << bCheck;

	if (bIsPopup && !bCheck)
	{
		//可能是控制台，不处理
		return;
	}

	if (extra_info->HasKey(ExtraKeyNames[EnableHighDpi]))
	{
		g_BrowserGlobalVar.FunctionFlag.bEnableHignDpi = extra_info->GetBool(ExtraKeyNames[EnableHighDpi]);
	}

	if (!EasyIPCClient::GetInstance().IsServerSet())
	{
		EasyIPCBase::IPCHandle ServerHandle = nullptr;
		auto valKeyName = extra_info->GetBinary(ExtraKeyNames[IpcBrowserServer]);
		valKeyName->GetData(&ServerHandle, valKeyName->GetSize(), 0);

		EasyIPCClient::GetInstance().SetServer(ServerHandle);

		EasyIPCClient::GetInstance().SetWorkCall(std::bind(&EasyRenderWorks::CommWork, &EasyRenderWorks::GetInstance(), std::placeholders::_1, std::placeholders::_2));

		//LOG(INFO) << GetCurrentProcessId() << "] OnBrowserCreated::SetServer:" << ServerHandle;
	}

	if (extra_info->HasKey(ExtraKeyNames[UIWndHwnd]))
	{
		HWND hUIWnd = nullptr;

		auto binHwnd = extra_info->GetBinary(ExtraKeyNames[UIWndHwnd]);
		binHwnd->GetData(&hUIWnd, std::min(binHwnd->GetSize(), sizeof(binHwnd)), 0);

		EasyRenderBrowserInfo::GetInstance().AddBrowser(browser->GetIdentifier(), browser,
			hUIWnd ? EasyRenderBrowserInfo::BrsData::BROWSER_UI : EasyRenderBrowserInfo::BrsData::BROWSER_WEB, hUIWnd);

		//通知设置
		EasyIPCClient::GetInstance().NotifyConnect(browser->GetIdentifier());
	}

	if (extra_info->HasKey(ExtraKeyNames[RegSyncJSFunctions]))
	{
		m_dictUserSyncFunc = extra_info->GetDictionary(ExtraKeyNames[RegSyncJSFunctions])->Copy(false);
	}

	if (extra_info->HasKey(ExtraKeyNames[RegAsyncJSFunctions]))
	{
		m_dictUserAsyncFunc = extra_info->GetDictionary(ExtraKeyNames[RegAsyncJSFunctions])->Copy(false);
	}
}

void EasyCefAppRender::OnBrowserDestroyed(CefRefPtr<CefBrowser> browser)
{
	//OnBrowserDestroyed好像只有自己创建的那个窗口被关闭时才有调用到，页面内弹出的不会调用到，是因为关联的吗
	//LOG(INFO) << GetCurrentProcessId() << "] EasyCefAppRender::OnBrowserDestroyed:" << browser ;

	EasyRenderBrowserInfo::GetInstance().RemoveBrowser(browser->GetIdentifier());

}


void EasyCefAppRender::OnContextCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context)
{

	//LOG(INFO)  << GetCurrentProcessId() << "]ClientAppRenderer::OnContextCreated(" << browser->GetIdentifier() << ")frame(" << frame->GetName().ToString() <<")";

	// Retrieve the context's window object.
	CefRefPtr<CefV8Value> object = context->GetGlobal();


	CefRefPtr<NativeV8Handler> v8Handler = new NativeV8Handler;

	CefRefPtr<NativeV8Accessor> v8Accessor = new NativeV8Accessor;

	auto objapp = CefV8Value::CreateObject(v8Accessor, nullptr);

	object->SetValue("nativeapp", objapp, static_cast<CefV8Value::PropertyAttribute>(V8_PROPERTY_ATTRIBUTE_READONLY | V8_PROPERTY_ATTRIBUTE_DONTENUM | V8_PROPERTY_ATTRIBUTE_DONTDELETE));

	auto BrowserType = EasyRenderBrowserInfo::GetInstance().GetType(browser->GetIdentifier());

	v8Handler->RegisterFunctions(objapp, BrowserType);

	v8Handler->RegisterUserFunctions(objapp, m_dictUserSyncFunc, true, BrowserType);
	v8Handler->RegisterUserFunctions(objapp, m_dictUserAsyncFunc, false, BrowserType);

	if (BrowserType == EasyRenderBrowserInfo::BrsData::BROWSER_UI)
	{
		v8Accessor->RegisterKeys(objapp);
	}

	const auto testJS = R"(document.addEventListener('DOMContentLoaded', (event) => { __easycef_app.__NotifyDomLoaded();});)";

	frame->ExecuteJavaScript(testJS, "", 0);

}

void EasyCefAppRender::OnContextReleased(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context)
{
	//LOG(INFO) << GetCurrentProcessId() << "]ClientAppRenderer::OnContextReleased(" << browser->GetIdentifier() << ")frame(" << frame->GetName().ToString() << ")";

}

void EasyCefAppRender::OnFocusedNodeChanged(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefDOMNode> node)
{
}

bool EasyCefAppRender::OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefProcessId source_process, CefRefPtr<CefProcessMessage> message)
{

	const std::string& message_name = message->GetName();

	bool bWorked = EasyRenderWorks::GetInstance().DoAsyncWork(message_name, browser, frame, message->GetArgumentList());

	if (bWorked)
	{
		return true;
	}


	return false;
}

void EasyCefAppOther::OnBeforeCommandLineProcessing(const CefString& process_type, CefRefPtr<CefCommandLine> command_line)
{
	int InnerGetProcessType();
	if (InnerGetProcessType() == 12)
	{
		if (command_line->HasSwitch("brs-svr"))
		{
			if (!g_BrowserGlobalVar.funcSetHostResolverWork)
				return;

			auto value = strtoul(command_line->GetSwitchValue("brs-svr").ToString().c_str(), nullptr, 16);

			if (value != 0)
			{
				EasyIPCClient::GetInstance().SetServer((EasyIPCClient::IPCHandle)value);
				EasyIPCClient::GetInstance().SetWorkCall([](const std::string& input, std::string& output)
					{
						auto recVal = CefParseJSON(input, JSON_PARSER_RFC);
						if (!recVal)
							return;

						auto list = recVal->GetList();

						if (!list)
							return;

						for (size_t i = 0; i < list->GetSize(); i++)
						{
							auto dict = list->GetDictionary(i);
							if (!dict->HasKey("host"))
								continue;

							auto szHost = dict->GetString("host");

							const char* pIp = nullptr;
							std::string szIp;
							if (dict->HasKey("ip"))
							{
								szIp = dict->GetString("ip").ToString();
								pIp = szIp.c_str();
							}

							g_BrowserGlobalVar.funcSetHostResolverWork(szHost.ToString().c_str(), pIp);
						}
					});

				EasyIPCClient::GetInstance().ThdRun();

				CefRefPtr<CefListValue> valueList = CefListValue::Create();
				valueList->SetSize(2);

				valueList->SetString(0, "init");

				auto handle = EasyIPCClient::GetInstance().GetHandle();
				auto valKey = CefBinaryValue::Create(&handle, sizeof(handle));
				valueList->SetInt(1, (int)handle);

				auto strData = QuickMakeIpcParms(-1, 0, 0, ExtraKeyNames[IPC_RenderUtilityNetwork], valueList);

				std::string ret;
				EasyIPCClient::GetInstance().SendDataToServer(strData, ret, 0);
			}
		}



	}
}
