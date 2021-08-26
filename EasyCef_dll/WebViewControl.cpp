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

    if (pParams->clientHandler)
    {
        
    }
    else
    {

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

        CefRefPtr<EasyClientHandler> clientHandler = new EasyClientHandler;
        m_clientHandler = clientHandler;

        clientHandler->SetManualHandle(m_itemHandle);

        //   m_clientHandler->m_webcontrol = this;

        auto extra_info = CefDictionaryValue::Create();


        extra_info->SetInt(IpcBrowserServerKeyName, (int)EasyIPCServer::GetInstance().GetHandle());
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

    InitBrowserImpl(pParams);

    return pParams->bRet;
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

    clientHandler->m_bIsUIControl = true;
    clientHandler->m_webuicontrol = this;

    auto extra_info = CefDictionaryValue::Create();


    extra_info->SetInt(IpcBrowserServerKeyName, (int)EasyIPCServer::GetInstance().GetHandle());

    extra_info->SetBool(ExtraKeyNameIsUIBrowser, IsUIControl());


    m_pWindow = std::make_unique<EasyLayeredWindow>(m_itemHandle);

    DWORD dwStyle = /*WS_VISIBLE |*/ WS_SYSMENU | WS_POPUP;
    DWORD dwExStyle = 0;

    int nCmdShow = SW_NORMAL;

    if (pParams->pExt)
    {
        m_pWindow->SetAlpha(pParams->pExt->alpha);

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

HWND WebViewTransparentUIControl::GetHWND()
{
    if (m_pWindow->m_hWnd)
    {
        return m_pWindow->m_hWnd;
    }

    return WebViewControl::GetHWND();
}


