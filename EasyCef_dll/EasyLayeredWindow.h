#pragma once

#include "EasyUIWindow.h"
#include "cefclient/osr_dragdrop_events.h"
#include "cefclient/osr_dragdrop_win.h"

namespace client {
    class OsrImeHandlerWin;
    //class DropTargetWin;
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

    BYTE* GetBits() const {
        return m_bits;
    }

    HDC GetDC() const {
        return m_dc;
    }
};


class EasyLayeredWindow :
    public EasyUIWindowBase,
    public client::OsrDragEvents
{
public:
    LayeredWindowInfo m_info;
private:


    std::unique_ptr<GdiBitmap> m_bitmap;

public:

    EasyLayeredWindow();

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

        MESSAGE_HANDLER(WM_WINDOWPOSCHANGED, OnWindowPosChanged)
        MESSAGE_HANDLER(WM_SIZE, OnSize)


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
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)

        MESSAGE_HANDLER(WM_IME_SETCONTEXT, OnIMESetContext)
        MESSAGE_HANDLER(WM_IME_STARTCOMPOSITION, OnIMEStartComposition)
        MESSAGE_HANDLER(WM_IME_COMPOSITION, OnIMEComposition)
        MESSAGE_HANDLER(WM_IME_ENDCOMPOSITION, OnIMECancelCompositionEvent)

        MESSAGE_HANDLER(WM_WININICHANGE, OnWinIniChange)
        CHAIN_MSG_MAP(EasyUIWindowBase)
    END_MSG_MAP()

    LRESULT OnMouseEvent(UINT msg, WPARAM wp, LPARAM lp, BOOL&);
    LRESULT OnWindowPosChanged(UINT msg, WPARAM wp, LPARAM lp, BOOL& handle);
    LRESULT OnSize(UINT msg, WPARAM wp,LPARAM lp, BOOL &handle);

    LRESULT OnFocus(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnCaptureLost(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnKeyEvent(UINT msg, WPARAM wp, LPARAM lp, BOOL&);

    LRESULT OnCreate(UINT, WPARAM, LPARAM, BOOL& handle);
    LRESULT OnDestroy(UINT, WPARAM, LPARAM, BOOL& handle);
    LRESULT OnIMESetContext(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnIMEStartComposition(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnIMEComposition(UINT, WPARAM, LPARAM lp, BOOL&);
    LRESULT OnIMECancelCompositionEvent(UINT, WPARAM, LPARAM, BOOL&);

    LRESULT OnWinIniChange(UINT, WPARAM, LPARAM, BOOL& handle);

    LRESULT OnPaint(UINT, WPARAM, LPARAM, BOOL& handle);

    LRESULT OnIgnore(UINT, WPARAM, LPARAM, BOOL&) { return 0; }

    bool SetBitmapData(const void* pData, int width, int height);
    bool SetBitmapData(const BYTE* pData, int x, int y, int width, int height, bool SameSize, int src_width);
    void Render();


    //LRESULT OnTime(UINT, WPARAM wp, LPARAM, BOOL&);

    void SetAlpha(BYTE alpha, bool bRepaint) override;


    void SetToolTip(const CefString& str);

    void ImePosChange(const CefRange& selected_range, const CefRenderHandler::RectList& character_bounds);

    bool StartDragging(CefRefPtr<CefDragData> drag_data, CefRenderHandler::DragOperationsMask allowed_ops, int x, int y);

    void UpdateDragCursor(CefRenderHandler::DragOperationsMask operation) {
        current_drag_op_ = operation;
    }


    void SetPopupRectInWebView(const CefRect& original_rect);
    void ClearPopupRects();

    CefRect GetPopupRect() const {
        return popup_rect_;
    };

    bool CheckViewSizeChanged(int width, int height);

    bool IsTransparentUI() override { return true; }

    void DpiChangeWork() override;

private:


    CefBrowserHost::DragOperationsMask OnDragEnter(
        CefRefPtr<CefDragData> drag_data,
        CefMouseEvent ev,
        CefBrowserHost::DragOperationsMask effect) override;
    CefBrowserHost::DragOperationsMask OnDragOver(
        CefMouseEvent ev,
        CefBrowserHost::DragOperationsMask effect) override;
    void OnDragLeave() override;
    CefBrowserHost::DragOperationsMask OnDrop(
        CefMouseEvent ev,
        CefBrowserHost::DragOperationsMask effect) override;

    bool IsOverPopupWidget(int x, int y) const;
    void ApplyPopupOffset(int& x, int& y) const;
    int GetPopupXOffset() const;
    int GetPopupYOffset() const;


    bool window_size_changed_ = true;
    int paint_width_old_ = 0;
    int paint_height_old_ = 0;

    int view_width_old_ = 0;
    int view_height_old_ = 0;
    int view_width_ = 0;
    int view_height_ = 0;
    CefRect popup_rect_;
    CefRect original_popup_rect_;


    // Mouse state tracking.
    bool mouse_rotation_ = {};
    bool mouse_tracking_ = {};
    int last_click_x_ = {};
    int last_click_y_ = {};
    CefBrowserHost::MouseButtonType last_click_button_ = {};
    int last_click_count_ = {};
    double last_click_time_ = {};
    bool last_mouse_down_on_view_ = {};

    HWND m_hToolTip = nullptr;
    size_t m_sCurrentToolTipTextHash = 0;
    std::unique_ptr<TOOLINFOW> m_pToolInfo;

    // Class that encapsulates IMM32 APIs and controls IMEs attached to a window.
    std::shared_ptr<client::OsrImeHandlerWin> ime_handler_;

    CComPtr<client::DropTargetWin> drop_target_;
    CefRenderHandler::DragOperation current_drag_op_ = DRAG_OPERATION_NONE;

};

class EasyMiniLayeredWindow : public CWindowImpl<EasyMiniLayeredWindow, CWindow, CWinTraits <WS_OVERLAPPED, WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW>>
{
public:
    LayeredWindowInfo m_info;


    void SetBitmapData(const void* pData, int width, int height);

    void Render();

    EasyMiniLayeredWindow();

    BEGIN_MSG_MAP(EasyMiniLayeredWindow)
    END_MSG_MAP()

private:


    std::unique_ptr<GdiBitmap> m_bitmap;
};