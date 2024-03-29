﻿#include "pch.h"
#include "NativeV8Handler.h"
#include <ShlObj_core.h>
#include "EasyIPC.h"
#include "EasyRenderBrowserInfo.h"
#include "EasyIPCWorks.h"
#include "EasySchemes.h"

HWND JsGetWindowHwnd()
{
	auto context = CefV8Context::GetCurrentContext();
	auto frame = context->GetFrame();
	auto bid = frame->GetBrowser()->GetIdentifier();
	auto hWnd = EasyRenderBrowserInfo::GetInstance().GetHwnd(bid);
	return hWnd;
}


static void FowardRender2Browser(bool bSync, const CefString& name, const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval, CefString& exception)
{
	CefRefPtr<CefV8Context> context = CefV8Context::GetCurrentContext();
	CefRefPtr<CefFrame> frame = context->GetFrame();

	CefRefPtr<CefListValue> valueList;
	CefRefPtr<CefProcessMessage> msg;

	if (bSync)
	{
		valueList = CefListValue::Create();
	}
	else
	{
		msg = CefProcessMessage::Create(name);

		valueList = msg->GetArgumentList();
	}

	valueList->SetSize(arguments.size());

	const auto funcSetTypeValue = [](size_t index, const CefRefPtr<CefV8Value>& item, CefRefPtr<CefListValue>& toargs) {
		if (item->IsBool())
		{
			toargs->SetBool(index, item->GetBoolValue());
		}
		else if (item->IsInt() || item->IsUInt())
		{
			toargs->SetInt(index, item->GetIntValue());
		}
		else if (item->IsDouble())
		{
			toargs->SetDouble(index, item->GetDoubleValue());
		}
		else if (item->IsString())
		{
			toargs->SetString(index, item->GetStringValue());
		}
		else if (item->IsDate())
		{
#if CEF_VERSION_MAJOR > 108
			CefTime date;
			cef_time_from_basetime(item->GetDateValue(), &date);
#else
			auto date = item->GetDateValue();
#endif

			toargs->SetString(index, std::format("{}-{:02}-{:02} {:02}:{:02}:{:02}", date.year, date.month, date.day_of_month, date.hour, date.minute, date.second));
		}
		else if (item->IsObject() || item->IsArray())
		{
			toargs->SetString(index, CefV8ValueToString(item));
		}
	};


	int i = 0;
	for (auto& it : arguments)
	{
		funcSetTypeValue(i++, it, valueList);

		//不处理其他类型
	}

	if (bSync)
	{
		constexpr int dwTimeOut = 0;
		
		auto strData = QuickMakeIpcParms(frame->GetBrowser()->GetIdentifier(), frame->GetIdentifier(), dwTimeOut ? GetTimeNowMS(dwTimeOut) : 0, name, valueList);

		//LOG(INFO) << GetCurrentProcessId() << "] FowardRender2Browser tosend:" << strData;

		std::string strOut;
		if (EasyIPCClient::GetInstance().SendDataToServer(strData, strOut, dwTimeOut))
		{
			if (!strOut.empty())
			{
				auto recVal = CefParseJSON(strOut, JSON_PARSER_RFC);
				if (recVal)
				{
					retval = CefValueToCefV8Value(recVal);
				}
				else
				{
					retval = CefV8Value::CreateString(strOut);
				}

			}

			//空值不能抛异常
			//	exception = "return data is empty";

		}
		else
		{
			exception = "invoke operation failed";
		}
	}
	else
	{
		frame->SendProcessMessage(PID_BROWSER, msg);
		retval = CefV8Value::CreateInt(1);
	}
}

static void ParseDOMGetAttr(CefRefPtr<CefFrame> frame)
{

	static std::string strParseDomJS;
	if (strParseDomJS.empty())
	{
		strParseDomJS = R"((function(){const edges = [)";

		if (IsSystemWindows11OrGreater())
		{
			strParseDomJS += R"({ cls: "captionMaxButton", nc: "maxbutton" },)";
		}

		strParseDomJS += R"(
{ cls: "borderTop", nc: "top" },
{ cls: "borderTopLeft", nc: "topleft" },
{ cls: "borderTopRight", nc: "topright" },
{ cls: "borderLeft", nc: "left" },
{ cls: "borderRight", nc: "right" },
{ cls: "borderBottom", nc: "bottom" },
{ cls: "borderBottomLeft", nc: "bottomleft" },
{ cls: "borderBottomRight", nc: "bottomright" }
];

function isArray(myArray) {
return myArray.constructor.toString().indexOf("Array") > -1;
}

var eleList = new Array();

function getNeedEleInfo() {
eleList.splice(0, eleList.length);
for (let index = 0; index < edges.length; ++index) {
let eles = document.body.querySelectorAll("div." + edges[index].cls);
for (let i = 0; i < eles.length; ++i) {
let ele = eles[i];
let attr = ele.getAttribute("data-nc");
if (attr == edges[index].nc) {
eleList.push(ele);
resizeObserver.observe(ele);
}
}
}
}

function updateNcEleRects(myArray) {
getNeedEleInfo();
var notify_nc_alledge = new Object();
myArray.forEach(ele => {
let attr = ele.getAttribute("data-nc");
let drect = ele.getBoundingClientRect();

if (!isArray(!notify_nc_alledge[attr]))
notify_nc_alledge[attr] = new Array();

notify_nc_alledge[attr].push({ left: Math.floor(drect.left), top: Math.floor(drect.top), right: Math.floor(drect.right), bottom: Math.floor(drect.bottom) });
});

nativeapp.__nc_setalledge__ = notify_nc_alledge;
return notify_nc_alledge;
}

var timeoutID;

const resizeObserver = new ResizeObserver(entries => {
window.clearTimeout(timeoutID);
timeoutID = window.setTimeout(updateNcEleRects, 200, eleList);
});

getNeedEleInfo();
}()
)
)";

	}

   frame->ExecuteJavaScript(strParseDomJS, "", 0);
}





namespace JSCallFunctions
{
	void getPrivateProfileString(const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval, CefString& exception)
	{
		assert(arguments.size() == 4);
		if (arguments.size() != 4)
		{
			exception = "invalid param";
			return;
		}

		WCHAR strTemp[MAX_PATH];

		auto dwRes = GetPrivateProfileStringW(arguments[0]->GetStringValue().ToWString().c_str(),
			arguments[1]->GetStringValue().ToWString().c_str(),
			arguments[2]->GetStringValue().ToWString().c_str(),
			strTemp, MAX_PATH, arguments[3]->GetStringValue().ToWString().c_str());
		if (dwRes > 0)
		{
			bool bIsBufferOK = true;
			if (dwRes == MAX_PATH - 1)
			{
				bIsBufferOK = false;
				auto pChar = new WCHAR[MAXWORD];
				if (pChar) {
					GetPrivateProfileStringW(arguments[0]->GetStringValue().ToWString().c_str(),
						arguments[1]->GetStringValue().ToWString().c_str(),
						arguments[2]->GetStringValue().ToWString().c_str(),
						pChar, MAXWORD, arguments[3]->GetStringValue().ToWString().c_str());

					retval = CefV8Value::CreateString(pChar);

					delete[]pChar;
				}
				else
				{
					bIsBufferOK = true;
				}

			}

			if (bIsBufferOK)
			{
				retval = CefV8Value::CreateString(strTemp);
			}
		}
		else
		{
			retval = CefV8Value::CreateString(arguments[2]->GetStringValue());
		}
	}

	void getPrivateProfileInt(const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval, CefString& exception)
	{
		assert(arguments.size() == 4);
		if (arguments.size() != 4)
		{
			exception = "invalid param";
			return;
		}

		int iVal = arguments[2]->GetIntValue();

		int iRet = GetPrivateProfileIntW(arguments[0]->GetStringValue().ToWString().c_str(),
			arguments[1]->GetStringValue().ToWString().c_str(), iVal,
			arguments[3]->GetStringValue().ToWString().c_str());

		retval = CefV8Value::CreateInt(iRet);
	}


	void writePrivateProfileString(const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval, CefString& exception)
	{
		assert(arguments.size() == 4);
		if (arguments.size() != 4)
		{
			exception = "invalid param";
			return;
		}

		std::wstring strPath = arguments[3]->GetStringValue().ToWString();
		auto iPos = strPath.rfind(L'\\');
		if (iPos != std::string::npos)
		{
			auto strParentDir = strPath.substr(0, iPos);
			if (-1 == _waccess(strParentDir.c_str(), 0))
			{
				SHCreateDirectoryExW(nullptr, strParentDir.c_str(), nullptr);
			}
		}

		int ret = !!WritePrivateProfileStringW(arguments[0]->GetStringValue().ToWString().c_str(),
			arguments[1]->GetStringValue().ToWString().c_str(),
			arguments[2]->GetStringValue().ToWString().c_str(),
			arguments[3]->GetStringValue().ToWString().c_str());

		retval = CefV8Value::CreateInt(ret);
	}

	void __DOMContentLoaded__(const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval, CefString& exception)
	{
		//LOG(INFO) << GetCurrentProcessId() << "] hello!!__DOMContentLoaded__ ";

		CefRefPtr<CefV8Context> context = CefV8Context::GetCurrentContext();
		CefRefPtr<CefFrame> frame = context->GetFrame();
		auto browser = frame->GetBrowser();

		const auto type = EasyRenderBrowserInfo::GetInstance().GetType(browser->GetIdentifier());

		if (type == EasyRenderBrowserInfo::BrsData::BROWSER_UI)
		{
			if (frame->IsMain()) {
				//解析一下内容
				ParseDOMGetAttr(frame);
			}
		}

		CefV8ValueList args;
		FowardRender2Browser(false, __func__, args, retval, exception);
	}



	void setWindowSize(const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval, CefString& exception)
	{
		HWND hWnd = JsGetWindowHwnd();
		if (IsWindow(hWnd))
		{
			const int x = arguments[0]->GetIntValue();
			const int y = arguments[1]->GetIntValue();
			const int width = arguments[2]->GetIntValue();
			const int height = arguments[3]->GetIntValue();

			const auto fixPos = LogicalToDevice({ x, y, width, height }, GetWindowScaleFactor(hWnd));

			//LOG(INFO) << "setWindowSize " << " x:" << x << " y:" << y << " w:" << width << " h:" << height 
			//	<< " fIX-> x:" << fixPos.x << " y:" << fixPos.y << " w:" << fixPos.width << " h:" << fixPos.height;

			if (SetWindowPos(hWnd, nullptr, fixPos.x, fixPos.y, fixPos.width, fixPos.height, SWP_NOZORDER | SWP_ASYNCWINDOWPOS))
			{
				retval = CefV8Value::CreateBool(true);
			}
		}
	}

	void minWindow(const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval, CefString& exception)
	{
		HWND hWnd = JsGetWindowHwnd();
		if (IsWindow(hWnd))
		{
			PostMessage(hWnd, WM_SYSCOMMAND, SC_MINIMIZE, NULL);
			retval = CefV8Value::CreateBool(true);
		}
	}

	void maxWindow(const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval, CefString& exception)
	{
		HWND hWnd = JsGetWindowHwnd();
		if (IsWindow(hWnd))
		{
			PostMessage(hWnd, WM_SYSCOMMAND, SC_MAXIMIZE, NULL);
			retval = CefV8Value::CreateBool(true);
		}
	}

	void restoreWindow(const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval, CefString& exception)
	{
		HWND hWnd = JsGetWindowHwnd();
		if (IsWindow(hWnd))
		{
			PostMessage(hWnd, WM_SYSCOMMAND, SC_RESTORE, NULL);
			retval = CefV8Value::CreateBool(true);
		}
	}

	void setWindowText(const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval, CefString& exception)
	{
		HWND hWnd = JsGetWindowHwnd();
		if (IsWindow(hWnd))
		{
			if (SetWindowTextW(hWnd, arguments[0]->GetStringValue().ToWString().c_str()))
			{
				retval = CefV8Value::CreateBool(true);
			}
		}
	}



	void __ContinueUnsecure__(const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval, CefString& exception)
	{
		//需要是主框架
		auto context = CefV8Context::GetCurrentContext();
		auto frame = context->GetFrame();

		if (frame->IsMain())
		{
			DomainPackInfo::Uri url(frame->GetURL());
			if (url.Protocol() == EASYCEFSCHEMEW)
			{
				FowardRender2Browser(false, __func__, arguments, retval, exception);
				return;
			}
		}

		exception = "invalid request";
	}


	
}


bool NativeV8Handler::Execute(const CefString& name, CefRefPtr<CefV8Value> object, const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval, CefString& exception)
{

	//LOG(INFO) << GetCurrentProcessId() << "] NativeV8Handler::Execute:" << name << " argc:" << arguments.size();

	auto item = m_mapFuncs.find(name.ToString());

	if (item != m_mapFuncs.end())
	{
		if (item->second.first)
		{
			item->second.first(arguments, retval, exception);
		}
		else
		{
			FowardRender2Browser(item->second.second, name, arguments, retval, exception);
		}

		return true;
	}

	exception = "not native implement function";

	return false;
}

NativeV8Handler::NativeV8Handler()
{
}

void NativeV8Handler::RegisterFunctions(CefRefPtr<CefV8Value> obj, int BrowserType)
{
	const auto attributes = static_cast<CefV8Value::PropertyAttribute>(
		V8_PROPERTY_ATTRIBUTE_READONLY |
		V8_PROPERTY_ATTRIBUTE_DONTENUM |
		V8_PROPERTY_ATTRIBUTE_DONTDELETE);

	//同步的修改处理方式，以便绕过js调用
	// 1.只注册ui， 2.只注册控件模式窗口, 3.两种窗口都注册
#define REG_SYNCJS_FUN(fnName, regtype) \
	if (regtype & BrowserType){\
		m_mapFuncs.insert(std::make_pair(#fnName,std::make_pair(nullptr,true)));\
		obj->SetValue(#fnName, CefV8Value::CreateFunction(#fnName, this), attributes);\
	} while(0)

	//要求browser处理的异步接口
#define REG_ASYNCJS_FUN(fnName, regtype) \
	if (regtype & BrowserType){\
		m_mapFuncs.insert(std::make_pair(#fnName, std::make_pair(nullptr,false)));\
		obj->SetValue(#fnName, CefV8Value::CreateFunction(#fnName, this), attributes);\
	} while(0)

	//render进程可以直接处理的接口
#define REG_JS_FUN(fnName, regtype) \
	if (regtype & BrowserType){\
		m_mapFuncs.insert(std::make_pair(std::string(#fnName), std::make_pair(JSCallFunctions::fnName,false)));\
		obj->SetValue(#fnName, CefV8Value::CreateFunction(#fnName, this), attributes);\
	} while(0)





	//不需要返回值异步处理
	REG_ASYNCJS_FUN(__dbgmode__, 1);

	REG_JS_FUN(writePrivateProfileString, 1);
	REG_JS_FUN(getPrivateProfileInt, 1);
	REG_JS_FUN(getPrivateProfileString, 1);

	//这几个调整一下直接放本进程处理
	REG_JS_FUN(minWindow, 1);
	REG_JS_FUN(maxWindow, 1);
	REG_JS_FUN(restoreWindow, 1);
	REG_JS_FUN(setWindowSize, 1);
	REG_JS_FUN(setWindowText, 1);



	REG_JS_FUN(__ContinueUnsecure__, 3);

}

void NativeV8Handler::RegisterUserFunctions(CefRefPtr<CefV8Value> obj, CefRefPtr<CefDictionaryValue> list, bool bSync, int BrowserType)
{
	if (!list)
		return;

	CefDictionaryValue::KeyList keyList;
	list->GetKeys(keyList);

	for (const auto& name : keyList)
	{
		if (list->GetInt(name) & BrowserType)
		{
			m_mapFuncs.insert(std::make_pair(name, std::make_pair(nullptr, bSync)));
			obj->SetValue(name, CefV8Value::CreateFunction(name, this), V8_PROPERTY_ATTRIBUTE_READONLY);
		}
	}
}

bool GobalNativeV8Handler::Execute(const CefString& name, CefRefPtr<CefV8Value> object, const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval, CefString& exception)
{
	//LOG(INFO) << GetCurrentProcessId() << "] GobalNativeV8Handler::Execute " << name;

	auto item = m_mapFuncs.find(name.ToString());

	if (item != m_mapFuncs.end())
	{
		item->second(arguments, retval, exception);
		return true;
	}


	return false;
}

GobalNativeV8Handler::GobalNativeV8Handler()
{
#define REG_GOBAL_JS_FUN(fnName) \
		m_mapFuncs.insert(std::make_pair(std::string(#fnName), JSCallFunctions::fnName))

	//用于本地通知
	REG_GOBAL_JS_FUN(__DOMContentLoaded__);
}
