#include "pch.h"
#include "EasyCefAppRender.h"
#include "EasyIPC.h"
#include "EasySchemes.h"
#include "EasyRenderBrowserInfo.h"

#include "NativeV8Handler.h"
#include "NativeV8Accessor.h"

#include "EasyRenderWorks.h"
#include "EasyRenderBrowserInfo.h"

#include "LegacyImplement.h"



void EasyCefAppRender::OnWebKitInitialized()
{
	CEF_REQUIRE_RENDERER_THREAD();

	EasyIPCClient::GetInstance().SetMainThread(GetCurrentThreadId());
	EasyIPCClient::GetInstance().ThdRun();


	//LOG(INFO) << GetCurrentProcessId() << "]ClientAppRenderer::OnWebKitInitialized";


	std::string app_code = R"(
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

	bool bCheck = extra_info->GetBool(ExtraKeyNameIsManagedPopup);

	//LOG(INFO) << GetCurrentProcessId() << "] EasyCefAppRender::OnBrowserCreated:" << browser << " is pop " << bIsPopup << " pass:" << bCheck;

	if (bIsPopup && !bCheck)
	{
		//可能是控制台，不处理
		return;
	}

	if (!EasyIPCClient::GetInstance().IsServerSet())
	{
		EasyIPCBase::IPCHandle ServerHandle = nullptr;
		auto valKeyName = extra_info->GetBinary(IpcBrowserServerKeyName);
		valKeyName->GetData(&ServerHandle, valKeyName->GetSize(), 0);

		EasyIPCClient::GetInstance().SetServer(ServerHandle);

		EasyIPCClient::GetInstance().SetWorkCall(std::bind(&EasyRenderWorks::CommWork, &EasyRenderWorks::GetInstance(), std::placeholders::_1, std::placeholders::_2));

		//LOG(INFO) << GetCurrentProcessId() << "] OnBrowserCreated::SetServer:" << ServerHandle;
	}

	if (extra_info->HasKey(ExtraKeyNameIsUIBrowser))
	{
		HWND hUIWnd = nullptr;

		auto IsUIBrowser = extra_info->GetBool(ExtraKeyNameIsUIBrowser);

		if (IsUIBrowser)
		{
			auto binHwnd = extra_info->GetBinary(ExtraKeyNameUIWndHwnd);
			binHwnd->GetData(&hUIWnd, binHwnd->GetSize(), 0);
		}

		EasyRenderBrowserInfo::GetInstance().AddBrowser(browser->GetIdentifier(), browser,
			IsUIBrowser ? EasyRenderBrowserInfo::BrsData::BROWSER_UI : EasyRenderBrowserInfo::BrsData::BROWSER_WEB, hUIWnd);

		//通知设置
		EasyIPCClient::GetInstance().NotifyConnect(browser->GetIdentifier());
	}

}

void EasyCefAppRender::OnBrowserDestroyed(CefRefPtr<CefBrowser> browser)
{
	//OnBrowserDestroyed好像只有自己创建的那个窗口被关闭时才有调用到，页面内弹出的不会调用到，是因为关联的吗
	//LOG(INFO) << GetCurrentProcessId() << "] EasyCefAppRender::OnBrowserDestroyed:" << browser ;

	EasyRenderBrowserInfo::GetInstance().RemoveBrowser(browser->GetIdentifier());

}


void call_FrameStateChanged(CefRefPtr<CefFrame>& frame, const char* frameName, const char* url, const int& code, bool didComit)
{
	CefRefPtr<CefValue> json = CefValue::Create();
	
	auto jDict = CefDictionaryValue::Create();
	jDict->SetString("frameid", frameName);

	jDict->SetString("src",  url);
	jDict->SetInt("state", code);
	jDict->SetBool("resloaded", didComit);

	json->SetDictionary(jDict);

	auto strJson = CefWriteJSON(json, JSON_WRITER_DEFAULT);

	std::string strJs = "_onFrameStateChanged(" + strJson.ToString() + ')';

	frame->ExecuteJavaScript(strJs, "", 0);

	//LOG(INFO) << GetCurrentProcessId() << "]call_FrameStateChanged(" << strJs << ") ";

}

void EasyCefAppRender::OnLoadingStateChange(CefRefPtr<CefBrowser> browser, bool isLoading, bool canGoBack, bool canGoForward)
{
	//这边只处理UI相关
	if (EasyRenderBrowserInfo::BrsData::BROWSER_UI != EasyRenderBrowserInfo::GetInstance().GetType(browser->GetIdentifier()))
	{
		return;
	}

	//LOG(INFO) << GetCurrentProcessId() << "]ClientAppRenderer::OnLoadingStateChange(" << browser->GetIdentifier() << ") isLoading" << isLoading << ")" << canGoBack << canGoForward;


	if (!isLoading) {
		//避免描点跳转无endload导致异常
		DocComplate::getInst().setBrowsr(browser->GetIdentifier(), true);
	}
}

void EasyCefAppRender::OnLoadStart(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, TransitionType transition_type)
{
	//这边只处理UI相关
	if (EasyRenderBrowserInfo::BrsData::BROWSER_UI != EasyRenderBrowserInfo::GetInstance().GetType(browser->GetIdentifier()))
	{
		return;
	}


	//LOG(INFO) << GetCurrentProcessId() << "]ClientAppRenderer::OnLoadStart(" << browser->GetIdentifier() << ")frame(" << frame->GetName().ToString() << ")" << transition_type;


	if (frame->IsMain())
		DocComplate::getInst().setBrowsr(browser->GetIdentifier(), false);

	CefRefPtr<CefFrame> parent = frame->GetParent();
	if (parent)
	{
		std::string frameNam = frame->GetName().ToString();
		if (!RecordFrameName::getInst().SaveRecord(browser->GetIdentifier(), frame->GetIdentifier(), frameNam.c_str())) {
			frameNam = RecordFrameName::getInst().GetRecord(browser->GetIdentifier(), frame->GetIdentifier());
		}
		std::hash<std::string> string_hash;
		auto id = string_hash(frameNam);
		if (DectetFrameLoad::getInst().hit(browser->GetIdentifier(), getFramePath(parent), id, -10086)) {
			std::string url = frame->GetURL().ToString();
			call_FrameStateChanged(parent, frameNam.c_str(), url.c_str(), -10086, false);
		}
	}
}

void EasyCefAppRender::OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode)
{
	//这边只处理UI相关
	if (EasyRenderBrowserInfo::BrsData::BROWSER_UI != EasyRenderBrowserInfo::GetInstance().GetType(browser->GetIdentifier()))
	{
		return;
	}

	//LOG(INFO) << GetCurrentProcessId() << "]ClientAppRenderer::OnLoadEnd(" << browser->GetIdentifier() << ")frame(" << frame->GetName().ToString() << ")" << httpStatusCode;



	CefRefPtr<CefFrame> parent = frame->GetParent();
	if (parent)
	{
		std::hash<std::string> string_hash;
		std::string frameNam = RecordFrameName::getInst().GetRecord(browser->GetIdentifier(), frame->GetIdentifier());
		auto id = string_hash(frameNam);
		if (DectetFrameLoad::getInst().hit(browser->GetIdentifier(), getFramePath(parent), id, httpStatusCode)) {
			std::string url = frame->GetURL().ToString();
			call_FrameStateChanged(parent, frameNam.c_str(), url.c_str(), httpStatusCode, true);
		}
	}
}

void EasyCefAppRender::OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, ErrorCode errorCode, const CefString& errorText, const CefString& failedUrl)
{
	//这边只处理UI相关
	if (EasyRenderBrowserInfo::BrsData::BROWSER_UI != EasyRenderBrowserInfo::GetInstance().GetType(browser->GetIdentifier()))
	{
		return;
	}

	CefRefPtr<CefFrame> parent = frame->GetParent();
	if (parent)
	{
		std::hash<std::string> string_hash;
		std::string frameNam = RecordFrameName::getInst().GetRecord(browser->GetIdentifier(), frame->GetIdentifier());
		size_t id = string_hash(frameNam);
		if (DectetFrameLoad::getInst().hit(browser->GetIdentifier(), getFramePath(parent), id, errorCode)) {
			std::string url = frame->GetURL().ToString();
			call_FrameStateChanged(parent, frameNam.c_str(), url.c_str(), errorCode, false);
			call_FrameStateChanged(parent, frameNam.c_str(), url.c_str(), errorCode, true);
		}
	}
}

void EasyCefAppRender::OnContextCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context)
{

	//LOG(INFO)  << GetCurrentProcessId() << "]ClientAppRenderer::OnContextCreated(" << browser->GetIdentifier() << ")frame(" << frame->GetName().ToString() <<")";

	// Retrieve the context's window object.
	CefRefPtr<CefV8Value> object = context->GetGlobal();


	CefRefPtr<NativeV8Handler> v8Handler = new NativeV8Handler;

	CefRefPtr<NativeV8Accessor> v8Accessor = new NativeV8Accessor;

	auto objapp = CefV8Value::CreateObject(v8Accessor, nullptr);

	object->SetValue("nativeapp", objapp, V8_PROPERTY_ATTRIBUTE_NONE);

	auto BrowserType = EasyRenderBrowserInfo::GetInstance().GetType(browser->GetIdentifier());

	v8Handler->RegisterFunctions(objapp, BrowserType);

	if (BrowserType == EasyRenderBrowserInfo::BrsData::BROWSER_UI)
	{
		v8Accessor->RegisterKeys(objapp);
	}

	auto testJS = R"(document.addEventListener('DOMContentLoaded', (event) => {
	__easycef_app.__NotifyDomLoaded();
});)";

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

void EasyCefAppRender::OnRegisterCustomSchemes(CefRawPtr<CefSchemeRegistrar> registrar)
{
	EasyRegisterCustomSchemes(registrar);
}

