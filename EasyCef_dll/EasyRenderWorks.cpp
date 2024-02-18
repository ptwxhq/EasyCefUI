#include "pch.h"
#include "EasyRenderWorks.h"

#include "EasyRenderBrowserInfo.h"

namespace RenderAsyncWorkFunctions
{
	void __AdjustRenderSpeed__(const CefRefPtr<CefBrowser> browser, const CefRefPtr<CefFrame> frame, const CefRefPtr<CefListValue>& args)
	{
		if (g_BrowserGlobalVar.funcSpeedupCallback)
		{
			g_BrowserGlobalVar.funcSpeedupCallback(static_cast<float>(args->GetDouble(0)));
		}
	}

}


namespace RenderSyncWorkFunctions
{
	bool __InvokedJSMethod__(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefRefPtr<CefListValue>& args, CefString& retval)
	{
		LOG(INFO) << GetCurrentProcessId() << "]EasyCefAppRender::InvokeJSFunction in " << args->GetString(0);
		const auto nArgs = args->GetSize();
		if (nArgs < 1)
			return false;


		if (!frame)
		{
			LOG(INFO) << GetCurrentProcessId() << "] USE GetMainFrame ";
			frame = browser->GetMainFrame();
		}
		else
		{
			LOG(INFO) << "f url:" << frame->GetURL();
		}
		auto v8Context = frame->GetV8Context();

		v8Context->Enter();

		auto window = v8Context->GetGlobal();
		auto funcInvoke = window->GetValue(args->GetString(0));
		if (funcInvoke && funcInvoke->IsFunction())
		{
			CefV8ValueList v8args;
			for (size_t i = 1; i < nArgs; i++)
			{
				v8args.push_back(CefValueToCefV8Value(args->GetValue(i)));
			}

			auto retVal = funcInvoke->ExecuteFunction(window, v8args);
			if (retVal)
			{
				retval = CefV8ValueToString(retVal);
			
				LOG(INFO) << GetCurrentProcessId() << "]EasyCefAppRender::InvokeJSFunction end(" << retval << ")";
			}
			else
			{
				LOG(WARNING) << GetCurrentProcessId() << "]EasyCefAppRender::InvokeJSFunction Failed(" << funcInvoke->GetException()->GetMessage() << ")";
				
			}

			
		}
		else
		{
			LOG(WARNING) << GetCurrentProcessId() << "]EasyCefAppRender::InvokeJSFunction NotFunction";

		}



		v8Context->Exit();

		LOG(INFO) << GetCurrentProcessId() << "]EasyCefAppRender::InvokeJSFunction End)";


		return true;
	}

	bool __queryElementAttrib__(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefRefPtr<CefListValue>& args, CefString& retval)
	{
//		LOG(INFO) << GetCurrentProcessId() << "]EasyCefAppRender::queryElementAttrib (" << 1 << ")";

		auto v8Context = frame->GetV8Context();

		v8Context->Enter();

		POINT pt = { args->GetInt(1), args->GetInt(2) };

	//	LOG(INFO) << GetCurrentProcessId() << "]ClientAppRenderer::OnProcessMessageReceived(" << "(" << pt.x << "," << pt.y << ")";
		

		CefRefPtr<CefV8Value> jsRetval;
		CefRefPtr<CefV8Exception> exception;

		//后面看看怎么用function替代Eval
		//auto dwTimeTest1 = GetTimeNow();

		std::string strJS;

		strJS = std::format(R"((function (x,y) {{
var e=document.elementFromPoint(x,y);
if(e===null)return null;
else return e.getAttribute("{}");
}})({},{}))", args->GetString(0).ToString(), pt.x, pt.y);


		bool bOpr = v8Context->Eval(strJS, CefString(), 0, jsRetval, exception);
			
			

		//auto dwTimeTest2 = GetTimeNow();

		//LOG(INFO) << GetCurrentProcessId() << "]v8Context->Eval cost:(" << dwTimeTest2- dwTimeTest1 << ")" << dwTimeTest2;

		if (bOpr && jsRetval && jsRetval->IsString())
		{
			retval = jsRetval->GetStringValue();

		}
		else
		{
			retval.clear();
		}

		v8Context->Exit();

	//	LOG(INFO) << GetCurrentProcessId() << "]EasyCefAppRender::queryElementAttrib fin(" << bOpr << ")[" << retval << "]";

		return true;
	}



}



EasyRenderWorks::EasyRenderWorks()
{
#define REG_SYNCWORK_FUNCTION(fnName) \
	m_mapSyncFuncs.insert(std::make_pair(std::string(#fnName), RenderSyncWorkFunctions::fnName))

#define REG_ASYNCWORK_FUNCTION(fnName) \
	m_mapAsyncFuncs.insert(std::make_pair(std::string(#fnName), RenderAsyncWorkFunctions::fnName))


	REG_SYNCWORK_FUNCTION(__InvokedJSMethod__);
	REG_SYNCWORK_FUNCTION(__queryElementAttrib__);

	REG_ASYNCWORK_FUNCTION(__AdjustRenderSpeed__);

}

EasyRenderWorks& EasyRenderWorks::GetInstance()
{
	static EasyRenderWorks obj;
	return obj;
}

void EasyRenderWorks::DoWork(std::shared_ptr<EasyIPCWorks::BRDataPack> pData) 
{

	//LOG(INFO) << GetCurrentProcessId() << "] EasyRenderWorks::UIWork  DoWork " << pData->Name;

	if (pData->DataInvalid)
		return;

	do
	{
		auto browser = EasyRenderBrowserInfo::GetInstance().GetBrowser(pData->BrowserId);
		if (!browser)
			break;

		CefRefPtr<CefFrame> frame;
		if (pData->FrameId > -1)
		{
			frame = browser->GetFrame(pData->FrameId);
		}

		CefString strReturn;

		if (DoSyncWork(pData->Name, browser, frame, pData->Args, strReturn))
		{
			pData->ReturnVal = strReturn.ToString();
		}

	} while (false);

	if (pData->Future.valid())
		pData->Signal.set_value();

}
