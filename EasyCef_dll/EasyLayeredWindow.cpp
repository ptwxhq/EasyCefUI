#include "pch.h"
#include "EasyLayeredWindow.h"
#include "cefclient/osr_ime_handler_win.h"
#include "cefclient/osr_dragdrop_win.h"
#include "EasyWebViewMgr.h"
#include <dwmapi.h>

#pragma comment(lib, "Dwmapi.lib")
#pragma comment(lib, "UxTheme.lib")

#define VERIFYHR(x) VERIFY(SUCCEEDED(x))



inline bool IsMouseEventFromTouch(UINT message) {
#define MOUSEEVENTF_FROMTOUCH 0xFF515700
	return (message >= WM_MOUSEFIRST) && (message <= WM_MOUSELAST) &&
		(GetMessageExtraInfo() & MOUSEEVENTF_FROMTOUCH) ==
		MOUSEEVENTF_FROMTOUCH;
}



LayeredWindowInfo::LayeredWindowInfo(
	__in LONG width,
	__in LONG height) :
	m_sourcePosition(),
	m_windowPosition(),
	m_size({ width, height }),
	m_dirty({ -1,-1,-1,-1 }),
	m_blend(),
	m_info() 
{

	m_info.cbSize = sizeof(UPDATELAYEREDWINDOWINFO);
	m_info.pptSrc = &m_sourcePosition;
	m_info.pptDst = nullptr/*&m_windowPosition*/;
	m_info.psize = &m_size;
	m_info.pblend = &m_blend;
	m_info.dwFlags = ULW_ALPHA;

	m_blend.SourceConstantAlpha = 255;
	m_blend.AlphaFormat = AC_SRC_ALPHA;
}


void LayeredWindowInfo::Update(__in HWND window, __in HDC source)
{
	m_info.hdcSrc = source;

	BOOL bRet = UpdateLayeredWindowIndirect(window, &m_info);
	ASSERT(bRet);
	if (!bRet)
	{
		//解决被脚本修改导致界面异常
		if (ERROR_INVALID_PARAMETER == GetLastError())
		{
			long val = GetWindowLong(window, GWL_EXSTYLE);
			val &= ~WS_EX_LAYERED;
			SetWindowLong(window, GWL_EXSTYLE, val);
			::SetWindowPos(window, 0, 0, 0, 0, 0,
				SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
			val = GetWindowLong(window, GWL_EXSTYLE);
			val |= WS_EX_LAYERED;
			SetWindowLong(window, GWL_EXSTYLE, val);
			::SetWindowPos(window, 0, 0, 0, 0, 0,
				SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
			bRet = UpdateLayeredWindow(window, m_info.hdcDst, const_cast<POINT*>(m_info.pptDst), const_cast<SIZE*>(m_info.psize), m_info.hdcSrc, const_cast<POINT*>(m_info.pptSrc), m_info.crKey, const_cast<BLENDFUNCTION*>(m_info.pblend), m_info.dwFlags);
			ASSERT(bRet);
		}
	
	}

	//使用完毕复原避免漏掉
	SetDirtyRect(nullptr);
}




GdiBitmap::GdiBitmap(__in UINT width,
	__in UINT height) :
	m_width(width),
	m_height(height),
	m_bits(0),
	m_oldBitmap(0) {

	BITMAPINFO bitmapInfo = { };
	bitmapInfo.bmiHeader.biSize =
		sizeof(bitmapInfo.bmiHeader);
	bitmapInfo.bmiHeader.biWidth =
		width;
	bitmapInfo.bmiHeader.biHeight =
		0 - height;
	bitmapInfo.bmiHeader.biPlanes = 1;
	bitmapInfo.bmiHeader.biBitCount = 32;
	bitmapInfo.bmiHeader.biCompression =
		BI_RGB;

	VERIFY(m_bitmap = CreateDIBSection(
		0, // device context
		&bitmapInfo,
		DIB_RGB_COLORS,
		(void**)&m_bits,
		0, // file mapping object
		0)); // file offset
	if (0 == m_bits) {
		throw;
	}

	VERIFY(m_dc = CreateCompatibleDC(nullptr));
	if (0 == m_dc) {
		throw;
	}

	m_oldBitmap = (HBITMAP)SelectObject(m_dc, m_bitmap);
}

GdiBitmap::~GdiBitmap()
{
	SelectObject(m_dc, m_oldBitmap);
	DeleteObject(m_bitmap);
	DeleteDC(m_dc);
}






EasyLayeredWindow::EasyLayeredWindow(): m_info(1, 1)
{
}


void EasyLayeredWindow::ImePosChange(const CefRange& selected_range, const CefRenderHandler::RectList& character_bounds)
{
	if (ime_handler_)
	{
		// Convert from view coordinates to device coordinates.
		CefRenderHandler::RectList device_bounds;
		CefRenderHandler::RectList::const_iterator it = character_bounds.begin();
		for (; it != character_bounds.end(); ++it) {
			device_bounds.push_back(LogicalToDevice(*it, device_scale_factor_));
		}
		ime_handler_->ChangeCompositionRange(selected_range, device_bounds);
	}
}

bool EasyLayeredWindow::StartDragging(CefRefPtr<CefDragData> drag_data, CefRenderHandler::DragOperationsMask allowed_ops, int x, int y)
{
	if (!drop_target_)
		return false;

	current_drag_op_ = DRAG_OPERATION_NONE;
	CefBrowserHost::DragOperationsMask result =
		drop_target_->StartDragging(m_browser, drag_data, allowed_ops, x, y);
	current_drag_op_ = DRAG_OPERATION_NONE;
	POINT pt = {};
	GetCursorPos(&pt);
	ScreenToClient(&pt);

	m_browser->GetHost()->DragSourceEndedAt(
		DeviceToLogical(pt.x, device_scale_factor_),
		DeviceToLogical(pt.y, device_scale_factor_), result);
	m_browser->GetHost()->DragSourceSystemDragEnded();
	return true;
}

void EasyLayeredWindow::SetPopupRectInWebView(const CefRect& original_rect)
{
	original_popup_rect_ = original_rect;
	CefRect rc(original_rect);
	// if x or y are negative, move them to 0.
	if (rc.x < 0)
		rc.x = 0;
	if (rc.y < 0)
		rc.y = 0;
	// if popup goes outside the view, try to reposition origin
	if (rc.x + rc.width > view_width_)
		rc.x = view_width_ - rc.width;
	if (rc.y + rc.height > view_height_)
		rc.y = view_height_ - rc.height;
	// if x or y became negative, move them to 0 again.
	if (rc.x < 0)
		rc.x = 0;
	if (rc.y < 0)
		rc.y = 0;

	popup_rect_ = rc;
}

void EasyLayeredWindow::ClearPopupRects()
{
	popup_rect_.Set(0, 0, 0, 0);
	original_popup_rect_.Set(0, 0, 0, 0);
}

bool EasyLayeredWindow::CheckViewSizeChanged(int width, int height)
{
	if (window_size_changed_)
	{
		if (paint_width_old_ != width || paint_height_old_ != height)
		{
			paint_width_old_ = width;
			paint_height_old_ = height;
			window_size_changed_ = false;
		}
	}

	return window_size_changed_;
}

void EasyLayeredWindow::DpiChangeWork()
{
	if (!g_BrowserGlobalVar.FunctionFlag.bEnableHignDpi)
	{
		return;
	}

	if (m_browser)
	{
		m_browser->GetHost()->NotifyScreenInfoChanged();
	}

	if (m_hToolTip)
	{
		::PostMessage(m_hToolTip, WM_CLOSE, 0, 0);
		m_hToolTip = nullptr;
	}
}

CefBrowserHost::DragOperationsMask EasyLayeredWindow::OnDragEnter(CefRefPtr<CefDragData> drag_data, CefMouseEvent ev, CefBrowserHost::DragOperationsMask effect)
{
	if (m_browser) {
		DeviceToLogical(ev, device_scale_factor_);
		m_browser->GetHost()->DragTargetDragEnter(drag_data, ev, effect);
		m_browser->GetHost()->DragTargetDragOver(ev, effect);
	}
	return current_drag_op_;
}

CefBrowserHost::DragOperationsMask EasyLayeredWindow::OnDragOver(CefMouseEvent ev, CefBrowserHost::DragOperationsMask effect)
{
	if (m_browser) {
		DeviceToLogical(ev, device_scale_factor_);
		m_browser->GetHost()->DragTargetDragOver(ev, effect);
	}
	return current_drag_op_;
}

void EasyLayeredWindow::OnDragLeave()
{
	if (m_browser)
		m_browser->GetHost()->DragTargetDragLeave();
}

CefBrowserHost::DragOperationsMask EasyLayeredWindow::OnDrop(CefMouseEvent ev, CefBrowserHost::DragOperationsMask effect)
{
	if (m_browser) {
		DeviceToLogical(ev, device_scale_factor_);
		m_browser->GetHost()->DragTargetDragOver(ev, effect);
		m_browser->GetHost()->DragTargetDrop(ev);
	}
	return current_drag_op_;
}

bool EasyLayeredWindow::IsOverPopupWidget(int x, int y) const {
	const CefRect& rc = popup_rect_;
	int popup_right = rc.x + rc.width;
	int popup_bottom = rc.y + rc.height;
	return (x >= rc.x) && (x < popup_right) && (y >= rc.y) && (y < popup_bottom);
}

int EasyLayeredWindow::GetPopupXOffset() const
{
	return original_popup_rect_.x - popup_rect_.x;
}

int EasyLayeredWindow::GetPopupYOffset() const
{
	return original_popup_rect_.y - popup_rect_.y;
}

void EasyLayeredWindow::ApplyPopupOffset(int& x, int& y) const
{
	if (IsOverPopupWidget(x, y)) {
		x += GetPopupXOffset();
		y += GetPopupYOffset();
	}
}

LRESULT EasyLayeredWindow::OnMouseEvent(UINT msg, WPARAM wp, LPARAM lp, BOOL&)
{
	if (IsMouseEventFromTouch(msg))
		return 0;

	CefRefPtr<CefBrowserHost> browserhost;
	if (m_browser)
		browserhost = m_browser->GetHost();

	LONG currentTime = 0;
	bool cancelPreviousClick = false;

	if (msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN ||
		msg == WM_MBUTTONDOWN || msg == WM_MOUSEMOVE ||
		msg == WM_MOUSELEAVE) {
		currentTime = GetMessageTime();
		int x = GET_X_LPARAM(lp);
		int y = GET_Y_LPARAM(lp);
		cancelPreviousClick =
			(abs(last_click_x_ - x) > (GetSystemMetrics(SM_CXDOUBLECLK) / 2)) ||
			(abs(last_click_y_ - y) > (GetSystemMetrics(SM_CYDOUBLECLK) / 2)) ||
			((currentTime - last_click_time_) > GetDoubleClickTime());
		if (cancelPreviousClick &&
			(msg == WM_MOUSEMOVE || msg == WM_MOUSELEAVE)) {
			last_click_count_ = 1;
			last_click_x_ = 0;
			last_click_y_ = 0;
			last_click_time_ = 0;
		}
	}

	switch (msg) {
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN: {
			::SetCapture(m_hWnd);
			::SetFocus(m_hWnd);
			int x = GET_X_LPARAM(lp);
			int y = GET_Y_LPARAM(lp);
			if (wp & MK_SHIFT) {
				// Start rotation effect.
				mouse_rotation_ = true;
			}
			else {
				CefBrowserHost::MouseButtonType btnType =
					(msg == WM_LBUTTONDOWN
						? MBT_LEFT
						: (msg == WM_RBUTTONDOWN ? MBT_RIGHT : MBT_MIDDLE));
				if (!cancelPreviousClick && (btnType == last_click_button_)) {
					++last_click_count_;
				}
				else {
					last_click_count_ = 1;
					last_click_x_ = x;
					last_click_y_ = y;
				}
				last_click_time_ = currentTime;
				last_click_button_ = btnType;

				if (browserhost) {
					CefMouseEvent mouse_event;
					mouse_event.x = x;
					mouse_event.y = y;
					last_mouse_down_on_view_ = !IsOverPopupWidget(x, y);
					ApplyPopupOffset(mouse_event.x, mouse_event.y);
					DeviceToLogical(mouse_event, device_scale_factor_);
					mouse_event.modifiers = GetCefMouseModifiers(wp);
					browserhost->SendMouseClickEvent(mouse_event, btnType, false,
						last_click_count_);
				}
			}
		} break;

	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
		if (GetCapture() == m_hWnd)
			ReleaseCapture();
		if (mouse_rotation_) {
			// End rotation effect.
			mouse_rotation_ = false;
		}
		else {
			int x = GET_X_LPARAM(lp);
			int y = GET_Y_LPARAM(lp);
			CefBrowserHost::MouseButtonType btnType =
				(msg == WM_LBUTTONUP
					? MBT_LEFT
					: (msg == WM_RBUTTONUP ? MBT_RIGHT : MBT_MIDDLE));
			if (browserhost) {
				CefMouseEvent mouse_event;
				mouse_event.x = x;
				mouse_event.y = y;
				if (last_mouse_down_on_view_ && IsOverPopupWidget(x, y) &&
					(GetPopupXOffset() || GetPopupYOffset())) {
					break;
				}
				ApplyPopupOffset(mouse_event.x, mouse_event.y);
				DeviceToLogical(mouse_event, device_scale_factor_);
				mouse_event.modifiers = GetCefMouseModifiers(wp);
				browserhost->SendMouseClickEvent(mouse_event, btnType, true,
					last_click_count_);
			}
		}
		break;

	case WM_MOUSEMOVE: {
			int x = GET_X_LPARAM(lp);
			int y = GET_Y_LPARAM(lp);
			if (mouse_rotation_) {
				// Apply rotation effect.
			}
			else {
				if (!mouse_tracking_) {
					// Start tracking mouse leave. Required for the WM_MOUSELEAVE event to
					// be generated.
					TRACKMOUSEEVENT tme;
					tme.cbSize = sizeof(TRACKMOUSEEVENT);
					tme.dwFlags = TME_LEAVE;
					tme.hwndTrack = m_hWnd;
					TrackMouseEvent(&tme);
					mouse_tracking_ = true;
				}

				if (browserhost) {
					CefMouseEvent mouse_event;
					mouse_event.x = x;
					mouse_event.y = y;
					ApplyPopupOffset(mouse_event.x, mouse_event.y);
					DeviceToLogical(mouse_event, device_scale_factor_);
					mouse_event.modifiers = GetCefMouseModifiers(wp);
					browserhost->SendMouseMoveEvent(mouse_event, false);
				}
			}
			break;
		}

	case WM_MOUSELEAVE: {
			if (mouse_tracking_) {
				// Stop tracking mouse leave.
				TRACKMOUSEEVENT tme;
				tme.cbSize = sizeof(TRACKMOUSEEVENT);
				tme.dwFlags = TME_LEAVE & TME_CANCEL;
				tme.hwndTrack = m_hWnd;
				TrackMouseEvent(&tme);
				mouse_tracking_ = false;
			}

			if (browserhost) {
				// Determine the cursor position in screen coordinates.
				POINT p;
				::GetCursorPos(&p);
				::ScreenToClient(m_hWnd, &p);

				CefMouseEvent mouse_event;
				mouse_event.x = p.x;
				mouse_event.y = p.y;
				DeviceToLogical(mouse_event, device_scale_factor_);
				mouse_event.modifiers = GetCefMouseModifiers(wp);
				browserhost->SendMouseMoveEvent(mouse_event, true);
			}
		} break;

	case WM_MOUSEWHEEL:
		if (browserhost) {
			POINT screen_point = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
			HWND scrolled_wnd = ::WindowFromPoint(screen_point);
			if (scrolled_wnd != m_hWnd)
				break;

			ScreenToClient(&screen_point);
			int delta = GET_WHEEL_DELTA_WPARAM(wp);

			CefMouseEvent mouse_event;
			mouse_event.x = screen_point.x;
			mouse_event.y = screen_point.y;
			ApplyPopupOffset(mouse_event.x, mouse_event.y);
			DeviceToLogical(mouse_event, device_scale_factor_);
			mouse_event.modifiers = GetCefMouseModifiers(wp);
			browserhost->SendMouseWheelEvent(mouse_event,
				IsKeyDown(VK_SHIFT) ? delta : 0,
				!IsKeyDown(VK_SHIFT) ? delta : 0);
		}
		break;
	}

	return 0;
}

LRESULT EasyLayeredWindow::OnWindowPosChanged(UINT, WPARAM wp, LPARAM lp, BOOL& h)
{
	h = FALSE;
	auto pWndPos = (LPWINDOWPOS)lp;
	if (pWndPos)
	{
		bool bShow = pWndPos->flags & SWP_SHOWWINDOW;
		if (bShow || (pWndPos->flags & SWP_HIDEWINDOW))
		{
			if (m_browser)
				m_browser->GetHost()->WasHidden(!bShow);
		}
	}


	return 0;
}

LRESULT EasyLayeredWindow::OnSize(UINT, WPARAM wp, LPARAM lp, BOOL& h)
{
	if (wp == SIZE_MINIMIZED)
	{
		h = FALSE;
		return 0;
	}


	if (lp == 0)
		return 0;


	view_width_ = LOWORD(lp);
	view_height_ = HIWORD(lp);

	if (view_width_old_ != view_width_ || view_height_old_ != view_height_)
	{
		window_size_changed_ = true;
		m_info.SetWindowSize({ view_width_, view_height_ });

		if (m_browser)
		{
			m_browser->GetHost()->WasResized();
		}

		view_width_old_ = view_width_;
		view_height_old_ = view_height_;
	}

	return 0;
}

LRESULT EasyLayeredWindow::OnFocus(UINT msg, WPARAM, LPARAM, BOOL&)
{
#if CEF_VERSION_MAJOR < 95
	if (m_browser)
		m_browser->GetHost()->SendFocusEvent(WM_SETFOCUS == msg);
#endif
	return 0;
}

LRESULT EasyLayeredWindow::OnCaptureLost(UINT, WPARAM, LPARAM, BOOL&)
{
	if (!mouse_rotation_)
	{
		if (m_browser)
			m_browser->GetHost()->SendCaptureLostEvent();
	}

	return 0;
}

LRESULT EasyLayeredWindow::OnKeyEvent(UINT msg, WPARAM wp, LPARAM lp, BOOL&)
{
	if (!m_browser)
		return 0;

	CefKeyEvent event;
	event.windows_key_code = wp;
	event.native_key_code = lp;
	event.is_system_key = msg == WM_SYSCHAR || msg == WM_SYSKEYDOWN ||
		msg == WM_SYSKEYUP;

	if (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN)
		event.type = KEYEVENT_RAWKEYDOWN;
	else if (msg == WM_KEYUP || msg == WM_SYSKEYUP)
		event.type = KEYEVENT_KEYUP;
	else
		event.type = KEYEVENT_CHAR;
	event.modifiers = GetCefKeyboardModifiers(wp, lp);

	// mimic alt-gr check behaviour from
	// src/ui/events/win/events_win_utils.cc: GetModifiersFromKeyState
	if ((event.type == KEYEVENT_CHAR) && IsKeyDown(VK_RMENU)) {
		// reverse AltGr detection taken from PlatformKeyMap::UsesAltGraph
		// instead of checking all combination for ctrl-alt, just check current char
		HKL current_layout = ::GetKeyboardLayout(0);

		// https://docs.microsoft.com/en-gb/windows/win32/api/winuser/nf-winuser-vkkeyscanexw
		// ... high-order byte contains the shift state,
		// which can be a combination of the following flag bits.
		// 2 Either CTRL key is pressed.
		// 4 Either ALT key is pressed.
		SHORT scan_res = ::VkKeyScanExW((WCHAR)wp, current_layout);
		if (((scan_res >> 8) & 0xFF) == (2 | 4)) {  // ctrl-alt pressed
			event.modifiers &= ~(EVENTFLAG_CONTROL_DOWN | EVENTFLAG_ALT_DOWN);
			event.modifiers |= EVENTFLAG_ALTGR_DOWN;
		}
	}

	m_browser->GetHost()->SendKeyEvent(event);



	return 0;
}

LRESULT EasyLayeredWindow::OnCreate(UINT, WPARAM, LPARAM lp, BOOL& handle)
{
	auto pCREATESTRUCT = (CREATESTRUCT*)lp;
	Cls_OnCreate(m_hWnd, pCREATESTRUCT);

	handle = TRUE;
	if (g_BrowserGlobalVar.FunctionFlag.bUIImeFollow)
		ime_handler_.reset(new client::OsrImeHandlerWin(m_hWnd));



	m_info.SetWindowPos({ pCREATESTRUCT->x, pCREATESTRUCT->y });
	m_info.SetWindowSize({ pCREATESTRUCT->cx, pCREATESTRUCT->cy });
	m_bitmap = std::make_unique<GdiBitmap>(pCREATESTRUCT->cx, pCREATESTRUCT->cy);

	//LOG(INFO) << GetCurrentProcessId() << "] OnCreate new:" << pCREATESTRUCT->x << pCREATESTRUCT->y << pCREATESTRUCT->cx << pCREATESTRUCT->cy << "\n";

	//SetWindowThemeNonClientAttributes(m_hWnd, WTNCA_NODRAWCAPTION, WTNCA_NODRAWCAPTION);

	drop_target_ = client::DropTargetWin::Create(this, m_hWnd);
	VERIFYHR(RegisterDragDrop(m_hWnd, drop_target_));


	return 0;
}

LRESULT EasyLayeredWindow::OnDestroy(UINT, WPARAM, LPARAM, BOOL& handle)
{
	handle = FALSE;

	::SendMessage(m_hToolTip, WM_CLOSE, 0, 0);

	RevokeDragDrop(m_hWnd);
	drop_target_ = nullptr;

	ime_handler_.reset();

	return 0;
}

LRESULT EasyLayeredWindow::OnIMESetContext(UINT msg, WPARAM wp, LPARAM lp, BOOL& h)
{
	if (ime_handler_) {

		// We handle the IME Composition Window ourselves (but let the IME Candidates
// Window be handled by IME through DefWindowProc()), so clear the
// ISC_SHOWUICOMPOSITIONWINDOW flag:
		lp &= ~ISC_SHOWUICOMPOSITIONWINDOW;
		DefWindowProc(msg, wp, lp);

		// Create Caret Window if required
		ime_handler_->CreateImeWindow();
		ime_handler_->MoveImeWindow();
	}
	else
	{
		h = FALSE;
	}
	return 0;
}

LRESULT EasyLayeredWindow::OnIMEStartComposition(UINT, WPARAM, LPARAM, BOOL& h)
{
	if (ime_handler_) {
		ime_handler_->CreateImeWindow();
		ime_handler_->MoveImeWindow();
		ime_handler_->ResetComposition();
	}
	else
	{
		h = FALSE;
	}
	return 0;
}

LRESULT EasyLayeredWindow::OnIMEComposition(UINT, WPARAM, LPARAM lp, BOOL& h)
{
	if (m_browser && ime_handler_) {
		CefString cTextStr;
		if (ime_handler_->GetResult(lp, cTextStr)) {
			// Send the text to the browser. The |replacement_range| and
			// |relative_cursor_pos| params are not used on Windows, so provide
			// default invalid values.
			m_browser->GetHost()->ImeCommitText(cTextStr,
				CefRange(UINT32_MAX, UINT32_MAX), 0);
			ime_handler_->ResetComposition();
			// Continue reading the composition string - Japanese IMEs send both
			// GCS_RESULTSTR and GCS_COMPSTR.
		}

		std::vector<CefCompositionUnderline> underlines;
		int composition_start = 0;

		if (ime_handler_->GetComposition(lp, cTextStr, underlines,
			composition_start)) {
			// Send the composition string to the browser. The |replacement_range|
			// param is not used on Windows, so provide a default invalid value.
			m_browser->GetHost()->ImeSetComposition(
				cTextStr, underlines, CefRange(UINT32_MAX, UINT32_MAX),
				CefRange(composition_start,
					static_cast<int>(composition_start + cTextStr.length())));

			// Update the Candidate Window position. The cursor is at the end so
			// subtract 1. This is safe because IMM32 does not support non-zero-width
			// in a composition. Also,  negative values are safely ignored in
			// MoveImeWindow
			ime_handler_->UpdateCaretPosition(composition_start - 1);
		}
		else {
			BOOL b;
			OnIMECancelCompositionEvent(0, 0, 0, b);
		}
	}
	else
	{
		h = FALSE;
	}
	return 0;
}

LRESULT EasyLayeredWindow::OnIMECancelCompositionEvent(UINT, WPARAM, LPARAM, BOOL& h)
{
	if (m_browser && ime_handler_) {
		m_browser->GetHost()->ImeCancelComposition();
		ime_handler_->ResetComposition();
		ime_handler_->DestroyImeWindow();
	}
	else
	{
		h = FALSE;
	}
	return 0;
}

LRESULT EasyLayeredWindow::OnSettingChange(UINT, WPARAM wp, LPARAM lp, BOOL& handle)
{
	handle = FALSE;
	auto str = (LPCWSTR)lp;
	if (str && !wcscmp(str, L"ImmersiveColorSet"))
	{
		if (::IsWindow(m_hToolTip))
		{
			::PostMessage(m_hToolTip, WM_CLOSE, 0, 0);
			m_hToolTip = nullptr;
		}

	}

	return 0;
}

LRESULT EasyLayeredWindow::OnPaint(UINT, WPARAM, LPARAM, BOOL& handle)
{
	Render();
	return 0;
}

bool EasyLayeredWindow::SetBitmapData(const void* pData, int width, int height)
{
	if (!m_bitmap || m_bitmap->GetWidth() != width || m_bitmap->GetHeight() != height)
	{
		m_bitmap = std::make_unique<GdiBitmap>(width, height);
	}

	if (view_width_ != width || view_height_ != height)
	{
		//可能是从非标准dpi转为标准dpi
		return false;
	}

	ASSERT(view_width_ == width && view_height_ == height);

	memcpy(m_bitmap->GetBits(), pData, width * height * 4);

	m_info.SetDirtyRect(nullptr);

	return true;
}

bool EasyLayeredWindow::SetBitmapData(const void* pData, int x, int y, int width, int height, bool SameSize, int src_x, int src_y, int origin_width, int origin_height)
{
	if (x < 0 || y < 0)
	{
		return false;
	}


	if (x + width > m_bitmap->GetWidth() || y + height > m_bitmap->GetHeight())
	{
		m_bitmap = std::make_unique<GdiBitmap>(view_width_, view_height_);
		m_browser->GetHost()->WasResized();
		m_browser->GetHost()->Invalidate(PET_VIEW);
		return false;
	}

	const int nCopySize = width * 4;
	BYTE* pDest = m_bitmap->GetBits();
	auto pSrc = static_cast<const BYTE*>(pData);

	if (SameSize)
	{
		if (m_bitmap->GetWidth() != origin_width || m_bitmap->GetHeight() != origin_height)
		{
			LOG(WARNING) << GetCurrentProcessId() << "] SetBitmapData: check failed! ";
			m_browser->GetHost()->WasResized();
			return false;
		}

		for (int nLine = 0; nLine < height; nLine++)
		{
			const int nDstPos = ((y + nLine) * m_bitmap->GetWidth() + x) * 4;
			memcpy(pDest + nDstPos, pSrc + nDstPos, nCopySize);
		}


		m_info.SetDirtyRect(nullptr);
	}
	else
	{
		if (src_x > origin_width || (src_y + height > origin_height) || x > m_bitmap->GetWidth() || (y + height > m_bitmap->GetHeight()))
		{
			LOG(WARNING) << GetCurrentProcessId() << "] SetBitmapData s: check failed! ";
			m_browser->GetHost()->WasResized();
			return false;
		}

		for (int nLine = 0; nLine < height; nLine++)
		{
			const int nSrcPos = ((src_y + nLine) * origin_width + src_x) * 4;
			const int nDstPos = ((y + nLine) * m_bitmap->GetWidth() + x) * 4;
			memcpy(pDest + nSrcPos, pSrc + nDstPos, nCopySize);
		}

		if (src_x == 0 && src_y == 0 && origin_width == width)
		{
			//弹出的时候清除信息，以免页面内容不刷新
			m_info.SetDirtyRect(nullptr);
		}
		else
		{
			RECT rc = { x, y, x + width, y + height };
			m_info.SetDirtyRect(&rc);
		}
	}

	return true;
}

void EasyLayeredWindow::Render()
{
 	if (m_hWnd)
	{
		m_info.Update(m_hWnd, m_bitmap->GetDC());
	}
}



void EasyLayeredWindow::SetAlpha(BYTE alpha, bool bRepaint)
{
	m_info.SetAlpha(alpha);
	if (bRepaint)
	{
		PostMessage(WM_PAINT);
	}
}

void EasyLayeredWindow::SetToolTip(const CefString& str)
{
	std::hash<std::wstring> hash;

	if (!::IsWindow(m_hToolTip))
	{
		m_hToolTip = CreateWindowExW(WS_EX_TRANSPARENT, TOOLTIPS_CLASS, nullptr,
			WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP | TTS_NOFADE,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
			m_hWnd, nullptr, g_BrowserGlobalVar.hInstance, nullptr);

		bool bUseDark = false;
		if (g_BrowserGlobalVar.DarkModeType == PreferredAppMode::AllowDark)
		{
			DWORD dwData = 1;
			CRegKey reg;
			try
			{
				if (reg.Open(HKEY_CURRENT_USER, LR"(Software\Microsoft\Windows\CurrentVersion\Themes\Personalize)", KEY_QUERY_VALUE) == ERROR_SUCCESS)
				{
					reg.QueryDWORDValue(L"AppsUseLightTheme", dwData);
				}
			}
			catch (...)
			{

			}

			if (dwData == 0)
			{
				bUseDark = true;
			}

		}
		else if (g_BrowserGlobalVar.DarkModeType == PreferredAppMode::ForceDark)
		{
			bUseDark = true;
		}

		if (bUseDark)
		{
			SetWindowTheme(m_hToolTip, L"DarkMode_Explorer", nullptr);
		}

		m_pToolInfo = std::make_unique<TOOLINFOW>();
		memset(m_pToolInfo.get(), 0, sizeof(TOOLINFOW));
		m_pToolInfo->cbSize = sizeof(TOOLINFOW);
		m_pToolInfo->uFlags = TTF_SUBCLASS | TTF_IDISHWND;
		m_pToolInfo->hwnd = m_hWnd;
		m_pToolInfo->hinst = g_BrowserGlobalVar.hInstance;
		m_pToolInfo->uId = (UINT_PTR)m_hWnd;

		m_sCurrentToolTipTextHash = hash(L"");
		::SendMessage(m_hToolTip, TTM_ADDTOOLW, 0, (LPARAM)m_pToolInfo.get());
		::SendMessage(m_hToolTip, TTM_SETMAXTIPWIDTH, 0, 800);
	}


	std::wstring strtmp = str.ToWString();


	auto sNewToolTipTextHash = hash(strtmp);
	if (m_sCurrentToolTipTextHash != sNewToolTipTextHash)
	{
		m_sCurrentToolTipTextHash = sNewToolTipTextHash;
		m_pToolInfo->lpszText = (LPWSTR)strtmp.c_str();

		POINT pt;
		GetCursorPos(&pt);
		ScreenToClient(&pt);

		m_pToolInfo->rect = { pt.x - 10, pt.y - 10, pt.x + 10, pt.y + 10 };

		::SendMessage(m_hToolTip, TTM_SETTOOLINFOW, 0, (LPARAM)m_pToolInfo.get());
	}
}



void EasyMiniLayeredWindow::SetBitmapData(const void* pData, int width, int height)
{
	if (!m_bitmap || m_bitmap->GetWidth() != width || m_bitmap->GetHeight() != height)
	{
		m_bitmap = std::make_unique<GdiBitmap>(width, height);
	}

	memcpy(m_bitmap->GetBits(), pData, width * height * 4);

	m_info.SetDirtyRect(nullptr);
}

void EasyMiniLayeredWindow::Render()
{
	if (m_hWnd)
		m_info.Update(m_hWnd, m_bitmap->GetDC());
}

EasyMiniLayeredWindow::EasyMiniLayeredWindow() : m_info(1, 1)
{
}
