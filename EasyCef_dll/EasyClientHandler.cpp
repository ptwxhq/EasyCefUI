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

    extra_info->SetInt(IpcBrowserServerKeyName, (int)EasyIPCServer::GetInstance().GetHandle());
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

            if (!bCancel && IsWindow(hAttchWnd))
            {
                RECT rect;
                GetClientRect(hAttchWnd, &rect);

                //这边得使用相同的handler
                auto handle = EasyWebViewMgr::GetInstance().CreatePopWebViewControl(hAttchWnd, rect, target_url.ToWString().c_str(), this);
                m_preCreatePopHandle.insert(std::make_pair(target_url.ToWString(), handle));
                return false;
            }

            //?
            if (WebkitEcho::getFunMap() && WebkitEcho::getFunMap()->webkitOpenNewUrl)
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
            m_preCreatePopHandle.insert(std::make_pair(target_url.ToWString(), handle));

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

    // Add to the list of existing browsers.

    wvhandle handler;

    if (browser->IsPopup())
    {
        auto url = browser->GetMainFrame()->GetURL();
        auto it = m_preCreatePopHandle.find(url.ToWString());
        handler = it->second;
        m_preCreatePopHandle.erase(it);
     //   m_popbrowsers.push_back(browser);

     //   return;
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

        if (m_bIsUIControl && m_webuicontrol)
             m_webuicontrol->m_pWindow->SetBrowser(browser);
    }


    auto browserhost = browser->GetHost();
    if (browserhost)
    {
        HWND hWnd = browserhost->GetWindowHandle();

        EasyWebViewMgr::GetInstance().AsyncSetIndexInfo(m_hManualCreateHandle, browser->GetIdentifier(), hWnd);


        //SetBrowser
       // CHECK(m_webcontrol);
        //透明UI窗口
        //if(m_webuicontrol)
        //    m_webuicontrol->SetBrowser(browser);


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
    // Closing the main window requires special handling. See the DoClose()
  // documentation in the CEF header for a detailed destription of this
  // process.
    //if (browser_list_.size() == 1) {
    //    // Set a flag to indicate that the window close should be allowed.

    //}

    // Allow the close. For windowed browsers this will result in the OS close
    // event being sent.
    return false;
}

void EasyClientHandler::OnBeforeClose(CefRefPtr<CefBrowser> browser)
{
    CEF_REQUIRE_UI_THREAD();

    //LOG(INFO) << GetCurrentProcessId() << "] EasyClientHandler::OnBeforeClose:(" << browser << ")  ";

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
        if(m_bIsUIControl)
            m_webuicontrol = nullptr;
    }

    EasyIPCServer::GetInstance().RemoveClient(browser->GetIdentifier());

    //这边如果是浏览器内部自己关透明UI窗口的话会有点问题，目前在EasyLayeredWindow析构前判断窗口存在解决，后续看还有么问题
    //目前这个调用在了对应窗口销毁之后，此时不再有问题了
   // if(!m_webuicontrol)
        EasyWebViewMgr::GetInstance().RemoveWebViewByBrowserId(browser->GetIdentifier());


    if (m_browser && m_browser->IsSame(browser))
    {
        //防止关闭时仍然有引用导致无法检查异常
        m_browser = nullptr;
    }


     //CefBrowser
    if (!EasyIPCServer::GetInstance().GetClientsCount()) {
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
        SetWindowTextW(browser->GetHost()->GetWindowHandle(), title.c_str());
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
    if (m_bIsUIControl && m_webuicontrol)
    {
        m_webuicontrol->m_pWindow->SetToolTip(text);
        return true;
    }

    return false;
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
    //LOG(INFO) << GetCurrentProcessId() << "] OnLoadStart name:" << frame->GetName() << "(((" << transition_type << "||" << frame->GetURL() << "\n";
}

void EasyClientHandler::OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode)
{
    HWND hWnd = browser->GetHost()->GetWindowHandle();
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
    if (m_bIsUIControl)
    {
        return this;
    }

    return nullptr;
}

bool EasyClientHandler::GetRootScreenRect(CefRefPtr<CefBrowser> browser, CefRect& rect)
{
    //RECT window_rect = { 0 };
    //HWND root_window = GetAncestor(browser->GetHost()->GetWindowHandle(), GA_ROOT);
    //if (::GetWindowRect(root_window, &window_rect)) {
    //    rect = CefRect(window_rect.left,
    //        window_rect.top,
    //        window_rect.right - window_rect.left,
    //        window_rect.bottom - window_rect.top);
    //    return true;
    //}

    return false;
}

void EasyClientHandler::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect)
{
    RECT rc = {};
    GetClientRect(browser->GetHost()->GetWindowHandle(), &rc);

    rect = { 0,0, rc.right,rc.bottom };
}

void EasyClientHandler::OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList& dirtyRects, const void* buffer, int width, int height)
{
    int a;
    a = 1;

    if (!m_webuicontrol)
        return;

    if (dirtyRects.size() > 1)
    {
        //目前好像没见过这种情况
        int a;
        a = 1;
    }

    //static bool bTestSkip = false;
    //if (bTestSkip)
    //{
    //    bTestSkip = false;
    //    return;
    //}

    //RECT rcNow = {};
    //m_webuicontrol->m_pWindow->GetClientRect(&rcNow);

    for (auto& it: dirtyRects)
    //{
    //    if (type == PET_POPUP)
    //    {
    //        OutputDebugStringA(std::format("aera p({}) :x:{},y:{},w:{},h:{}[x:{},y:{}]\n", dirtyRects.size(),
    //            m_webuicontrol->m_pWindow->popup_rect_.x + it.x,
    //            m_webuicontrol->m_pWindow->popup_rect_.y + it.y,
    //            it.width, it.height,
    //            it.x, it.y).c_str());


    //    }
    //    else
    //    {
  //          OutputDebugStringA(std::format("{} aera n({}) :x:{},y:{},w:{},h:{}\n", (int)browser->GetIdentifier(), dirtyRects.size(), it.x, it.y, it.width, it.height).c_str());
    //    }

 
    //}


    //m_webuicontrol->m_pWindow->Render();

    //return;


    if (type == PET_VIEW)
    {
        int old_width = m_webuicontrol->m_pWindow->view_width_;
        int old_height = m_webuicontrol->m_pWindow->view_height_;

        if (old_width != width || old_height != height ||
            (dirtyRects.size() == 1 &&
                dirtyRects[0] == CefRect(0, 0, width, height)))
        {
            m_webuicontrol->m_pWindow->SetBitmapData(buffer, width, height);
        }
        else
        {
            for (auto& it : dirtyRects)
            {
                m_webuicontrol->m_pWindow->SetBitmapData((BYTE*)buffer, it.x, it.y, it.width, it.height, true);
            }
        }

        if (!m_webuicontrol->m_pWindow->popup_rect_.IsEmpty())
        {
            browser->GetHost()->Invalidate(PET_POPUP);
            return;
        }


    }
    else //PET_POPUP
    {
        m_webuicontrol->m_pWindow->SetBitmapData((BYTE*)buffer, m_webuicontrol->m_pWindow->popup_rect_.x, m_webuicontrol->m_pWindow->popup_rect_.y, width, height, false);

    }

    m_webuicontrol->m_pWindow->Render();

}

bool EasyClientHandler::GetScreenPoint(CefRefPtr<CefBrowser> browser, int viewX, int viewY, int& screenX, int& screenY)
{
    HWND root_window = GetAncestor(browser->GetHost()->GetWindowHandle(), GA_ROOT);
    POINT pt = { viewX, viewY };
    ClientToScreen(root_window, &pt);
    screenX = pt.x;
    screenY = pt.y;

    return true;
}

bool EasyClientHandler::OnCursorChange(CefRefPtr<CefBrowser> browser, CefCursorHandle cursor, cef_cursor_type_t type, const CefCursorInfo& custom_cursor_info)
{
    CEF_REQUIRE_UI_THREAD();

    if (!m_bIsUIControl)
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

void EasyClientHandler::ClearPopupRects() {

    m_webuicontrol->m_pWindow->popup_rect_.Set(0, 0, 0, 0);
    m_webuicontrol->m_pWindow->original_popup_rect_.Set(0, 0, 0, 0);
}

CefRect EasyClientHandler::GetPopupRectInWebView(const CefRect& original_rect) {
    CefRect rc(original_rect);
    // if x or y are negative, move them to 0.
    if (rc.x < 0)
        rc.x = 0;
    if (rc.y < 0)
        rc.y = 0;
    // if popup goes outside the view, try to reposition origin
    if (rc.x + rc.width > m_webuicontrol->m_pWindow->view_width_)
        rc.x = m_webuicontrol->m_pWindow->view_width_ - rc.width;
    if (rc.y + rc.height > m_webuicontrol->m_pWindow->view_height_)
        rc.y = m_webuicontrol->m_pWindow->view_height_ - rc.height;
    // if x or y became negative, move them to 0 again.
    if (rc.x < 0)
        rc.x = 0;
    if (rc.y < 0)
        rc.y = 0;
    return rc;
}

void EasyClientHandler::OnPopupShow(CefRefPtr<CefBrowser> browser, bool show)
{
    if (!show) {
        // Clear the popup rectangle.
        ClearPopupRects();
        browser->GetHost()->Invalidate(PET_VIEW);
    }
}

void EasyClientHandler::OnPopupSize(CefRefPtr<CefBrowser> browser, const CefRect& rect)
{
    if (rect.width <= 0 || rect.height <= 0)
        return;
    m_webuicontrol->m_pWindow->original_popup_rect_ = rect;
    m_webuicontrol->m_pWindow->popup_rect_ = GetPopupRectInWebView(m_webuicontrol->m_pWindow->original_popup_rect_);
}

bool EasyClientHandler::StartDragging(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDragData> drag_data, CefRenderHandler::DragOperationsMask allowed_ops, int x, int y)
{
    return false;
}

void EasyClientHandler::UpdateDragCursor(CefRefPtr<CefBrowser> browser, DragOperation operation)
{
}

void EasyClientHandler::OnImeCompositionRangeChanged(CefRefPtr<CefBrowser> browser, const CefRange& selected_range, const RectList& character_bounds)
{
    if (m_bIsUIControl && m_webuicontrol)
        m_webuicontrol->m_pWindow->ImePosChange(selected_range, character_bounds);
}

void EasyClientHandler::OnTextSelectionChanged(CefRefPtr<CefBrowser> browser, const CefString& selected_text, const CefRange& selected_range)
{
}

void EasyClientHandler::OnBeforeContextMenu(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefContextMenuParams> params, CefRefPtr<CefMenuModel> model)
{
    //LOG(INFO) << GetCurrentProcessId() << "] EasyClientHandler::OnBeforeContextMenu";

    if (m_bIsUIControl)
    {
        if (params->IsEditable())
        {
            if (m_webuicontrol)
            {
                int x = params->GetXCoord();
                int y = params->GetYCoord();
                                 
                HWND hWnd = m_webuicontrol->GetHWND();
                auto item = EasyWebViewMgr::GetInstance().GetItemByHwnd(hWnd);
                std::wstring strAttr;
                if (item)
                {
                    if (QueryNodeAttrib(item, x, y, "data-nc", strAttr))
                    {
                        const int size = 64;
                        auto menus = std::make_unique<wrapQweb::WRAP_CEF_MENU_COMMAND>(size);
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
        

        //if (!frame->IsMain())
        //{
        //    model->AddItem(CLIENT_ID_ROLOAD_FRAME, "Reload Frame");
        //}

        
    }


 
}


bool EasyClientHandler::OnContextMenuCommand(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefContextMenuParams> params, int command_id, EventFlags event_flags)
{
    switch (command_id)
    {
    case CLIENT_ID_SHOW_DEVTOOLS:
        {
            if (!browser->GetHost()->HasDevTools())
            {
                CefWindowInfo windowInfo;
                windowInfo.SetAsPopup(nullptr, L"dev");
                browser->GetHost()->ShowDevTools(windowInfo, nullptr, CefBrowserSettings(), CefPoint());
            }
            
        }
        return true;
    case CLIENT_ID_CLOSE_DEVTOOLS:
        if (browser->GetHost()->HasDevTools())
            browser->GetHost()->CloseDevTools();
        return true;
    case CLIENT_ID_INSPECT_ELEMENT:
        {
            if (!browser->GetHost()->HasDevTools())
            {
                CefWindowInfo windowInfo;
                windowInfo.SetAsPopup(nullptr, L"dev");
                browser->GetHost()->ShowDevTools(windowInfo, nullptr, CefBrowserSettings(), CefPoint(params->GetXCoord(), params->GetYCoord()));
            }
          
        }
        return true;
    default:
        {
            if (m_bIsUIControl)
            {
                bool bExec = false;
                HWND hWnd = browser->GetHost()->GetWindowHandle();
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

void EasyClientHandler::OnDraggableRegionsChanged(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const std::vector<CefDraggableRegion>& regions)
{
    CEF_REQUIRE_UI_THREAD();

    if (m_bIsUIControl && m_webuicontrol && frame->IsMain())
    {
        m_webuicontrol->m_pWindow->SetDraggableRegion(regions);
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

