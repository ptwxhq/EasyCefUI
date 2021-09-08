#pragma once

#include <windowsx.h>

class EasyUIWindowBase
{
public:
    enum HT_INFO
    {
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
        E_HTBASE = HTLEFT
    };

    void SetBrowser(CefRefPtr<CefBrowser> browser) {
        m_browser = browser;
    }

    void SetDraggableRegion(const std::vector<CefDraggableRegion>& regions);

    void SetEdgeNcAera(HT_INFO ht, const std::vector<RECT>& vecRc);

    void SubclassChildHitTest(bool bSet);

    virtual HWND GetSafeHwnd() = 0;

    EasyUIWindowBase();
    virtual ~EasyUIWindowBase();

private:
    UINT Cls_OnNCHitTest(HWND hwnd, int x, int y);
    void Cls_OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpMinMaxInfo);
    LRESULT Cls_OnDpiChanged(HWND hwnd, WPARAM wParam, LPARAM lParam);


protected:

    BOOL ProcessWindowMessage(
         HWND hWnd,
         UINT uMsg,
         WPARAM wParam,
         LPARAM lParam,
         LRESULT& lResult);


    float device_scale_factor_ = 1.f;

    CefRefPtr<CefBrowser> m_browser;


private:
    HRGN m_EdgeRegions[E_END] = {};

};



class EasyOpaqueWindow : public EasyUIWindowBase, public CWindowImpl<EasyOpaqueWindow,
    CWindow, CWinTraits
    <WS_OVERLAPPED, 0>>
{
public:
    ~EasyOpaqueWindow();

    DECLARE_WND_CLASS_EX(g_BrowserGlobalVar.WebViewClassName.c_str(), 0, COLOR_WINDOW)

    BEGIN_MSG_MAP(0)
        CHAIN_MSG_MAP(EasyUIWindowBase)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
    END_MSG_MAP()

    HWND GetSafeHwnd() final {
        return m_hWnd;
    };

    LRESULT OnSize(UINT msg, WPARAM wParam, LPARAM lParam, BOOL& handle);

    void OnFinalMessage(HWND) override;

};

