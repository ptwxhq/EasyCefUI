#include "pch.h"
#include "EasyClientHandler.h"
#include <include\wrapper\cef_helpers.h>
#include <include\cef_parser.h>
#include <include\cef_app.h>
#include <ShlObj.h>
#include <regex>
#include "EasyIPC.h"
#include "LegacyImplement.h"
#include "EasyLayeredWindow.h"
#include "WebViewControl.h"
#include "EasyWebViewMgr.h"
#include "EasyBrowserWorks.h"
#include "include/cef_resource_bundle.h"
#include "include/cef_pack_strings.h"
#include "EasySchemes.h"
#include "EasyReqRespModify.h"


bool QueryNodeAttrib(CefRefPtr<WebViewControl> item, int x, int y, std::string name, std::wstring& outVal)
{
    auto& ipcSvr = EasyIPCServer::GetInstance();
    auto hipcli = ipcSvr.GetClientHandle(item->GetBrowser()->GetIdentifier());
    if (hipcli)
    {
        CefRefPtr<CefListValue> valueList = CefListValue::Create();

        valueList->SetString(0, name.empty() ? "data-nc" : name);

        valueList->SetInt(1, x);
        valueList->SetInt(2, y);

        auto frame = item->GetBrowser()->GetMainFrame();

        auto send = QuickMakeIpcParms(item->GetBrowser()->GetIdentifier(), frame->GetIdentifier(), "queryElementAttrib", valueList);
        std::string ret;
        ipcSvr.SendData(hipcli, send, ret);

        if (ret.size() > 0)
        {
            CefString tmp(ret);
            outVal = tmp.ToWString();
            return true;
        }

    }

    outVal.clear();

    return false;
}

enum client_menu_ids {
    CLIENT_ID_SHOW_DEVTOOLS = MENU_ID_USER_FIRST,
    CLIENT_ID_CLOSE_DEVTOOLS,
    CLIENT_ID_INSPECT_ELEMENT,

   // CLIENT_ID_ROLOAD_FRAME = MENU_ID_USER_LAST - 10,


    

};

CefString GetLabel(int message_id) {
    auto label =
        CefResourceBundle::GetGlobal()->GetLocalizedString(message_id);
    DCHECK(!label.empty());
    return label;
}


bool EasyClientHandler::OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefProcessId source_process, CefRefPtr<CefProcessMessage> message)
{
    const std::string& message_name = message->GetName();

    if (message_name == "ContinueUnsecure")
    {
        auto url = message->GetArgumentList()->GetString(0).ToWString();
        if (!url.empty())
        {
            auto domain = DomainPackInfo::GetFormatedDomain(url.c_str());
            s_listAllowUnsecure.insert(domain);
            frame->LoadURL(url);
            return true;
        }
        return false;
    }


    return EasyBrowserWorks::GetInstance().DoAsyncWork(message_name, browser, frame, message->GetArgumentList());
}

bool EasyClientHandler::OnBeforePopup(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString& target_url, const CefString& target_frame_name, CefLifeSpanHandler::WindowOpenDisposition target_disposition, bool user_gesture, const CefPopupFeatures& popupFeatures, CefWindowInfo& windowInfo, CefRefPtr<CefClient>& client, CefBrowserSettings& settings, CefRefPtr<CefDictionaryValue>& extra_info, bool* no_javascript_access)
{
    CEF_REQUIRE_UI_THREAD();

    //LOG(INFO) << GetCurrentProcessId() << "] EasyClientHandler::OnBeforePopup:"
    //    << std::format(R"(target_disposition:{},user_gesture:{},menuBar:{},statusBar:{},toolBar:{},scrollbars:{})", (int)target_disposition, user_gesture, popupFeatures.menuBarVisible, popupFeatures.statusBarVisible, popupFeatures.toolBarVisible, popupFeatures.scrollbarsVisible) 
    //    << "url:" << target_url;

    extra_info = CefDictionaryValue::Create();

    const auto tmpVal = EasyIPCServer::GetInstance().GetHandle();
    auto valKeyName = CefBinaryValue::Create(&tmpVal, sizeof(tmpVal));
    extra_info->SetBinary(IpcBrowserServerKeyName, valKeyName);
    extra_info->SetBool(ExtraKeyNameIsManagedPopup, true);


    //UI弹出的窗口要作为普通窗口还是UI的？当作UI的吧
    auto item = EasyWebViewMgr::GetInstance().GetItemBrowserById(browser->GetIdentifier());
    if (item)
    {
        HWND hWnd = item->GetHWND();
        if (item->IsUIControl())
        {
            //得限制一下不能让UI自己弹窗口，不然影响流程
            if (WebkitEcho::getUIFunMap() && WebkitEcho::getUIFunMap()->newNativeUrl && IsWindow(hWnd))
            {
                WebkitEcho::getUIFunMap()->newNativeUrl(hWnd, target_url.ToWString().c_str(), target_frame_name.c_str());
            }
        }
        else
        {
            bool bCancel = false;
            HWND hAttchWnd = nullptr;
            if (WebkitEcho::getFunMap() && WebkitEcho::getFunMap()->webkitNewTab)
            {
                //可以post信息的新窗口弹出
                bCancel = WebkitEcho::getFunMap()->webkitNewTab(item->GetBrowserId(), target_url.ToWString().c_str(), &hAttchWnd);
            }

            if (bCancel && IsWindow(hAttchWnd))
            {
                RECT rect = {};
                GetClientRect(hAttchWnd, &rect);
                windowInfo.SetAsChild(hAttchWnd,
                    EasyCefRect(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top)
                );

                //这边得使用相同的handler
                auto handle = EasyWebViewMgr::GetInstance().CreatePopWebViewControl(hAttchWnd, RECT(), target_url.ToWString().c_str(), this);
                if (handle)
                {
                    m_preCreatePopHandle.push_back(handle);
                    return false;
                }
    
            }

            if (!bCancel && WebkitEcho::getFunMap() && WebkitEcho::getFunMap()->webkitOpenNewUrl)
            {
                auto request_context = browser->GetHost()->GetRequestContext();
                std::wstring strCachePath;
                if (request_context)
                {
                    strCachePath = request_context->GetCachePath().ToWString();
                }
                //仅保留缓存cookie等，会丢失post等请求信息
                WebkitEcho::getFunMap()->webkitOpenNewUrl(item->GetBrowserId(), target_url.ToWString().c_str(), strCachePath.c_str());
                return true;
            }


            //均未设置的情况下使用默认弹出的方式
            auto handle = EasyWebViewMgr::GetInstance().CreatePopWebViewControl(nullptr,
#if CEF_VERSION_MAJOR > 95
                { windowInfo.bounds.x, windowInfo.bounds.y, windowInfo.bounds.width, windowInfo.bounds.height }
#else
                { windowInfo.x, windowInfo.y, windowInfo.width, windowInfo.height }
#endif
            , target_url.ToWString().c_str(), this);
            m_preCreatePopHandle.push_back(handle);

            return false;

        }

   

        return true;
    }


    return false;
}



void EasyClientHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser)
{
    CEF_REQUIRE_UI_THREAD();

   // ++m_BrowserCount;

  //  LOG(INFO) << GetCurrentProcessId() << "] EasyClientHandler::OnAfterCreated:(" << browser->GetIdentifier() << ")  " << m_BrowserCount;

    wvhandle handler = m_hManualCreateHandle;

    if (browser->IsPopup())
    {
        if (m_preCreatePopHandle.empty())
        {
            ASSERT(0);
        }

        handler = m_preCreatePopHandle.front();
     
        m_preCreatePopHandle.pop_front();
    }
    else
    {
        handler = m_hManualCreateHandle;
        m_browser = browser;
    }

    auto item = EasyWebViewMgr::GetInstance().GetItemBrowserByHandle(handler);
    ASSERT(item);
    if (item)
    {
        item->SetBrowser(browser);
    }

    auto browserhost = browser->GetHost();
    if (browserhost)
    {
        HWND hWnd = browserhost->GetWindowHandle();

        EasyWebViewMgr::GetInstance().AsyncSetIndexInfo(handler, browser->GetIdentifier(), hWnd);

        if (!m_bIsUIControl && WebkitEcho::getFunMap() && WebkitEcho::getFunMap()->webkitAfterCreate)
        {
            //检查是Chrome_WidgetWin_0 还是 RenderWidgeHostHWND，已确认是前者
            HWND hWidget = FindWindowExW(hWnd, nullptr, nullptr, nullptr);
            WebkitEcho::getFunMap()->webkitAfterCreate(GetParent(hWnd), hWnd, hWidget, browser->GetIdentifier());
        }
    }

   
}

void EasyClientHandler::OnBeforeClose(CefRefPtr<CefBrowser> browser)
{
    CEF_REQUIRE_UI_THREAD();

    LOG(INFO) << GetCurrentProcessId() << "] EasyClientHandler::OnBeforeClose:(" << browser->GetIdentifier() << ") begin ";

    bool bIsPop = browser->IsPopup();

    if (m_browser && m_browser->IsSame(browser))
    {
        //防止关闭时仍然有引用导致无法检查异常
        m_browser = nullptr;
    }

    if (bIsPop)
    {
    }
    else
    {
        //webui目前不会有popup，直接这样吧
        if (m_bIsUIControl)
        {
            //如果窗口当前未销毁说明还在WM_CLOSE中，需要延长生命周期以便能正常释放窗口
            if (IsWindow(m_webuicontrol->GetHWND()))
            {
                EasyWebViewMgr::GetInstance().AddDelayItem(m_webuicontrol);
            }

            m_webuicontrol = nullptr;
        }
    }

    //if (m_BrowserCount < 1)
    {
        EasyIPCServer::GetInstance().RemoveClient(browser->GetIdentifier());

        EasyWebViewMgr::GetInstance().RemoveWebViewByBrowserId(browser->GetIdentifier());
    }

     //CefBrowser
    if (EasyIPCServer::GetInstance().GetClientsCount())
    {
        if (WebkitEcho::getFunMap() && WebkitEcho::getFunMap()->webkitBeforeClose)
        {
            WebkitEcho::getFunMap()->webkitBeforeClose(browser->GetIdentifier());
        }
    }
    else
    {

        //保存下cookies
        auto request_context = CefRequestContext::GetGlobalContext();
        auto cookie_mgr = request_context->GetCookieManager(nullptr);
        if (cookie_mgr)
            cookie_mgr->FlushStore(nullptr);

        // All browser windows have closed. Quit the application message loop.
        //CefQuitMessageLoop();
        if (g_BrowserGlobalVar.funcCloseCallback)
        {
            if (EasyWebViewMgr::GetInstance().HaveDelayItem())
            {
                EasyWebViewMgr::GetInstance().CleanDelayItem(nullptr);
            }

            g_BrowserGlobalVar.funcCloseCallback();
        }
        


    }
    
}

void EasyClientHandler::OnAddressChange(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString& url)
{
    std::wstring strUrl = url;
    if (strUrl.substr(0, _countof(EASYCEFPROTOCOLW)).find(EASYCEFPROTOCOLW) == 0)
    {
        strUrl.clear();
    }

    if (!m_bIsUIControl)
    {
        if (frame->IsMain() && WebkitEcho::getFunMap() && WebkitEcho::getFunMap()->webkitChangeUrl)
        {
            WebkitEcho::getFunMap()->webkitChangeUrl(browser->GetIdentifier(), strUrl.c_str());
        }
    }
}

void EasyClientHandler::OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title)
{
    HWND hWnd = nullptr;
    if (m_bIsUIControl)
    {
        auto item = EasyWebViewMgr::GetInstance().GetItemBrowserById(browser->GetIdentifier());
        if (!item)
            return;

        hWnd = item->GetHWND();
    }
    else
    {
        if (WebkitEcho::getFunMap() && WebkitEcho::getFunMap()->webkitChangeTitle)
        {
            WebkitEcho::getFunMap()->webkitChangeTitle(browser->GetIdentifier(), title.ToWString().c_str());
        }
        else
        {
            //让未使用自定义弹出页面的弹窗可以显示标题
            if (browser->IsPopup())
                hWnd = browser->GetHost()->GetWindowHandle();
        }
    }

    if (hWnd)
        SetWindowTextW(hWnd, title.c_str());
}

void EasyClientHandler::OnFaviconURLChange(CefRefPtr<CefBrowser> browser, const std::vector<CefString>& icon_urls)
{
    if (!m_bIsUIControl)
    {
        if (WebkitEcho::getFunMap() && WebkitEcho::getFunMap()->webkitSiteIcon)
        {
            std::wstring strUrl;
            auto frame = browser->GetMainFrame();
            if (frame)
            {
                strUrl = frame->GetURL().ToWString();
            }
            WebkitEcho::getFunMap()->webkitSiteIcon(browser->GetIdentifier(), strUrl.c_str(), icon_urls[0].c_str());
        }
    }
}

bool EasyClientHandler::OnTooltip(CefRefPtr<CefBrowser> browser, CefString& text)
{
    if (m_bIsUIControl && m_bIsUITransparent)
    {
        CefRefPtr<WebViewTransparentUIControl> item = dynamic_cast<WebViewTransparentUIControl*>(EasyWebViewMgr::GetInstance().GetItemBrowserById(browser->GetIdentifier()).get());
        if (item)
        {
            item->SetToolTip(text);
            return true;
        }
    }

    return false;
}

bool EasyClientHandler::OnCursorChange(CefRefPtr<CefBrowser> browser, CefCursorHandle cursor, cef_cursor_type_t type, const CefCursorInfo& custom_cursor_info)
{
    CEF_REQUIRE_UI_THREAD();

    if (!(m_bIsUIControl && m_bIsUITransparent))
        return false;

    HWND hWnd = browser->GetHost()->GetWindowHandle();

    if (!::IsWindow(hWnd))
        return false;

    // Change the plugin window's cursor.
    SetClassLongPtr(hWnd, GCLP_HCURSOR,
        static_cast<LONG>(reinterpret_cast<LONG_PTR>(cursor)));
    SetCursor(cursor);
    return true;
}

bool EasyClientHandler::OnConsoleMessage(CefRefPtr<CefBrowser> browser, cef_log_severity_t level, const CefString& message, const CefString& source, int line)
{
    return !g_BrowserGlobalVar.Debug;
}

void EasyClientHandler::OnLoadingStateChange(CefRefPtr<CefBrowser> browser, bool isLoading, bool canGoBack, bool canGoForward)
{
    if (m_bIsUIControl)
        return;

    if (WebkitEcho::getFunMap() && WebkitEcho::getFunMap()->webkitLoadingStateChange)
    {
        WebkitEcho::getFunMap()->webkitLoadingStateChange(browser->GetIdentifier(), isLoading, canGoBack, canGoForward);
    }
}

void EasyClientHandler::OnLoadStart(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, TransitionType transition_type)
{
   // LOG(INFO) << GetCurrentProcessId() << "] OnLoadStart name:" << frame->GetName() << "(((" << transition_type << "||" << frame->GetURL() << "\n";
}

void EasyClientHandler::OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode)
{
   // LOG(INFO) << GetCurrentProcessId() << "] OnLoadEnd name:" << frame->GetName() << "(((" << httpStatusCode << "||" << frame->GetURL() << "\n";
    auto item = EasyWebViewMgr::GetInstance().GetItemBrowserById(browser->GetIdentifier());
    if (!item)
        return;

    HWND hWnd = item->GetHWND();
    auto uifunmap = WebkitEcho::getUIFunMap();
    auto webcfunmap = WebkitEcho::getFunMap();

    if (uifunmap)
    {
        if (uifunmap->nativeFrameComplate)
        {
            uifunmap->nativeFrameComplate(hWnd, frame->GetURL().ToWString().c_str(), frame->GetName().ToWString().c_str());
        }

        if (uifunmap->nativeComplate && frame->IsMain())
        {
            uifunmap->nativeComplate(hWnd);
        }

    }

    if (webcfunmap && webcfunmap->webkitEndLoad && frame->IsMain())
    {
        webcfunmap->webkitEndLoad(browser->GetIdentifier());
    }
}

void EasyClientHandler::OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, ErrorCode errorCode, const CefString& errorText, const CefString& failedUrl)
{
    CEF_REQUIRE_UI_THREAD();

    // Don't display an error for downloaded files.
    if (errorCode == ERR_ABORTED)
        return;

    if (m_bIsUIControl)
    {
        if (WebkitEcho::getUIFunMap() && WebkitEcho::getUIFunMap()->loadError)
        {
            auto item = EasyWebViewMgr::GetInstance().GetItemBrowserById(browser->GetIdentifier());
            if (item)
            {
                HWND hWnd = item->GetHWND();
                WebkitEcho::getUIFunMap()->loadError(hWnd, errorCode, frame->GetName().ToWString().c_str(), failedUrl.ToWString().c_str());
                return;
            }
        }
    }
    else
    {
        if (WebkitEcho::getFunMap() && WebkitEcho::getFunMap()->webkitLoadError)
        {
            WebkitEcho::getFunMap()->webkitLoadError(browser->GetIdentifier(), errorCode, frame->IsMain(),
                frame->GetName().ToWString().c_str(), failedUrl.ToWString().c_str());
            return;
        }
    }

    // Display a load error message using a data: URI.

    webinfo::LoadErrorPage(frame, failedUrl, errorCode, errorText);
}

CefRefPtr<CefRenderHandler> EasyClientHandler::GetRenderHandler()
{
    if (m_bIsUIControl && m_bIsUITransparent)
    {
        if (m_browser)
            return dynamic_cast<WebViewTransparentUIControl*>(m_webuicontrol.get());
        else
            return this; //光标在输入框时关闭窗口在OnBeforeClose处理后仍然需要保证CefRenderHandler存在，否则CefRenderWidgetHostViewOSR::OnUpdateTextInputStateCalled会崩溃
    }

    return nullptr;
}


void EasyClientHandler::OnBeforeContextMenu(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefContextMenuParams> params, CefRefPtr<CefMenuModel> model)
{
    //LOG(INFO) << GetCurrentProcessId() << "] EasyClientHandler::OnBeforeContextMenu";

    if (m_bIsUIControl)
    {
        if (params->IsEditable())
        {
            auto item = EasyWebViewMgr::GetInstance().GetItemBrowserById(browser->GetIdentifier());
            if (!item)
                return;

            HWND hWnd = item->GetHWND();
            
            int x = params->GetXCoord();
            int y = params->GetYCoord();
                                 
            std::wstring strAttr;
            if (hWnd)
            {
                if (QueryNodeAttrib(item, x, y, "data-nc", strAttr))
                {
                    constexpr int size = 32;
                    auto menus = std::make_unique<wrapQweb::WRAP_CEF_MENU_COMMAND[]>(size);
                    memset(menus.get(), 0, sizeof(wrapQweb::WRAP_CEF_MENU_COMMAND) * size);
                    const auto fun = WebkitEcho::getUIFunMap();
                    if (fun)
                    {
                        fun->insertMenu(hWnd, strAttr.c_str(), menus.get());
                        for (int i = 0; i < size; ++i)
                        {
                            if (menus.get()[i].command > 0)
                            {
                                if (menus.get()[i].top)
                                {
                                    model->InsertItemAt(0, menus.get()[i].command, menus.get()[i].szTxt);
                                }
                                else {
                                    model->AddItem(menus.get()[i].command, menus.get()[i].szTxt);
                                }
                                model->SetEnabled(menus.get()[i].command, menus.get()[i].bEnable);
                            }
                            else {
                                break;
                            }
                        }
                    }
                }

            }

        }
        else
        {
            if (!g_BrowserGlobalVar.Debug)
            {
                model->Clear();
            }
        }
        
    }

    if (g_BrowserGlobalVar.Debug)
    {
        model->AddItem(CLIENT_ID_SHOW_DEVTOOLS, "&Show DevTools");
        model->AddItem(CLIENT_ID_CLOSE_DEVTOOLS, "Close DevTools");
        model->AddSeparator();
        model->AddItem(CLIENT_ID_INSPECT_ELEMENT, GetLabel(IDS_CONTENT_CONTEXT_INSPECTELEMENT));
        model->AddSeparator();

        model->AddItem(MENU_ID_RELOAD, GetLabel(IDS_CONTENT_CONTEXT_RELOAD));
        model->AddItem(MENU_ID_RELOAD_NOCACHE, "Reload NoCache");
        
    }


 
}


bool EasyClientHandler::OnContextMenuCommand(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefContextMenuParams> params, int command_id, EventFlags event_flags)
{
    switch (command_id)
    {
    case CLIENT_ID_SHOW_DEVTOOLS:
    case CLIENT_ID_INSPECT_ELEMENT:
        {
            if (!browser->GetHost()->HasDevTools())
            {
                CefWindowInfo windowInfo;
                windowInfo.SetAsPopup(nullptr, L"dev");
                browser->GetHost()->ShowDevTools(windowInfo, nullptr, CefBrowserSettings(), command_id == CLIENT_ID_INSPECT_ELEMENT ? CefPoint(params->GetXCoord(), params->GetYCoord()) : CefPoint());
            }
            
        }
        return true;
    case CLIENT_ID_CLOSE_DEVTOOLS:
        if (browser->GetHost()->HasDevTools())
            browser->GetHost()->CloseDevTools();
        return true;

    default:
        {
            if (m_bIsUIControl)
            {
                bool bExec = false;
                auto item = EasyWebViewMgr::GetInstance().GetItemBrowserById(browser->GetIdentifier());
                if (!item)
                    return false;

                HWND hWnd = item->GetHWND();
                if (WebkitEcho::getUIFunMap() && WebkitEcho::getUIFunMap()->doMenuCommand)
                {
                    bExec = WebkitEcho::getUIFunMap()->doMenuCommand(hWnd, command_id);
                }

                return bExec;
            }
        }
    }


    return false;
}

#if CEF_VERSION_MAJOR < 100
void EasyClientHandler::OnPluginCrashed(CefRefPtr<CefBrowser> browser, const CefString& plugin_path)
{
    if (!m_bIsUIControl && WebkitEcho::getFunMap() && WebkitEcho::getFunMap()->webkitPluginCrash)
    {
        WebkitEcho::getFunMap()->webkitPluginCrash(browser->GetIdentifier(), plugin_path.ToWString().c_str());
    }
}
#endif

void EasyClientHandler::OnRenderProcessTerminated(CefRefPtr<CefBrowser> browser, TerminationStatus status)
{
    EasyIPCServer::GetInstance().RemoveClient(browser->GetIdentifier());

 
    auto frame = browser->GetMainFrame();

    auto str = R"_raw(Render Process was Terminated... <a href="javascript:window.close();">Close</a>)_raw";

    webinfo::LoadErrorPage(frame, "", (cef_errorcode_t)100001, str);

    //LOG(INFO) << GetCurrentProcessId() << "] EasyClientHandler::OnRenderProcessTerminated:(" << browser << ") res: " << status;
}

bool EasyClientHandler::OnBeforeBrowse(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, bool user_gesture, bool is_redirect)
{

    if (frame->IsMain())
    {
        auto new_url = request->GetURL().ToWString();

        if ((request->GetTransitionType() & TT_FORWARD_BACK_FLAG) == TT_FORWARD_BACK_FLAG)
        {
            if (new_url.substr(0, _countof(EASYCEFPROTOCOLW)).find(EASYCEFPROTOCOLW) == 0)
            {
                browser->GoBack();
                return true;
            }
        }

        if (!m_bIsUIControl && WebkitEcho::getFunMap() && WebkitEcho::getFunMap()->webkitBeginLoad)
        {
            bool cancel = false;
            WebkitEcho::getFunMap()->webkitBeginLoad(browser->GetIdentifier(), new_url.c_str(), &cancel);
            return cancel;
        }

    }

    return false;
}

bool EasyClientHandler::OnOpenURLFromTab(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString& target_url, CefRequestHandler::WindowOpenDisposition target_disposition, bool user_gesture)
{
    if (m_bIsUIControl)
    {
        //阻止拖拽文件导致页面跳转
        if (user_gesture && target_disposition == WOD_NEW_FOREGROUND_TAB)
        {
            return true;
        }
    }

    return false;
}

bool EasyClientHandler::OnCertificateError(CefRefPtr<CefBrowser> browser, cef_errorcode_t cert_error, const CefString& request_url, CefRefPtr<CefSSLInfo> ssl_info, CefRefPtr<CEF_REQUEST_CALLBACK> callback)
{
    bool bAllowUnsecure = false;
    auto domain = DomainPackInfo::GetFormatedDomain(request_url.ToWString().c_str());
    if (!domain.empty())
    {
        bAllowUnsecure = s_listAllowUnsecure.find(domain) != s_listAllowUnsecure.end();
    }

    if (bAllowUnsecure)
    {
        callback->Continue(
#if CEF_VERSION_MAJOR <= 95
            true
#endif
        );
        return true;
    }
    else
    {
        CefRefPtr<CefX509Certificate> cert = ssl_info->GetX509Certificate();
        if (cert.get()) {

            if (!browser->CanGoBack())
            {
                browser->GetMainFrame()->LoadURL("about:blank");
            }

            // Load the error page.
            webinfo::LoadErrorPage(browser->GetMainFrame(), request_url, cert_error,
                webinfo::GetCertificateInformation(request_url, cert, ssl_info->GetCertStatus()));
        }
    }

    

    return false;  // Cancel the request.


}

bool EasyClientHandler::OnDragEnter(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDragData> dragData, CefDragHandler::DragOperationsMask mask)
{
    if (m_bIsUIControl)
    {
        if ((mask & DRAG_OPERATION_LINK) && !dragData->IsFragment())
        {
            if (dragData->IsLink())
            {
                //要放过链接，否则页面内自身的拖拽会被受限
                return false;
            }
            if (dragData->IsFile())
            {
                if (m_webuicontrol && m_webuicontrol->IsAllowDragFiles())
                {
                    return false;
                }
            }

            //blocks dragging of URLs and files
            return true;
        }

    }

    return false;
}

void EasyClientHandler::OnDraggableRegionsChanged(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const std::vector<CefDraggableRegion>& regions)
{
    CEF_REQUIRE_UI_THREAD();

    if (m_bIsUIControl && frame->IsMain())
    {
        CefRefPtr<WebViewUIControl> item = dynamic_cast<WebViewUIControl*>(EasyWebViewMgr::GetInstance().GetItemBrowserById(browser->GetIdentifier()).get());
        if (!item)
            return;

        item->SetDraggableRegion(regions);
    }

}

void EasyClientHandler::OnBeforeDownload(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDownloadItem> download_item, const CefString& suggested_name, CefRefPtr<CefBeforeDownloadCallback> callback)
{
    if (WebkitEcho::getFunMap() && WebkitEcho::getFunMap()->webkitDownFileUrl) {
        WebkitEcho::getFunMap()->webkitDownFileUrl(browser->GetIdentifier(), download_item->GetURL().ToWString().c_str(), suggested_name.ToWString().c_str());
    }
    else
        // Continue the download and show the "Save As" dialog.
        callback->Continue([](const std::wstring& file_name) {
            WCHAR szFolderPath[MAX_PATH];
            std::wstring path;

            // Save the file in the user's "My Documents" folder.
            if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PERSONAL | CSIDL_FLAG_CREATE, NULL,
                0, szFolderPath))) {
                path = szFolderPath;
                path += L"\\" + file_name;
            }

            return path;
            
            }(suggested_name.ToWString()), true);
}

void EasyClientHandler::SetUIWindowInfo(CefRefPtr<WebViewUIControl> webui, bool bTransparent)
{
    m_bIsUIControl = true;
    m_webuicontrol = webui;
    m_bIsUITransparent = bTransparent;
}

CefRefPtr<CefResourceRequestHandler> EasyClientHandler::GetResourceRequestHandler(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, bool is_navigation, bool is_download, const CefString& request_initiator, bool& disable_default_handling)
{
    if (m_bIsUIControl)
    {
        return this;
    }
    return nullptr;
}

CefResourceRequestHandler::ReturnValue EasyClientHandler::OnBeforeResourceLoad(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, CefRefPtr<CEF_REQUEST_CALLBACK> callback)
{
    auto strUrl = request->GetURL().ToString();

    //LOG(INFO) << GetCurrentProcessId() << "] OnBeforeResourceLoad id:" << request->GetIdentifier() << " url:" << strUrl << "\n";

    std::vector<RuleID> RequestListIds;
    if (EasyReqRespModifyMgr::GetInstance().CheckMatchUrl(strUrl, &RequestListIds, nullptr, 1))
    {
        EasyReqRespModifyMgr::GetInstance().ModifyRequest(RequestListIds, request);
        return RV_CONTINUE;
    }

    if (m_bIsUIControl)
    {
        return g_resource_manager->OnBeforeResourceLoad(browser, frame, request, callback);
    }

    return RV_CONTINUE;
 
}

CefRefPtr<CefResourceHandler> EasyClientHandler::GetResourceHandler(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request)
{
    auto strUrl = request->GetURL().ToString();

    //LOG(INFO) << GetCurrentProcessId() << "] GetResourceHandler id:" << request->GetIdentifier() << " url:" << strUrl << "\n";

    std::vector<RuleID> ResponseListIds;
    if (EasyReqRespModifyMgr::GetInstance().CheckMatchUrl(strUrl, nullptr, &ResponseListIds, 2))
    {
        return new EasyReqRespHandler(browser, std::move(ResponseListIds));
    }

    if (m_bIsUIControl)
    {
        return g_resource_manager->GetResourceHandler(browser, frame, request);
    }

    return nullptr;
}

