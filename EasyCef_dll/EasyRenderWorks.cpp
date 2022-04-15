#include "pch.h"
#include "EasyRenderWorks.h"
#include "Export.h"

#include "EasyRenderBrowserInfo.h"

namespace RenderAsyncWorkFunctions
{
	void AdjustRenderSpeed(const CefRefPtr<CefBrowser> browser, const CefRefPtr<CefFrame> frame, const CefRefPtr<CefListValue>& args)
	{
		if (g_BrowserGlobalVar.funcSpeedupCallback)
		{
			g_BrowserGlobalVar.funcSpeedupCallback(static_cast<float>(args->GetDouble(0)));
		}
	}

}


namespace RenderSyncWorkFunctions
{
	bool invokedJSMethod(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefRefPtr<CefListValue>& args, CefString& retval)
	{
		//LOG(INFO) << GetCurrentProcessId() << "]EasyCefAppRender::InvokeJSFunction in" << args->GetString(0);
		if (!frame)
		{
			frame = browser->GetMainFrame();
		}
		auto v8Context = frame->GetV8Context();

		v8Context->Enter();

		auto window = v8Context->GetGlobal();
		auto funcInvoke = window->GetValue("invokeMethod");
		if (funcInvoke && funcInvoke->IsFunction())
		{
			CefV8ValueList v8args;
			for (size_t i = 0; i < 3; i++)
			{
				v8args.push_back(CefV8Value::CreateString(args->GetString(i)));
			}

			v8args.push_back(CefV8Value::CreateBool(args->GetBool(3)));

			//LOG(INFO) << GetCurrentProcessId() << "]EasyCefAppRender::InvokeJSFunction begin(" << args->GetString(0) << ")";

			auto retVal = funcInvoke->ExecuteFunction(window, v8args);
			if (retVal)
			{
				retval = CefV8ValueToString(retVal);

			
				//LOG(INFO) << GetCurrentProcessId() << "]EasyCefAppRender::InvokeJSFunction end(" << retval << ")";
			}
			else
			{
				LOG(WARNING) << GetCurrentProcessId() << "]EasyCefAppRender::InvokeJSFunction Failed(" << funcInvoke->GetException()->GetMessage() << ")";
				
			}

			
		}



		v8Context->Exit();

		return true;
	}

	bool queryElementAttrib(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefRefPtr<CefListValue>& args, CefString& retval)
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
		
#if HAVE_CPP_FORMAT
		auto js = R"((function (x,y) {{
var e=document.elementFromPoint(x,y);
if(e===null)return null;
else return e.getAttribute("{}");
}})({},{}))";

		strJS = std::format(js, args->GetString(0).ToString(), pt.x, pt.y);
#else
		std::ostringstream ss;
		ss << R"(function (x,y) {
var e=document.elementFromPoint(x,y);
if(e===null)return null;
else return e.getAttribute("{}");
})()" << pt.x << "," << pt.y << ")";

		strJS = ss.str();
#endif

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


	REG_SYNCWORK_FUNCTION(invokedJSMethod);
	REG_SYNCWORK_FUNCTION(queryElementAttrib);

//	REG_ASYNCWORK_FUNCTION(asyncInvokedJSMethod);
	REG_ASYNCWORK_FUNCTION(AdjustRenderSpeed);

}

EasyRenderWorks& EasyRenderWorks::GetInstance()
{
	static EasyRenderWorks obj;
	return obj;
}

void EasyRenderWorks::UIWork(std::shared_ptr<EasyIPCWorks::BRDataPack> pData, bool bNeedUIThread)
{
	if (!m_UIWorkInstance)
	{
		class RenderUIWorks : public UIWorks {
			void DoWork(std::shared_ptr<EasyIPCWorks::BRDataPack> pData) override {

	//			LOG(INFO) << GetCurrentProcessId() << "] EasyRenderWorks::UIWork  DoWork " << pData->Name;

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

					if (EasyRenderWorks::GetInstance().DoSyncWork(pData->Name, browser, frame, pData->Args, strReturn))
					{
						pData->ReturnVal = strReturn.ToString();
					}

				} while (false);

				pData->Signal.set_value();

			}
		};

		m_UIWorkInstance = new RenderUIWorks;
	}

	__super::UIWork(pData, bNeedUIThread);
}
