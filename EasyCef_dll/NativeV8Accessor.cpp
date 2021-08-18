#include "pch.h"
#include "NativeV8Accessor.h"
#include "EasyIPC.h"

namespace JSKeysGet
{
#define DEFINE_GETKEY_SYNC_IMPL(keyName) bool keyName(CefRefPtr<CefV8Value>& retval)\
	{ return SyncGetKeyValue(#keyName, retval); }

	bool SyncGetKeyValue(const CefString &name, CefRefPtr<CefV8Value>& retval)
	{
		CefRefPtr<CefV8Context> context = CefV8Context::GetCurrentContext();
		CefRefPtr<CefFrame> frame = context->GetFrame();

		CefRefPtr<CefListValue> valueList = CefListValue::Create();
		valueList->SetSize(1);
		valueList->SetString(0, name);

		auto strData = QuickMakeIpcParms(frame->GetBrowser()->GetIdentifier(), frame->GetIdentifier(), "__V8AccessorAttribKeys__", valueList);

		std::string strOut;
		if (EasyIPCClient::GetInstance().SendDataToServer(strData, strOut))
		{
			if (!strOut.empty())
			{
				auto recVal = CefParseJSON(strOut, JSON_PARSER_RFC);
				if (recVal)
					retval = CefValueToCefV8Value(recVal);
				else
					retval = CefV8Value::CreateString(strOut);

				return true;
			}

			retval = CefV8Value::CreateString("");
			return true;
		}

		return false;
	}


	bool appname(CefRefPtr<CefV8Value>& retval)
	{
		//获取主进程exe名称，无需传送
		static CefString strVal;
		if (strVal.length() == 0)
		{
			std::wstring str;
			GetParentProcessInfo(nullptr, &str);
			auto pos = str.rfind('\\');

			strVal = str.substr(pos + 1, str.size() - pos - 1);
		}

		retval = CefV8Value::CreateString(strVal);

		return true;
	}

	bool appDir(CefRefPtr<CefV8Value>& retval)
	{
		//获取主进程exe所在路径，无需传送
		static CefString strVal;
		if (strVal.length() == 0)
		{
			std::wstring str;
			GetParentProcessInfo(nullptr, &str);
			auto pos = str.rfind('\\');

			strVal = str.substr(0, pos + 1);
		}

		retval = CefV8Value::CreateString(strVal);

		return true;
	}

	bool screen_w(CefRefPtr<CefV8Value>& retval)
	{
		int val = GetSystemMetrics(SM_CXSCREEN);
		retval = CefV8Value::CreateInt(val);
		return true;
	}

	bool screen_h(CefRefPtr<CefV8Value>& retval)
	{
		int val = GetSystemMetrics(SM_CYSCREEN);
		retval = CefV8Value::CreateInt(val);
		return true;
	}

	bool desktop_w(CefRefPtr<CefV8Value>& retval)
	{
		RECT rcScr;
		SystemParametersInfo(SPI_GETWORKAREA, 0, &rcScr, 0);
		retval = CefV8Value::CreateInt(rcScr.right - rcScr.left);
		return true;
	}

	bool desktop_h(CefRefPtr<CefV8Value>& retval)
	{
		RECT rcScr;
		SystemParametersInfo(SPI_GETWORKAREA, 0, &rcScr, 0);
		retval = CefV8Value::CreateInt(rcScr.bottom - rcScr.top);
		return true;
	}

	DEFINE_GETKEY_SYNC_IMPL(window_x)
	DEFINE_GETKEY_SYNC_IMPL(window_y)
	DEFINE_GETKEY_SYNC_IMPL(window_w)
	DEFINE_GETKEY_SYNC_IMPL(window_h)
	DEFINE_GETKEY_SYNC_IMPL(is_zoomed)
	DEFINE_GETKEY_SYNC_IMPL(is_iconic)

	DEFINE_GETKEY_SYNC_IMPL(appDataPath)

	bool ver(CefRefPtr<CefV8Value>& retval)
	{
		retval = CefV8Value::CreateInt(2000);
		return true;
	}

}

namespace JSKeysSet
{

	bool nc_setalledge(const CefRefPtr<CefV8Value> value)
	{
		auto context = CefV8Context::GetCurrentContext();
		auto frame = context->GetFrame();

		CefRefPtr<CefProcessMessage> msg = CefProcessMessage::Create("__V8AccessorSetAttribKeys__");

		auto valueList = msg->GetArgumentList();

		valueList->SetString(0, __func__);

		auto toVal = CefV8ValueToCefValue(value);

		valueList->SetValue(1, toVal);

		frame->SendProcessMessage(PID_BROWSER, msg);

		return true;
	}
}


bool NativeV8Accessor::Get(const CefString& name, const CefRefPtr<CefV8Value> object, CefRefPtr<CefV8Value>& retval, CefString& exception)
{
	auto it = m_mapGetFuncs.find(name);
	if (it != m_mapGetFuncs.end() && it->second)
	{
		if (it->second(retval))
			return true;
	}

	exception = "not valid native key";

	return false;
}

bool NativeV8Accessor::Set(const CefString& name, const CefRefPtr<CefV8Value> object, const CefRefPtr<CefV8Value> value, CefString& exception)
{
	auto it = m_mapSetFuncs.find(name);
	if (it != m_mapSetFuncs.end() && it->second)
	{
		if (it->second(value))
			return true;
	}

	exception = "not valid native key";

	return false;
}

void NativeV8Accessor::RegisterKeys(CefRefPtr<CefV8Value> obj)
{
	const auto attributes_r = static_cast<CefV8Value::PropertyAttribute>(
		V8_PROPERTY_ATTRIBUTE_READONLY |
		V8_PROPERTY_ATTRIBUTE_DONTENUM |
		V8_PROPERTY_ATTRIBUTE_DONTDELETE);

	const auto attributes_rw = static_cast<CefV8Value::PropertyAttribute>(
		V8_PROPERTY_ATTRIBUTE_DONTENUM |
		V8_PROPERTY_ATTRIBUTE_DONTDELETE);

#define REG_JS_KEY_R(kyName) \
	m_mapGetFuncs.insert(std::make_pair(std::string(#kyName), JSKeysGet::kyName));\
	obj->SetValue(#kyName, V8_ACCESS_CONTROL_ALL_CAN_READ, attributes_r)
	
#define REG_JS_KEY_W(kyName) \
	m_mapSetFuncs.insert(std::make_pair(std::string(#kyName), JSKeysSet::kyName)); \
	obj->SetValue(#kyName, V8_ACCESS_CONTROL_ALL_CAN_WRITE, attributes_rw)

#define REG_JS_KEY_RW(kyName) \
	m_mapGetFuncs.insert(std::make_pair(std::string(#kyName), JSKeysGet::kyName));\
	m_mapSetFuncs.insert(std::make_pair(std::string(#kyName), JSKeysSet::kyName));\
	obj->SetValue(#kyName, CefV8Value::AccessControl(V8_ACCESS_CONTROL_ALL_CAN_READ|V8_ACCESS_CONTROL_ALL_CAN_WRITE), attributes_rw)

	REG_JS_KEY_R(appname);
	REG_JS_KEY_R(appDir);
	REG_JS_KEY_R(appDataPath);
	REG_JS_KEY_R(screen_w);
	REG_JS_KEY_R(screen_h);
	REG_JS_KEY_R(desktop_w);
	REG_JS_KEY_R(desktop_h);
	REG_JS_KEY_R(window_x);
	REG_JS_KEY_R(window_y);
	REG_JS_KEY_R(window_w);
	REG_JS_KEY_R(window_h);
	REG_JS_KEY_R(is_zoomed);
	REG_JS_KEY_R(is_iconic);
	REG_JS_KEY_R(ver);

	REG_JS_KEY_W(nc_setalledge);
}
