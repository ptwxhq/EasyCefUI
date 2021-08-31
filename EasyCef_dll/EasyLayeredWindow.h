#pragma once

#include "EasyUIWindow.h"

#define VERIFYHR(x) VERIFY(SUCCEEDED(x))


namespace client {
    class OsrImeHandlerWin;
}



class LayeredWindowInfo {
    const POINT m_sourcePosition;
    POINT m_windowPosition;
    SIZE m_size;
    RECT m_dirty;
    BLENDFUNCTION m_blend;
    UPDATELAYEREDWINDOWINFO m_info;

public:

    LayeredWindowInfo(
        __in LONG width,
        __in LONG height);

    void Update(
        __in HWND window,
        __in HDC source);

    UINT GetWidth() const { return m_size.cx; }

    UINT GetHeight() const { return m_size.cy; }

    void SetWindowSize(SIZE size) {
        m_size = size;
    }

    void SetWindowPos(POINT pt) {
        m_windowPosition = pt;
    }

    void SetAlpha(BYTE alpha) {
        m_blend.SourceConstantAlpha = alpha;
    }

    void SetDirtyRect(const RECT* rc) {
        if (rc) {
            //���е���
            if (m_dirty.left < 0 || rc->left < m_dirty.left)
            {
                m_dirty.left = rc->left;
            }

            if (m_dirty.right < 0 || rc->right > m_dirty.right)
            {
                m_dirty.right = rc->right;
            }

            if (m_dirty.top < 0 || rc->top < m_dirty.top)
            {
                m_dirty.top = rc->top;
            }

            if (m_dirty.bottom < 0 || rc->bottom > m_dirty.bottom)
            {
                m_dirty.bottom = rc->bottom;
            }

            m_info.prcDirty = &m_dirty;
        }
        else {
            m_info.prcDirty = nullptr;
            m_dirty = { -1,-1,-1,-1 };
        }
    }
};



class GdiBitmap {
    const UINT m_width;
    const UINT m_height;
    const UINT m_stride;
    BYTE* m_bits;
    HBITMAP m_oldBitmap;

    HDC m_dc;
    HBITMAP m_bitmap;

public:

    GdiBitmap(__in UINT width,
        __in UINT height);

    ~GdiBitmap();

    LONG GetWidth() const {
        return m_width;
    }

    LONG GetHeight() const {
        return m_height;
    }

    UINT GetStride() const {
        return m_stride;
    }

    BYTE* GetBits() const {
        return m_bits;
    }

    HDC GetDC() const {
        return m_dc;
    }
};


class EasyLayeredWindow :
    public CWindowImpl<EasyLayeredWindow,
    CWindow, CWinTraits
    < WS_OVERLAPPED, WS_EX_LAYERED>>
    //<WS_POPUP | WS_SYSMENU, WS_EX_NOREDIRECTIONBITMAP>>   //���ԣ�win8+
    ,
    public EasyUIWindowBase
{
public:
    LayeredWindowInfo m_info;
private:


    std::unique_ptr<GdiBitmap> m_bitmap;

public:

    EasyLayeredWindow();
    ~EasyLayeredWindow();

    DECLARE_WND_CLASS_EX(g_BrowserGlobalVar.WebViewClassName.c_str(), 0, COLOR_WINDOW)


    BEGIN_MSG_MAP(EasyLayeredWindow)
        CHAIN_MSG_MAP(EasyUIWindowBase)
        MESSAGE_HANDLER(WM_LBUTTONDOWN, OnMouseEvent)
        MESSAGE_HANDLER(WM_RBUTTONDOWN, OnMouseEvent)
        MESSAGE_HANDLER(WM_MBUTTONDOWN, OnMouseEvent)
        MESSAGE_HANDLER(WM_LBUTTONUP, OnMouseEvent)
        MESSAGE_HANDLER(WM_RBUTTONUP, OnMouseEvent)
        MESSAGE_HANDLER(WM_MBUTTONUP, OnMouseEvent)
        MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseEvent)
        MESSAGE_HANDLER(WM_MOUSELEAVE, OnMouseEvent)
        MESSAGE_HANDLER(WM_MOUSEWHEEL, OnMouseEvent)

        MESSAGE_HANDLER(WM_SHOWWINDOW, OnShowWindow)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_MOVING, OnMove)
        MESSAGE_HANDLER(WM_MOVE, OnMove)

        MESSAGE_HANDLER(WM_SETFOCUS, OnFocus)
        MESSAGE_HANDLER(WM_KILLFOCUS, OnFocus)

        MESSAGE_HANDLER(WM_CAPTURECHANGED, OnCaptureLost)
        MESSAGE_HANDLER(WM_CANCELMODE, OnCaptureLost)

        MESSAGE_HANDLER(WM_SYSCHAR, OnKeyEvent)
        MESSAGE_HANDLER(WM_SYSKEYDOWN, OnKeyEvent)
        MESSAGE_HANDLER(WM_SYSKEYUP, OnKeyEvent)
        MESSAGE_HANDLER(WM_KEYDOWN, OnKeyEvent)
        MESSAGE_HANDLER(WM_KEYUP, OnKeyEvent)
        MESSAGE_HANDLER(WM_CHAR, OnKeyEvent)

        MESSAGE_HANDLER(WM_CREATE, OnCreate)

        //CEF windowless�����л�����Щbug��������ҳ����baidu�������޷�����
        MESSAGE_HANDLER(WM_IME_SETCONTEXT, OnIMESetContext)
        MESSAGE_HANDLER(WM_IME_STARTCOMPOSITION, OnIMEStartComposition)
        MESSAGE_HANDLER(WM_IME_COMPOSITION, OnIMEComposition)
        MESSAGE_HANDLER(WM_IME_ENDCOMPOSITION, OnIMECancelCompositionEvent)
   
      //  MESSAGE_HANDLER(WM_TIMER, OnTime)
        //������Ҫ����ģ�
        //WM_GETOBJECT
        //WM_ERASEBKGND
        //WM_TOUCH


//        MESSAGE_HANDLER(WM_PAINT, OnPaint)


        //������ҪUI��͸��ҲҪ�����濴��ô������
        // MESSAGE_HANDLER(WM_DWMCOMPOSITIONCHANGED, OnDwmCompositionChanged)

        
        //MESSAGE_HANDLER(WM_ACTIVATE, OnActivate)
        //MESSAGE_HANDLER(WM_NCACTIVATE, OnNcActivate)


       // MESSAGE_HANDLER(WM_CLOSE, OnClose)
    END_MSG_MAP()

    LRESULT OnMouseEvent(UINT msg, WPARAM wp, LPARAM lp, BOOL&);
    LRESULT OnShowWindow(UINT msg, WPARAM wp, LPARAM lp, BOOL& handle);
    LRESULT OnSize(UINT msg, WPARAM wp,LPARAM lp, BOOL &handle);
    LRESULT OnMove(UINT msg, WPARAM wp, LPARAM lp, BOOL& handle);

    LRESULT OnFocus(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnCaptureLost(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnKeyEvent(UINT msg, WPARAM wp, LPARAM lp, BOOL&);

    LRESULT OnCreate(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnIMESetContext(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnIMEStartComposition(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnIMEComposition(UINT, WPARAM, LPARAM lp, BOOL&);
    LRESULT OnIMECancelCompositionEvent(UINT, WPARAM, LPARAM, BOOL&);

    LRESULT OnDwmCompositionChanged(UINT, WPARAM wp, LPARAM lp, BOOL&);
    
    LRESULT OnNcActivate(UINT, WPARAM wp, LPARAM lp, BOOL& h);
    LRESULT OnActivate(UINT, WPARAM wp, LPARAM lp, BOOL& h);

    LRESULT OnSysCommand(UINT msg, WPARAM wp, LPARAM lp, BOOL& handle);

    LRESULT OnPaint(UINT, WPARAM, LPARAM, BOOL& handle);

    LRESULT OnIgnore(UINT, WPARAM, LPARAM, BOOL&) { return 0; }

    void SetBitmapData(const void* pData, int width, int height);
    void SetBitmapData(const BYTE* pData, int x, int y, int width, int height, bool SameSize);
    void Render();


    virtual void OnFinalMessage(HWND /*hWnd*/) override;

    //LRESULT OnTime(UINT, WPARAM wp, LPARAM, BOOL&);

    void SetAlpha(BYTE alpha, bool bRepaint);


    void SetToolTip(const CefString& str);

    void ImePosChange(const CefRange& selected_range, const CefRenderHandler::RectList& character_bounds);

    HWND GetSafeHwnd() final {
        return m_hWnd;
    };

    //��ʱ


    int view_width_ = 0;
    int view_height_ = 0;
    CefRect popup_rect_;
    CefRect original_popup_rect_;

private:

    bool IsOverPopupWidget(int x, int y) const;
    void ApplyPopupOffset(int& x, int& y) const;
    int GetPopupXOffset() const;
    int GetPopupYOffset() const;


    int view_width_old_ = 0;
    int view_height_old_ = 0;

    // Mouse state tracking.
    POINT last_mouse_pos_ = {};
    POINT current_mouse_pos_ = {};
    bool mouse_rotation_ = {};
    bool mouse_tracking_ = {};
    int last_click_x_ = {};
    int last_click_y_ = {};
    CefBrowserHost::MouseButtonType last_click_button_ = {};
    int last_click_count_ = {};
    double last_click_time_ = {};
    bool last_mouse_down_on_view_ = {};

    HWND m_hToolTip = nullptr;

    // Class that encapsulates IMM32 APIs and controls IMEs attached to a window.
    scoped_ptr<client::OsrImeHandlerWin> ime_handler_;

};