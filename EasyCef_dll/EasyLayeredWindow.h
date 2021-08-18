#pragma once


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
            //进行叠加
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
    //<WS_POPUP | WS_SYSMENU, WS_EX_NOREDIRECTIONBITMAP>>   //测试，win8+
{
public:
    LayeredWindowInfo m_info;
private:


    std::unique_ptr<GdiBitmap> m_bitmap;

public:

    EasyLayeredWindow(wvhandle handle);
    ~EasyLayeredWindow();

    DECLARE_WND_CLASS_EX(g_BrowserGlobalVar.WebViewClassName.c_str(), 0, COLOR_WINDOW)


    BEGIN_MSG_MAP(EasyLayeredWindow)
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

        //CEF windowless窗口中还是有些bug，比如框架页里面baidu搜索框无法输入
        MESSAGE_HANDLER(WM_IME_SETCONTEXT, OnIMESetContext)
        MESSAGE_HANDLER(WM_IME_STARTCOMPOSITION, OnIMEStartComposition)
        MESSAGE_HANDLER(WM_IME_COMPOSITION, OnIMEComposition)
        MESSAGE_HANDLER(WM_IME_ENDCOMPOSITION, OnIMECancelCompositionEvent)
   
      //  MESSAGE_HANDLER(WM_TIMER, OnTime)
        //可能需要补充的：
        //WM_GETOBJECT
        //WM_ERASEBKGND
        //WM_TOUCH


//        MESSAGE_HANDLER(WM_PAINT, OnPaint)


        //以下需要UI非透明也要，后面看怎么处理下
        MESSAGE_HANDLER(WM_DPICHANGED, OnDpiChanged)
        MESSAGE_HANDLER(WM_NCHITTEST, OnNcHitTest)
       // MESSAGE_HANDLER(WM_DWMCOMPOSITIONCHANGED, OnDwmCompositionChanged)
        MESSAGE_HANDLER(WM_GETMINMAXINFO, OnGetMinMaxInfo)
        

        
        //MESSAGE_HANDLER(WM_ACTIVATE, OnActivate)
        //MESSAGE_HANDLER(WM_NCACTIVATE, OnNcActivate)
        MESSAGE_HANDLER(WM_NCCALCSIZE, OnNcCalcSize)

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

    LRESULT OnDpiChanged(UINT, WPARAM wp, LPARAM lp, BOOL&);
    LRESULT OnDwmCompositionChanged(UINT, WPARAM wp, LPARAM lp, BOOL&);
    
    LRESULT OnNcHitTest(UINT , WPARAM wp, LPARAM lp, BOOL&);
    LRESULT OnNcCalcSize(UINT, WPARAM wp, LPARAM lp, BOOL& h);
    LRESULT OnNcActivate(UINT, WPARAM wp, LPARAM lp, BOOL& h);
    LRESULT OnActivate(UINT, WPARAM wp, LPARAM lp, BOOL& h);
    LRESULT OnGetMinMaxInfo(UINT, WPARAM, LPARAM lp, BOOL&);
    
    

    

    LRESULT OnClose(UINT msg, WPARAM wp, LPARAM lp, BOOL&);
    LRESULT OnSysCommand(UINT msg, WPARAM wp, LPARAM lp, BOOL& handle);

    LRESULT OnPaint(UINT, WPARAM, LPARAM, BOOL& handle);
    


    LRESULT OnIgnore(UINT, WPARAM, LPARAM, BOOL&) { return 0; }

    void SetBitmapData(const void* pData, int width, int height);
    void SetBitmapData(const BYTE* pData, int x, int y, int width, int height, bool SameSize);
    void Render();


    virtual void OnFinalMessage(HWND /*hWnd*/) override;

    //LRESULT OnTime(UINT, WPARAM wp, LPARAM, BOOL&);


    void Run() {};

    void SetAlpha(BYTE alpha) {
        m_info.SetAlpha(alpha);
    }

    void SetBrowser(CefRefPtr<CefBrowser> browser) {
        m_browser = browser;
    }

    void SetDraggableRegion(const std::vector<CefDraggableRegion>& regions);


    void SetToolTip(CefString& str);

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
        E_END,
        E_HTBASE = HTLEFT
    };

    void SetEdgeNcAera(HT_INFO ht, const std::vector<RECT> &vecRc);

    void ImePosChange(const CefRange& selected_range, const CefRenderHandler::RectList& character_bounds);



    //临时
    bool IsOverPopupWidget(int x, int y) const;
    int GetPopupXOffset() const;
    int GetPopupYOffset() const;
    void ApplyPopupOffset(int& x, int& y) const;
    int view_width_ = 0;
    int view_height_ = 0;
    CefRect popup_rect_;
    CefRect original_popup_rect_;

private:

    int view_width_old_ = 0;
    int view_height_old_ = 0;


    float device_scale_factor_ = 1.f;

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
    // Draggable region.
    HRGN draggable_region_ = {};
    HRGN edge_region_[E_END] = {};



    CefRefPtr<CefBrowser> m_browser;


};