#pragma once


class EasyUIWindowBase : public CWindowImpl<EasyUIWindowBase, CWindow, CWinTraits <WS_OVERLAPPED, 0>>
{
public:

    enum HT_INFO
    {
        E_HTMAXBUTTON,  //9
        E_HTLEFT,        //10
        E_HTRIGHT,
        E_HTTOP,
        E_HTTOPLEFT,
        E_HTTOPRIGHT,
        E_HTBOTTOM,
        E_HTBOTTOMLEFT,
        E_HTBOTTOMRIGHT, //17
        E_CAPTION,
        E_END,
        E_HTBASE = HTMAXBUTTON
    };

    DECLARE_WND_CLASS_EX(g_BrowserGlobalVar.WebViewClassName.c_str(), 0, COLOR_WINDOW)


    void SetBrowser(CefRefPtr<CefBrowser> browser) {
        m_browser = browser;
    }

    void SetDraggableRegion(const std::vector<CefDraggableRegion>& regions);

    void SetEdgeNcAera(HT_INFO ht, const std::vector<RECT>& vecRc);

    bool IsNcAeraExist();

    void SubclassChildHitTest(bool bSet);

    virtual void SetAlpha(BYTE alpha, bool bRepaint) = 0;

    EasyUIWindowBase();
    virtual ~EasyUIWindowBase();

    void OnFinalMessage(HWND h) override;

private:
    UINT Cls_OnNCHitTest(HWND hwnd, int x, int y);
    void Cls_OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpMinMaxInfo);
    LRESULT Cls_OnDpiChanged(HWND hwnd, WPARAM wParam, LPARAM lParam);
    void Cls_OnMove(HWND hwnd, int x, int y);


protected:

    BOOL ProcessWindowMessage(
        HWND hWnd,
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam,
        LRESULT& lResult,
        DWORD dwMsgMapID = 0) override;


    float device_scale_factor_ = 1.f;

    CefRefPtr<CefBrowser> m_browser;

    using CWindow::m_hWnd;


private:
    bool m_bEdgeRegionExist[HT_INFO::E_END] = {};
    HRGN m_EdgeRegions[HT_INFO::E_END] = {};

};



class EasyOpaqueWindow : public EasyUIWindowBase
{
public:

    BEGIN_MSG_MAP(EasyOpaqueWindow)
        CHAIN_MSG_MAP(EasyUIWindowBase)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_NCACTIVATE, OnNcActivate)
        MESSAGE_HANDLER(WM_NCPAINT, OnNcPaint)

    END_MSG_MAP()

    LRESULT OnSize(UINT msg, WPARAM wParam, LPARAM lParam, BOOL& handle);
    LRESULT OnNcActivate(UINT msg, WPARAM wParam, LPARAM lParam, BOOL& handle);
    LRESULT OnNcPaint(UINT msg, WPARAM wParam, LPARAM lParam, BOOL& handle);


    void SetAlpha(BYTE alpha, bool) override;

};

