#include "pch.h"
#include "WebViewControl.h"
#include "EasyIPC.h"
#include "include/cef_request_context_handler.h"
#include "EasyClientHandler.h"
#include "include/wrapper/cef_closure_task.h"

bool WebViewControl::SetBrowser(CefRefPtr<CefBrowser> browser)
{
    if (!m_browser && browser)
    {
        m_browser = browser;
        return true;
    }

    return false;
}

bool WebViewControl::LoadUrl(const CefString& url, bool cleanload)
{
    if (m_browser && m_browser->GetMainFrame())
    {
        auto request = CefRequest::Create();
        if (!cleanload)
        {
            request->SetFlags(request->GetFlags() | UR_FLAG_SKIP_CACHE);
        }

        request->SetURL(url);

        m_browser->GetMainFrame()->LoadRequest(request);
        return true;
    }

    return false;
}

void WebViewControl::GoBack()
{
    if (m_browser)
    {
        m_browser->GoBack();
    }
}

void WebViewControl::GoForward()
{
    if (m_browser)
    {
        m_browser->GoForward();
    }
}

void WebViewControl::Reload()
{
    if (m_browser)
    {
        m_browser->Reload();
    }
}

void WebViewControl::ReloadIgnoreCache()
{
    if (m_browser)
    {
        m_browser->ReloadIgnoreCache();
    }
}

void WebViewControl::StopLoad()
{
    if (m_browser)
    {
        m_browser->StopLoad();
    }
}

bool WebViewControl::IsMuteAudio()
{
    if (m_browser && m_browser->GetHost())
    {
        return m_browser->GetHost()->IsAudioMuted();
    }
    return false;
}

void WebViewControl::MuteAudio(bool mute)
{
    if (m_browser && m_browser->GetHost())
    {
        m_browser->GetHost()->SetAudioMuted(mute);
    }
}

double WebViewControl::GetZoomLevel()
{
    if (m_browser && m_browser->GetHost())
    {
        double d = m_browser->GetHost()->GetZoomLevel();

        if (d > 0)
            return d;
    }
    return 1.0;
}

void WebViewControl::SetZoomLevel(double zoomLevel)
{
    if (m_browser && m_browser->GetHost())
    {
        m_browser->GetHost()->SetZoomLevel(zoomLevel);
    }
}

bool WebViewControl::InitBrowser(wvhandle hWebview, HWND hParent, const RECT& rc, const CefString& url, const CefString& cookie, const WebViewExtraAttr* pExt, bool Sync)
{
    //已存在，跳过
    if (m_itemHandle)
        return false;


    std::shared_ptr<BrowserInitParams> pParams = std::make_shared<BrowserInitParams>();
    pParams->bSyncCreate = Sync;
    pParams->cookie = cookie;
    pParams->hParent = hParent;
    pParams->hWebview = hWebview;
    pParams->pExt = pExt;    //TODO 注意如果使用同步方式这里得额外处理下
    pParams->rc = rc;
    pParams->url = url;

    if (Sync && !CefCurrentlyOn(TID_UI))
    {
        // Execute on the UI thread.
        bool bPostSucc = CefPostTask(TID_UI, base::Bind(&WebViewControl::InitBrowserImpl, this, pParams));

        if(bPostSucc)
            pParams->signal.get_future().wait();
    }
    else
    {
        InitBrowserImpl(pParams);
    }

    return pParams->bRet;
}

HWND WebViewControl::GetHWND()
{
    if (m_browser && m_browser->GetHost())
    {
        return m_browser->GetHost()->GetWindowHandle();
    }

    return nullptr;
}

int WebViewControl::GetBrowserId()
{
    if (m_browser)
    {
        return m_browser->GetIdentifier();
    }

    return -1;
}

void WebViewControl::CloseBrowser()
{
    if (m_browser)
    {
        m_browser->GetHost()->CloseBrowser(true);
    }
}


void WebViewBrowserControl::InitBrowserImpl(std::shared_ptr<BrowserInitParams> pParams)
{
    m_itemHandle = pParams->hWebview;

    CefWindowInfo window_info;


    if (IsWindow(pParams->hParent))
    {

        RECT rcChild = {};

        if ((pParams->rc.left || pParams->rc.right || pParams->rc.top || pParams->rc.bottom) == 0)
        {
            GetClientRect(pParams->hParent, &rcChild);

            //附着父窗口并跟随大小
       //注意子类化跨线程问题，需要与父窗口创建在同一线程

            struct SubProcContext
            {
                WebViewBrowserControl* pThis;
                SUBCLASSPROC proc;
            };

            static auto SubWndProc = [](HWND hWnd, UINT uMsg, WPARAM wParam,
                LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)->LRESULT {
                    auto pContext = (SubProcContext*)dwRefData;

                    switch (uMsg)
                    {
                    case WM_SIZE:
                        {
                            UINT width = LOWORD(lParam);
                            UINT height = HIWORD(lParam);
                            SetWindowPos(pContext->pThis->GetHWND(), nullptr, 0, 0, width, height, SWP_NOZORDER | SWP_NOMOVE | SWP_ASYNCWINDOWPOS);
                        }
                        break;
                    case WM_DESTROY:
                        RemoveWindowSubclass(hWnd, pContext->proc, uIdSubclass);
                        delete pContext;
                    }

                    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
            };

            VERIFY(SetWindowSubclass(pParams->hParent, SubWndProc, m_itemHandle, (DWORD_PTR)new SubProcContext{ this, SubWndProc }));

        }
        else
        {
            rcChild = pParams->rc;
        }

        window_info.SetAsChild(pParams->hParent, rcChild);

    }
    else
    {
        //是否要做支持？旧接口根据句柄来查找的，这样会有问题
        window_info.SetAsPopup(nullptr, "EasyCef");
        window_info.x = pParams->rc.left;
        window_info.y = pParams->rc.top;
        window_info.width = pParams->rc.right - pParams->rc.left;
        window_info.height = pParams->rc.bottom - pParams->rc.top;
        window_info.style = WS_POPUP | WS_VISIBLE;
    }

    CefBrowserSettings browser_settings;
    CefRefPtr<CefRequestContext> request_context;

 //   browser_settings.javascript_close_windows = STATE_ENABLED;
    WCHAR strFonts[64] = {};
    GetPrivateProfileStringW(L"Settings", L"Fonts", L"", strFonts, _countof(strFonts), g_BrowserGlobalVar.BrowserSettingsPath.c_str());
    if (strFonts[0])
    {
        CefString(&browser_settings.standard_font_family).FromWString(strFonts);
        CefString(&browser_settings.serif_font_family).FromWString(strFonts);
        CefString(&browser_settings.sans_serif_font_family).FromWString(strFonts);
    }


    if (pParams->clientHandler)
    {
        pParams->bRet = true;
    }
    else
    {

        if (pParams->cookie.length())
        {
            CefRequestContextSettings req_settings;
            CefString(&req_settings.cache_path) = pParams->cookie;
            request_context = CefRequestContext::CreateContext(req_settings, nullptr);

            SetRequestDefaultSettings(request_context);
        }
        else
        {
            request_context = CefRequestContext::GetGlobalContext();
        }

        CefRefPtr<EasyClientHandler> clientHandler = new EasyClientHandler;
        m_clientHandler = clientHandler;

        clientHandler->SetManualHandle(m_itemHandle);

        //   m_clientHandler->m_webcontrol = this;

        auto extra_info = CefDictionaryValue::Create();

        const auto tmpVal = EasyIPCServer::GetInstance().GetHandle();
        auto valKeyName = CefBinaryValue::Create(&tmpVal, sizeof(tmpVal));
        extra_info->SetBinary(IpcBrowserServerKeyName, valKeyName);
        extra_info->SetBool(ExtraKeyNameIsUIBrowser, IsUIControl());
        //extra_info

        if (pParams->bSyncCreate)
        {
            //如果要用自己创建窗口再附着上去的方式也得要求在主UI线程调用，那干脆直接使用同步方式创建Browser好了，省事
            m_browser = CefBrowserHost::CreateBrowserSync(window_info, m_clientHandler, pParams->url, browser_settings, extra_info, request_context);

            pParams->bRet = m_browser;

            pParams->signal.set_value();
        }
        else
        {
            pParams->bRet = CefBrowserHost::CreateBrowser(window_info, m_clientHandler, pParams->url, browser_settings, extra_info, request_context);
        }
    }


}

bool WebViewBrowserControl::CreatePopup(wvhandle hWebview, HWND hParent, const RECT& rc, const CefString& url, CefRefPtr<CefClient> clientHandler)
{
    std::shared_ptr<BrowserInitParams> pParams = std::make_shared<BrowserInitParams>();
    pParams->bSyncCreate = false;
    pParams->hParent = hParent;
    pParams->hWebview = hWebview;
    pParams->rc = rc;
    pParams->url = url;
    pParams->clientHandler = clientHandler;

    InitBrowserImpl(pParams);

    return pParams->bRet;
}


void WebViewUIControl::SetDraggableRegion(const std::vector<CefDraggableRegion>& regions)
{
    GetWindowPtr()->SetDraggableRegion(regions);

    if (!IsTransparentUI())
    {
        GetWindowPtr()->SubclassChildHitTest(true);
    }
}

bool WebViewUIControl::SetBrowser(CefRefPtr<CefBrowser> browser)
{
    GetWindowPtr()->SetBrowser(browser);
    return __super::SetBrowser(browser);
}


HWND WebViewUIControl::GetHWND()
{
    if (GetWindowPtr())
    {
        return GetWindowPtr()->GetSafeHwnd();
    }

    return WebViewControl::GetHWND();
}

void WebViewUIControl::SetEdgeNcAera(EasyUIWindowBase::HT_INFO ht, const std::vector<RECT>& vecRc)
{
    GetWindowPtr()->SetEdgeNcAera(ht, vecRc);

    if (!IsTransparentUI())
    {
        GetWindowPtr()->SubclassChildHitTest(true);
    }

}

void WebViewUIControl::SetAlpha(BYTE alpha)
{
    if (IsTransparentUI())
    {
        auto pWnd = static_cast<EasyLayeredWindow*>(GetWindowPtr());
        pWnd->SetAlpha(alpha, true);
    }
    else
    {
        //TODO 调整窗口
        auto pWnd = static_cast<EasyOpaqueWindow*>(GetWindowPtr());
        if (!(pWnd->GetExStyle() & WS_EX_LAYERED))
        {
            pWnd->ModifyStyleEx(0, WS_EX_LAYERED, SWP_FRAMECHANGED);
            SetLayeredWindowAttributes(*pWnd, 0, alpha, LWA_ALPHA);
        }
    }
   
}




void WebViewOpaqueUIControl::InitBrowserImpl(std::shared_ptr<BrowserInitParams> pParams)
{
    m_itemHandle = pParams->hWebview;

    CefBrowserSettings browser_settings;
    CefRefPtr<CefRequestContext> request_context;

    browser_settings.javascript_close_windows = STATE_ENABLED;
    browser_settings.plugins = STATE_ENABLED;
    browser_settings.universal_access_from_file_urls = STATE_ENABLED;


    if (pParams->cookie.length())
    {
        CefRequestContextSettings req_settings;
        CefString(&req_settings.cache_path) = pParams->cookie;
        request_context = CefRequestContext::CreateContext(req_settings, nullptr);

        SetRequestDefaultSettings(request_context);
    }
    else
    {
        request_context = CefRequestContext::GetGlobalContext();
    }


    ASSERT(CefCurrentlyOn(TID_UI));
    CefRefPtr<EasyClientHandler> clientHandler = new EasyClientHandler;
    m_clientHandler = clientHandler;

    clientHandler->SetManualHandle(m_itemHandle);

    clientHandler->SetUIWindowInfo(this, false);

    auto extra_info = CefDictionaryValue::Create();

    const auto tmpVal = EasyIPCServer::GetInstance().GetHandle();
    auto valKeyName = CefBinaryValue::Create(&tmpVal, sizeof(tmpVal));
    extra_info->SetBinary(IpcBrowserServerKeyName, valKeyName);

    extra_info->SetBool(ExtraKeyNameIsUIBrowser, IsUIControl());


    m_pWindow = std::make_unique<EasyOpaqueWindow>(/*m_itemHandle*/);

    DWORD dwStyle = /*WS_VISIBLE |*/ WS_SYSMENU | WS_POPUP;
    DWORD dwExStyle = 0;

    int nCmdShow = SW_NORMAL;

    if (pParams->pExt)
    {
        //创建窗口时最小化
        constexpr auto WIDGET_MIN_SIZE = 1 << 1;
        //创建窗口时最大化
        constexpr auto WIDGET_MAX_SIZE = 1 << 2;
        //允许拖拽
        constexpr auto ENABLE_DRAG_DROP = 1;

        if ((pParams->pExt->windowinitstatus & 0x000f) == WIDGET_MIN_SIZE)
        {
            nCmdShow = SW_SHOWMINIMIZED;
        }
        else if ((pParams->pExt->windowinitstatus & 0x000f) == WIDGET_MAX_SIZE)
        {
            nCmdShow = SW_SHOWMAXIMIZED;
        }

        //以便可以实现aero snap. 放外面了
        //dwStyle |= WS_MAXIMIZEBOX  | WS_THICKFRAME;
        dwStyle |= pParams->pExt->dwAddStyle;
        dwExStyle |= pParams->pExt->dwAddExStyle;

        if (!pParams->pExt->taskbar)
        {
            dwStyle &= ~(WS_CAPTION | WS_SYSMENU);
            dwStyle |= WS_POPUP;

            dwExStyle &= ~WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
            dwExStyle |= WS_EX_TOOLWINDOW;
        }


    }

    VERIFY(m_pWindow->Create(pParams->hParent, (LPRECT)&pParams->rc, L"EasyUI Loading...", dwStyle, dwExStyle)); // parent

    //win11下自动圆角了，恢复下
    m_pWindow->SetWindowRgn(NULL);


    RECT rcCef = { 0, 0, pParams->rc.right - pParams->rc.left, pParams->rc.bottom - pParams->rc.top };

    CefWindowInfo window_info;
    window_info.SetAsChild(*m_pWindow, rcCef);

    if (pParams->bSyncCreate)
    {
        //如果要用自己创建窗口再附着上去的方式也得要求在主UI线程调用，那干脆直接使用同步方式创建Browser好了，省事
        m_browser = CefBrowserHost::CreateBrowserSync(window_info, m_clientHandler, pParams->url, browser_settings, extra_info, request_context);

        m_pWindow->SetBrowser(m_browser);

        pParams->bRet = m_browser;

        pParams->signal.set_value();
    }
    else
    {
        pParams->bRet = CefBrowserHost::CreateBrowser(window_info, m_clientHandler, pParams->url, browser_settings, extra_info, request_context);
    }

    m_pWindow->ShowWindow(nCmdShow);

}



void WebViewTransparentUIControl::InitBrowserImpl(std::shared_ptr<BrowserInitParams> pParams)
{
    m_itemHandle = pParams->hWebview;

    CefBrowserSettings browser_settings;
    CefRefPtr<CefRequestContext> request_context;

    browser_settings.javascript_close_windows = STATE_ENABLED;
    browser_settings.plugins = STATE_ENABLED;
    browser_settings.universal_access_from_file_urls = STATE_ENABLED;
  //  browser_settings.file_access_from_file_urls = STATE_ENABLED;
  //  browser_settings.web_security = STATE_DISABLED;

    if (pParams->cookie.length())
    {
        CefRequestContextSettings req_settings;
        CefString(&req_settings.cache_path) = pParams->cookie;
        //TODO 这里CefRequestContextHandler目前用不上
        request_context = CefRequestContext::CreateContext(req_settings, nullptr);

        SetRequestDefaultSettings(request_context);
    }
    else
    {
        request_context = CefRequestContext::GetGlobalContext();
    }


    ASSERT(CefCurrentlyOn(TID_UI));
    CefRefPtr<EasyClientHandler> clientHandler = new EasyClientHandler;
    m_clientHandler = clientHandler;

    clientHandler->SetManualHandle(m_itemHandle);

    clientHandler->SetUIWindowInfo(this, true);

    auto extra_info = CefDictionaryValue::Create();


    const auto tmpVal = EasyIPCServer::GetInstance().GetHandle();
    auto valKeyName = CefBinaryValue::Create(&tmpVal, sizeof(tmpVal));
    extra_info->SetBinary(IpcBrowserServerKeyName, valKeyName);

    extra_info->SetBool(ExtraKeyNameIsUIBrowser, IsUIControl());


    m_pWindow = std::make_unique<EasyLayeredWindow>(/*m_itemHandle*/);

    DWORD dwStyle = /*WS_VISIBLE |*/ WS_SYSMENU | WS_POPUP;
    DWORD dwExStyle = 0;

    int nCmdShow = SW_NORMAL;

    if (pParams->pExt)
    {
        m_pWindow->SetAlpha(pParams->pExt->alpha, false);

    //创建窗口时最小化
constexpr auto WIDGET_MIN_SIZE = 1 << 1;
    //创建窗口时最大化
constexpr auto WIDGET_MAX_SIZE = 1 << 2;
    //允许拖拽
constexpr auto ENABLE_DRAG_DROP = 1;

        if ((pParams->pExt->windowinitstatus & 0x000f) == WIDGET_MIN_SIZE)
        {
            //dwStyle |= WS_MINIMIZE;
            nCmdShow = SW_SHOWMINIMIZED;
        }
        else if ((pParams->pExt->windowinitstatus & 0x000f) == WIDGET_MAX_SIZE)
        {
            //dwStyle |= WS_MAXIMIZE;
            nCmdShow = SW_SHOWMAXIMIZED;
        }



        //以便可以实现aero snap. 放外面了
        //dwStyle |= WS_MAXIMIZEBOX  | WS_THICKFRAME;
        dwStyle |= pParams->pExt->dwAddStyle;
        dwExStyle |= pParams->pExt->dwAddExStyle;

        //TODO 后续补充下
        //if ((pExt->windowinitstatus & 0x000f) == ENABLE_DRAG_DROP)
        //{
        //    //HRESULT register_res = RegisterDragDrop();
        // 
        //dwExStyle|= WS_EX_ACCEPTFILES
        //}

        //不知道为啥WS_EX_TOOLWINDOW设置了还是不行，只能使用一个隐藏窗口来处理了
        // 窗口首次显示前修改可以了 
        //if (!pParams->pExt->taskbar)
         //pParams->hParent = g_BrowserGlobalVar.hWndHidden;

    }

    //dwStyle &= ~WS_CAPTION;


    VERIFY(m_pWindow->Create(pParams->hParent, (LPRECT)&pParams->rc, L"EasyUI Loading...", dwStyle, dwExStyle)); // parent
   
    //win11下自动圆角了，恢复下
    m_pWindow->SetWindowRgn(NULL);


    if (pParams->pExt)
    {
        ////不能在创建时就加入属性，否则UpdateLayeredWindow会失败，why？
        if (!pParams->pExt->taskbar)
        {
            m_pWindow->ModifyStyle(WS_CAPTION| WS_SYSMENU, WS_POPUP);
            m_pWindow->ModifyStyleEx(WS_EX_APPWINDOW | WS_EX_WINDOWEDGE, WS_EX_TOOLWINDOW, SWP_FRAMECHANGED );
        }
    }

    CefWindowInfo window_info;
    window_info.SetAsWindowless(*m_pWindow);
    //window_info.style = 0;


    if (pParams->bSyncCreate)
    {
        //如果要用自己创建窗口再附着上去的方式也得要求在主UI线程调用，那干脆直接使用同步方式创建Browser好了，省事
        m_browser = CefBrowserHost::CreateBrowserSync(window_info, m_clientHandler, pParams->url, browser_settings, extra_info, request_context);

        m_pWindow->SetBrowser(m_browser);

        pParams->bRet = m_browser;

        pParams->signal.set_value();
    }
    else
    {
        pParams->bRet = CefBrowserHost::CreateBrowser(window_info, m_clientHandler, pParams->url, browser_settings, extra_info, request_context);
    }

    m_pWindow->ShowWindow(nCmdShow);
    m_pWindow->Render();
}



void WebViewTransparentUIControl::SetToolTip(const CefString& str)
{
    m_pWindow->SetToolTip(str);
}


void WebViewTransparentUIControl::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect)
{
    RECT rc = {};
    GetClientRect(browser->GetHost()->GetWindowHandle(), &rc);

    rect = { 0,0, rc.right,rc.bottom };
}

void WebViewTransparentUIControl::OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList& dirtyRects, const void* buffer, int width, int height)
{
    if (type == PET_VIEW)
    {
        const int old_width = m_pWindow->view_width_;
        const int old_height = m_pWindow->view_height_;

        if (old_width != width || old_height != height ||
            (dirtyRects.size() == 1 &&
                dirtyRects[0] == CefRect(0, 0, width, height)))
        {
            m_pWindow->SetBitmapData(buffer, width, height);
        }
        else
        {
            for (auto& it : dirtyRects)
            {
                m_pWindow->SetBitmapData(static_cast<const BYTE*>(buffer), it.x, it.y, it.width, it.height, true);
            }
        }

        if (!m_pWindow->popup_rect_.IsEmpty())
        {
            m_browser->GetHost()->Invalidate(PET_POPUP);
            return;
        }
    }
    else //PET_POPUP
    {
        m_pWindow->SetBitmapData(static_cast<const BYTE*>(buffer), m_pWindow->popup_rect_.x, m_pWindow->popup_rect_.y, width, height, false);

    }

    m_pWindow->Render();
}

bool WebViewTransparentUIControl::GetScreenPoint(CefRefPtr<CefBrowser> browser, int viewX, int viewY, int& screenX, int& screenY)
{
    HWND root_window = GetAncestor(browser->GetHost()->GetWindowHandle(), GA_ROOT);
    POINT pt = { viewX, viewY };
    ClientToScreen(root_window, &pt);
    screenX = pt.x;
    screenY = pt.y;

    return true;
}


void WebViewTransparentUIControl::ClearPopupRects() {

    m_pWindow->popup_rect_.Set(0, 0, 0, 0);
    m_pWindow->original_popup_rect_.Set(0, 0, 0, 0);
}

CefRect WebViewTransparentUIControl::GetPopupRectInWebView(const CefRect& original_rect) {
    CefRect rc(original_rect);
    // if x or y are negative, move them to 0.
    if (rc.x < 0)
        rc.x = 0;
    if (rc.y < 0)
        rc.y = 0;
    // if popup goes outside the view, try to reposition origin
    if (rc.x + rc.width > m_pWindow->view_width_)
        rc.x = m_pWindow->view_width_ - rc.width;
    if (rc.y + rc.height > m_pWindow->view_height_)
        rc.y = m_pWindow->view_height_ - rc.height;
    // if x or y became negative, move them to 0 again.
    if (rc.x < 0)
        rc.x = 0;
    if (rc.y < 0)
        rc.y = 0;
    return rc;
}

void WebViewTransparentUIControl::OnPopupShow(CefRefPtr<CefBrowser> browser, bool show)
{
    if (!show) {
        // Clear the popup rectangle.
        ClearPopupRects();
        browser->GetHost()->Invalidate(PET_VIEW);
    }
}

void WebViewTransparentUIControl::OnPopupSize(CefRefPtr<CefBrowser> browser, const CefRect& rect)
{
    if (rect.width <= 0 || rect.height <= 0)
        return;
    m_pWindow->original_popup_rect_ = rect;
    m_pWindow->popup_rect_ = GetPopupRectInWebView(m_pWindow->original_popup_rect_);
}

void WebViewTransparentUIControl::OnImeCompositionRangeChanged(CefRefPtr<CefBrowser> browser, const CefRange& selected_range, const RectList& character_bounds)
{
    m_pWindow->ImePosChange(selected_range, character_bounds);
}

