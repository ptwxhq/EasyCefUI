#include "pch.h"
#include "EasyClientHandler.h"
#include <include\wrapper\cef_helpers.h>
#include <include\cef_parser.h>
#include <include\cef_app.h>
#include <ShlObj.h>

#include "EasyIPC.h"
#include "LegacyImplement.h"
#include "EasyLayeredWindow.h"
#include "WebViewControl.h"
#include "EasyWebViewMgr.h"
#include "EasyBrowserWorks.h"
#include "include/cef_resource_bundle.h"
#include "include/cef_pack_strings.h"

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
        ipcSvr./*Legacy*/SendData(hipcli, send, ret);

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
    base::string16 label =
        CefResourceBundle::GetGlobal()->GetLocalizedString(message_id);
    DCHECK(!label.empty());
    return label;
}


bool EasyClientHandler::OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefProcessId source_process, CefRefPtr<CefProcessMessage> message)
{
    const std::string& message_name = message->GetName();


    return EasyBrowserWorks::GetInstance().DoAsyncWork(message_name, browser, frame, message->GetArgumentList());
}

bool EasyClientHandler::OnBeforePopup(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString& target_url, const CefString& target_frame_name, CefLifeSpanHandler::WindowOpenDisposition target_disposition, bool user_gesture, const CefPopupFeatures& popupFeatures, CefWindowInfo& windowInfo, CefRefPtr<CefClient>& client, CefBrowserSettings& settings, CefRefPtr<CefDictionaryValue>& extra_info, bool* no_javascript_access)
{
    CEF_REQUIRE_UI_THREAD();

    //LOG(INFO) << GetCurrentProcessId() << "] EasyClientHandler::OnBeforePopup:" << target_url;

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
                windowInfo.SetAsChild(hAttchWnd, rect);

                //这边得使用相同的handler
                auto handle = EasyWebViewMgr::GetInstance().CreatePopWebViewControl(hAttchWnd, RECT(), target_url.ToWString().c_str(), this);
                if (handle)
                {
                    m_preCreatePopHandle.push_back(handle);
                    return false;
                }
    
            }

            //?
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
            auto handle = EasyWebViewMgr::GetInstance().CreatePopWebViewControl(nullptr, {0,0,640,480}, target_url.ToWString().c_str(), this);
            m_preCreatePopHandle.push_back(handle);

            return false;

        }

   

        return true;
        //extra_info->SetBool(ExtraKeyNameIsUIBrowser, true);
    }


    return false;
}

void EasyClientHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser)
{
    CEF_REQUIRE_UI_THREAD();

    ++m_BrowserCount;

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

bool EasyClientHandler::DoClose(CefRefPtr<CefBrowser> browser)
{
    CEF_REQUIRE_UI_THREAD();

    if (m_bIsUIControl)
    {
        if (!m_bIsUITransparent)
        {
            HWND hWnd = m_webuicontrol->GetHWND();
            HWND hBrs = browser->GetHost()->GetWindowHandle();
            ShowWindow(hBrs, SW_HIDE);
            SetParent(hBrs, nullptr);
            DestroyWindow(hWnd);
        }
    }

    // Allow the close. For windowed browsers this will result in the OS close
    // event being sent.
    return false;
}

void EasyClientHandler::OnBeforeClose(CefRefPtr<CefBrowser> browser)
{
    CEF_REQUIRE_UI_THREAD();

    //LOG(INFO) << GetCurrentProcessId() << "] EasyClientHandler::OnBeforeClose:(" << browser << ")  ";

    --m_BrowserCount;

    bool bIsPop = browser->IsPopup();

    if (bIsPop)
    {
        //// Remove from the list of existing browsers.
        //BrowserList::iterator bit = m_popbrowsers.begin();
        //for (; bit != m_popbrowsers.end(); ++bit) {
        //    if ((*bit)->IsSame(browser)) {
        //        m_popbrowsers.erase(bit);
        //        break;
        //    }
        //}
    }
    else
    {
        //webui目前不会有popup，直接这样吧
        if (m_bIsUIControl)
        {
            m_webuicontrol = nullptr;
        }
    }

    if (m_BrowserCount < 1)
    {
        EasyIPCServer::GetInstance().RemoveClient(browser->GetIdentifier());

        EasyWebViewMgr::GetInstance().RemoveWebViewByBrowserId(browser->GetIdentifier());
    }

    if (m_browser && m_browser->IsSame(browser))
    {
        //防止关闭时仍然有引用导致无法检查异常
        m_browser = nullptr;
    }

     //CefBrowser
    if (!EasyIPCServer::GetInstance().GetClientsCount()) {

        //保存下cookies
        auto request_context = CefRequestContext::GetGlobalContext();
        auto cookie_mgr = request_context->GetCookieManager(nullptr);
        if (cookie_mgr)
            cookie_mgr->FlushStore(nullptr);

        // All browser windows have closed. Quit the application message loop.
        //CefQuitMessageLoop();
        if (g_BrowserGlobalVar.funcCloseCallback)
        {
            static_cast<EASYCEF::CloseHandler>(g_BrowserGlobalVar.funcCloseCallback)();
        }
    }
    
}

void EasyClientHandler::OnAddressChange(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString& url)
{
    if (!m_bIsUIControl)
    {
        if (frame->IsMain() && WebkitEcho::getFunMap() && WebkitEcho::getFunMap()->webkitChangeUrl)
        {
            WebkitEcho::getFunMap()->webkitChangeUrl(browser->GetIdentifier(), url.ToWString().c_str());
        }
    }
}

void EasyClientHandler::OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title)
{
    if (m_bIsUIControl)
    {
        auto item = EasyWebViewMgr::GetInstance().GetItemBrowserById(browser->GetIdentifier());
        if (!item)
            return;

        HWND hWnd = item->GetHWND();

        SetWindowTextW(hWnd, title.c_str());
    }
    else
    {
        if (WebkitEcho::getFunMap() && WebkitEcho::getFunMap()->webkitChangeTitle)
        {
            WebkitEcho::getFunMap()->webkitChangeTitle(browser->GetIdentifier(), title.ToWString().c_str());
        }
    }
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

    // Display a load error message using a data: URI.
    std::stringstream ss;
    ss << "<html><title>Load Failed</title><body bgcolor=\"white\">"
        "<h2>Failed to load URL "
        << std::string(failedUrl) << " with error " << std::string(errorText)
        << " (" << errorCode << ").</h2></body></html>";

    frame->LoadURL(GetDataURI(ss.str(), "text/html"));
}

CefRefPtr<CefRenderHandler> EasyClientHandler::GetRenderHandler()
{
    if (m_bIsUIControl && m_bIsUITransparent)
    {
        return dynamic_cast<WebViewTransparentUIControl*>(m_webuicontrol.get());
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

void EasyClientHandler::OnPluginCrashed(CefRefPtr<CefBrowser> browser, const CefString& plugin_path)
{
}

void EasyClientHandler::OnRenderProcessTerminated(CefRefPtr<CefBrowser> browser, TerminationStatus status)
{
    EasyIPCServer::GetInstance().RemoveClient(browser->GetIdentifier());

 
    auto frame = browser->GetMainFrame();
    frame->LoadURL(GetDataURI(R"(
<html><head><title>Crash</title><style>
body{border-style:solid;
border-width:2px;
background-color:white;}
</style></head>
<body>
<h1>Render Process was Terminated...</h1>
</body></html>
)", "text/html"));

    //LOG(INFO) << GetCurrentProcessId() << "] EasyClientHandler::OnRenderProcessTerminated:(" << browser << ") res: " << status;
}

void EasyClientHandler::OnDocumentAvailableInMainFrame(CefRefPtr<CefBrowser> browser)
{
     //LOG(INFO) << GetCurrentProcessId() << "] EasyClientHandler::OnDocumentAvailableInMainFrame:(" << browser << ")";
}

bool EasyClientHandler::OnBeforeBrowse(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, bool user_gesture, bool is_redirect)
{
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

