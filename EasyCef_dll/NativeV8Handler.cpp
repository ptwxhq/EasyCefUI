#include "pch.h"
#include "NativeV8Handler.h"
#include <ShlObj_core.h>
#include "EasyIPC.h"
#include "EasyRenderBrowserInfo.h"
#include "LegacyImplement.h"
#include "EasyIPCWorks.h"

void call_FrameStateChanged(CefRefPtr<CefFrame>& frame, const char* frameName, const char* url, const int& code, bool didComit);


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
		else if (item->IsString() || item->IsDate())
		{
			toargs->SetString(index, item->GetStringValue());
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
		auto strData = QuickMakeIpcParms(frame->GetBrowser()->GetIdentifier(), frame->GetIdentifier(), name, valueList);

		//LOG(INFO) << GetCurrentProcessId() << "] FowardRender2Browser tosend:" << strData;

		std::string strOut;
		if (EasyIPCClient::GetInstance().SendDataToServer(strData, strOut))
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
//
//class TestDOMVisitor : public CefDOMVisitor
//{
//public:
//	explicit TestDOMVisitor(CefRefPtr<CefBrowser> browser)
//		: browser_(browser) {}
//
//	IMPLEMENT_REFCOUNTING(TestDOMVisitor);
//
//	CefRefPtr<CefBrowser> browser_;
//
//
//	void TestBodyNodeStructure(CefRefPtr<CefDOMNode> bodyNode) {
//
//		do
//		{
//			if (!bodyNode)break;
//			if (!bodyNode->IsElement())break;
//			if (!bodyNode->HasChildren())break;
//
//			CefRefPtr<CefDOMNode> curNode = bodyNode->GetFirstChild();
//			while (curNode)
//			{
//				 if(curNode->Ha)
//			}
//
//
//		} while (false);
//
//		CefRefPtr<CefDOMNode> h1Node = bodyNode->GetFirstChild();
//		EXPECT_TRUE(h1Node.get());
//		EXPECT_TRUE(h1Node->IsElement());
//		EXPECT_FALSE(h1Node->IsText());
//		EXPECT_EQ(h1Node->GetName(), "H1");
//		EXPECT_EQ(h1Node->GetElementTagName(), "H1");
//
//		EXPECT_TRUE(h1Node->GetNextSibling().get());
//		EXPECT_FALSE(h1Node->GetPreviousSibling().get());
//		EXPECT_TRUE(h1Node->HasChildren());
//		EXPECT_FALSE(h1Node->HasElementAttributes());
//
//		CefRefPtr<CefDOMNode> textNode = h1Node->GetFirstChild();
//		EXPECT_TRUE(textNode.get());
//		EXPECT_FALSE(textNode->IsElement());
//		EXPECT_TRUE(textNode->IsText());
//		EXPECT_EQ(textNode->GetValue(), "Hello From");
//
//		EXPECT_FALSE(textNode->GetPreviousSibling().get());
//		EXPECT_FALSE(textNode->HasChildren());
//
//		CefRefPtr<CefDOMNode> brNode = textNode->GetNextSibling();
//		EXPECT_TRUE(brNode.get());
//		EXPECT_TRUE(brNode->IsElement());
//		EXPECT_FALSE(brNode->IsText());
//		EXPECT_EQ(brNode->GetName(), "BR");
//		EXPECT_EQ(brNode->GetElementTagName(), "BR");
//
//		EXPECT_FALSE(brNode->HasChildren());
//
//		EXPECT_TRUE(brNode->HasElementAttributes());
//		EXPECT_TRUE(brNode->HasElementAttribute("class"));
//		EXPECT_EQ(brNode->GetElementAttribute("class"), "some_class");
//		EXPECT_TRUE(brNode->HasElementAttribute("id"));
//		EXPECT_EQ(brNode->GetElementAttribute("id"), "some_id");
//		EXPECT_FALSE(brNode->HasElementAttribute("no_existing"));
//
//		CefDOMNode::AttributeMap map;
//		brNode->GetElementAttributes(map);
//		ASSERT_EQ(map.size(), (size_t)2);
//		EXPECT_EQ(map["class"], "some_class");
//		EXPECT_EQ(map["id"], "some_id");
//
//		// Can also retrieve by ID.
//		brNode = bodyNode->GetDocument()->GetElementById("some_id");
//		EXPECT_TRUE(brNode.get());
//		EXPECT_TRUE(brNode->IsElement());
//		EXPECT_FALSE(brNode->IsText());
//		EXPECT_EQ(brNode->GetName(), "BR");
//		EXPECT_EQ(brNode->GetElementTagName(), "BR");
//
//		textNode = brNode->GetNextSibling();
//		EXPECT_TRUE(textNode.get());
//		EXPECT_FALSE(textNode->IsElement());
//		EXPECT_TRUE(textNode->IsText());
//		EXPECT_EQ(textNode->GetValue(), "Main Frame");
//
//		EXPECT_FALSE(textNode->GetNextSibling().get());
//		EXPECT_FALSE(textNode->HasChildren());
//
//		CefRefPtr<CefDOMNode> divNode = h1Node->GetNextSibling();
//		EXPECT_TRUE(divNode.get());
//		EXPECT_TRUE(divNode->IsElement());
//		EXPECT_FALSE(divNode->IsText());
//		CefRect divRect = divNode->GetElementBounds();
//		EXPECT_EQ(divRect.width, 50);
//		EXPECT_EQ(divRect.height, 25);
//		EXPECT_EQ(divRect.x, 150);
//		EXPECT_EQ(divRect.y, 100);
//		EXPECT_FALSE(divNode->GetNextSibling().get());
//	}
//
//	// Test document structure by iterating through the DOM tree.
//	void TestStructure(CefRefPtr<CefDOMDocument> document) {
//		
//
//		CefRefPtr<CefDOMNode> bodyNode = document->GetBody();
//		TestBodyNodeStructure(bodyNode);
//	}
//
//
//
//	void Visit(CefRefPtr<CefDOMDocument> document) override {
//		
//		TestStructure(document);
//
//		CefRefPtr<CefProcessMessage> return_msg =
//			CefProcessMessage::Create("get_result!!");
//		//EXPECT_TRUE(return_msg->GetArgumentList()->SetBool(0, result));
//		browser_->GetMainFrame()->SendProcessMessage(PID_BROWSER, return_msg);
//	}
//};

void ParseDOMGetAttr(CefRefPtr<CefFrame> frame)
{

	//frame->VisitDOM(new TestDOMVisitor(frame->GetBrowser()));
   auto testJs = R"((
    function () {
        const edges = [
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

        function updateNcEleRects(myArray) {
            var notify_nc_alledge = new Object();
            myArray.forEach(ele => {
                let attr = ele.getAttribute("data-nc");
                let drect = ele.getBoundingClientRect();

                if (!isArray(!notify_nc_alledge[attr]))
                    notify_nc_alledge[attr] = new Array();

                notify_nc_alledge[attr].push({ left: drect.left, top: drect.top, right: drect.right, bottom: drect.bottom });
            });

            nativeapp.nc_setalledge = notify_nc_alledge;
            console.log("updateNcEleRects" + JSON.stringify(notify_nc_alledge));
            return notify_nc_alledge;
        }

        var tmp_nc_alledge = new Object();
        var eleList = new Array();

        var timeoutID;

        const resizeObserver = new ResizeObserver(entries => {

            //console.log("hello settimer");

            window.clearTimeout(timeoutID);
            timeoutID = window.setTimeout(updateNcEleRects, 200, eleList);

            //var callbacktmp_alledge = new Object();

            //  for (let entry of entries) {
            //	let ele = entry.target;
            //	let attr=ele.getAttribute("data-nc");
            //	let drect = ele.getBoundingClientRect();

            //	if(!isArray(!callbacktmp_alledge[attr]))
            //		callbacktmp_alledge[attr] = new Array();

            //	callbacktmp_alledge[attr].push({left:drect.left,top:drect.top,right:drect.right,bottom:drect.bottom});

            //console.log(attr + JSON.stringify(drect));

            //  }

            //nativeapp.nc_setalledge =  callbacktmp_alledge;

            //console.log("from chg:"+JSON.stringify(callbacktmp_alledge));

        });


        for (let index = 0; index < edges.length; ++index) {
            let eles = document.body.querySelectorAll("div." + edges[index].cls);
            for (let i = 0; i < eles.length; ++i) {
                let ele = eles[i];
                let attr = ele.getAttribute("data-nc");
                if (attr == edges[index].nc) {
                    eleList.push(ele);
                    resizeObserver.observe(ele);

                    //console.log(JSON.stringify(ele.getBoundingClientRect()));
                    //let drect = ele.getBoundingClientRect(); 

                    //if(!isArray(!tmp_nc_alledge[attr]))
                    //	tmp_nc_alledge[attr] = new Array();

                    //tmp_nc_alledge[attr].push({left:drect.left,top:drect.top,right:drect.right,bottom:drect.bottom});

                }
            }
        }

        //nativeapp.nc_setalledge = tmp_nc_alledge;
        //console.log("from init:" +JSON.stringify(tmp_nc_alledge));
    }()
))";

   frame->ExecuteJavaScript(testJs, "", 0);


   //同时还要监视变化

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

	void addFrameStateChanged(const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval, CefString& exception)
	{
		if (arguments.size() != 1 || !arguments[0]->IsString())
		{
			exception = "invalid param";
			return;
		}


		CefRefPtr<CefV8Context> context = CefV8Context::GetCurrentContext();
		CefRefPtr<CefFrame> frame = context->GetFrame();

		std::string id = arguments[0]->GetStringValue();
		std::hash<std::string> string_hash;
		unsigned int uid = string_hash(id);
		bool bAdd = DectetFrameLoad::getInst().Add(frame->GetBrowser()->GetIdentifier(), getFramePath(frame), uid, frame->GetIdentifier());
		retval = CefV8Value::CreateInt(bAdd ? 1 : 0);
	}

	void removeFrameStateChanged(const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval, CefString& exception)
	{
		if (arguments.size() != 1 || !arguments[0]->IsString())
		{
			exception = "invalid param";
			return;
		}
		CefRefPtr<CefV8Context> context = CefV8Context::GetCurrentContext();
		CefRefPtr<CefFrame> frame = context->GetFrame();

		std::string id = arguments[0]->GetStringValue();
		std::hash<std::string> string_hash;
		unsigned int uid = string_hash(id);
		bool bAdd = DectetFrameLoad::getInst().Remove(frame->GetBrowser()->GetIdentifier(), getFramePath(frame), uid);
		retval = CefV8Value::CreateInt(bAdd ? 1 : 0);
	}

	void queryProduct(const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval, CefString& exception)
	{
		retval = CefV8Value::CreateString("cyjh");
	}



	void __DOMContentLoaded__(const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval, CefString& exception)
	{
		//LOG(INFO) << GetCurrentProcessId() << "] hello!!__DOMContentLoaded__ ";

		CefRefPtr<CefV8Context> context = CefV8Context::GetCurrentContext();
		CefRefPtr<CefFrame> frame = context->GetFrame();
		auto browser = frame->GetBrowser();
		auto url = frame->GetURL().ToString();

		std::wstring strMainUrl;
		auto mainframe = browser->GetMainFrame();
		if (mainframe)
		{
			strMainUrl = mainframe->GetURL().ToWString();
		}

		const auto type = EasyRenderBrowserInfo::GetInstance().GetType(browser->GetIdentifier());

		if (type == EasyRenderBrowserInfo::BrsData::BROWSER_UI)
		{
			if (frame->IsMain()) {
				DocComplate::getInst().setBrowsr(browser->GetIdentifier(), true);

				//解析一下内容
				ParseDOMGetAttr(frame);
			}

			CefRefPtr<CefFrame> parent = frame->GetParent();
			if (parent)
			{
				std::hash<std::string> string_hash;
				//std::string frameNam = frame->GetName().ToString();
				std::string frameNam = RecordFrameName::getInst().GetRecord(browser->GetIdentifier(), frame->GetIdentifier());
				unsigned int id = string_hash(frameNam);
				if (DectetFrameLoad::getInst().hit(browser->GetIdentifier(), getFramePath(parent), id, 200)) {
					call_FrameStateChanged(parent, frameNam.c_str(), url.c_str(), 200, false);
				}
			}
		}
		else if (type == EasyRenderBrowserInfo::BrsData::BROWSER_WEB)
		{

		}

		//接下来注入js
		CefV8ValueList args;
		FowardRender2Browser(false, __func__, args, retval, exception);
	
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

	//引入原有的同步接口

	REG_SYNCJS_FUN(crossInvokeWebMethod, 1);
	REG_SYNCJS_FUN(crossInvokeWebMethod2, 1);

	REG_SYNCJS_FUN(invokeMethod, 3);

	REG_SYNCJS_FUN(winProty, 1);
	REG_SYNCJS_FUN(setProfile, 3);
	REG_SYNCJS_FUN(getProfile, 3);
	REG_SYNCJS_FUN(getSoftwareAttribute, 1);

	//不需要返回值，改异步处理
	REG_ASYNCJS_FUN(minWindow, 1);
	REG_ASYNCJS_FUN(maxWindow, 1);
	REG_ASYNCJS_FUN(restoreWindow, 1);
	REG_ASYNCJS_FUN(setWindowSize, 1);
	REG_ASYNCJS_FUN(setWindowPos, 1);
	//REG_ASYNCJS_FUN(fullScreen, 1);
	REG_ASYNCJS_FUN(createWindow, 1);
	REG_ASYNCJS_FUN(createModalWindow, 1);
	REG_ASYNCJS_FUN(createModalWindow2, 1);
	REG_ASYNCJS_FUN(setAlpha, 1);


	REG_ASYNCJS_FUN(closeWindow, 1);
	REG_ASYNCJS_FUN(setWindowText, 1);

	REG_ASYNCJS_FUN(asyncCrossInvokeWebMethod, 1);
	REG_ASYNCJS_FUN(asyncCrossInvokeWebMethod2, 1);

	REG_ASYNCJS_FUN(asyncCallMethod, 3); 


	if (g_BrowserGlobalVar.Debug)
	{
		REG_ASYNCJS_FUN(addCrossOriginWhitelistEntry, 1);//本次新增
		REG_ASYNCJS_FUN(removeCrossOriginWhitelistEntry, 1);//本次新增
	}



	REG_JS_FUN(addFrameStateChanged, 1);
	REG_JS_FUN(removeFrameStateChanged, 1);
	REG_JS_FUN(writePrivateProfileString, 1);
	REG_JS_FUN(getPrivateProfileInt, 1);
	REG_JS_FUN(getPrivateProfileString, 1);

	REG_JS_FUN(queryProduct, 2);





	//以下接口另外处理或者废弃
	//REG_JS_FUN(pushMessage, 1);	 //原先就实现，应该是用不上废弃
	//REG_SYNCJS_FUN(launchServerData, 1);//提交post请求，可以直接实现，这里废弃
	//REG_SYNCJS_FUN(abortServerData, 1);//旧版未实现，这里废弃
	//REG_SYNCJS_FUN(collectAllGarbage, 1); //未知是否还有意义，暂时先不接入

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
