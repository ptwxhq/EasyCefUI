#include "pch.h"
#include "EasyLayeredWindow.h"
#include "osr_ime_handler_win.h"
#include "EasyWebViewMgr.h"
#include <dwmapi.h>

#pragma comment(lib, "Dwmapi.lib")
#pragma comment(lib, "UxTheme.lib")




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
	//if (!m_size.cx || m_size.cy)
	//{
	//	return;
	//}

	m_info.hdcSrc = source;
	//m_info.hdcDst = GetDC(window);

	BOOL bRet = UpdateLayeredWindowIndirect(window, &m_info);
	VERIFY(bRet);
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

	//ReleaseDC(window, m_info.hdcDst);
	//m_info.hdcDst = nullptr;

	//使用完毕复原避免漏掉
	SetDirtyRect(nullptr);

	//m_info.pptDst = nullptr;
}




GdiBitmap::GdiBitmap(__in UINT width,
	__in UINT height) :
	m_width(width),
	m_height(height),
	m_stride((width * 32 + 31) / 32 * 4),
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

EasyLayeredWindow::~EasyLayeredWindow()
{
	if (IsWindow())
	{
		CWindow::DestroyWindow();
	}

}

void EasyLayeredWindow::ImePosChange(const CefRange& selected_range, const CefRenderHandler::RectList& character_bounds)
{
	if(ime_handler_)
		ime_handler_->ChangeCompositionRange(selected_range, character_bounds);
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
				last_mouse_pos_.x = current_mouse_pos_.x = x;
				last_mouse_pos_.y = current_mouse_pos_.y = y;
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
	//		render_handler_->SetSpin(0, 0);
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
				current_mouse_pos_.x = x;
				current_mouse_pos_.y = y;
				//render_handler_->IncrementSpin(
				//	current_mouse_pos_.x - last_mouse_pos_.x,
				//	current_mouse_pos_.y - last_mouse_pos_.y);
				//last_mouse_pos_.x = current_mouse_pos_.x;
				//last_mouse_pos_.y = current_mouse_pos_.y;
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

LRESULT EasyLayeredWindow::OnShowWindow(UINT, WPARAM wp, LPARAM lp, BOOL& h)
{
	if (m_browser)
		m_browser->GetHost()->WasHidden(!wp);
	h = FALSE;
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

	if (!m_bitmap)
	{
		m_bitmap = std::make_unique<GdiBitmap>(view_width_, view_height_);
	}


	//LOG(INFO) << GetCurrentProcessId() << "] OnSize new:" << view_width_ << " " << view_height_ << " old:" << view_width_old_ << " " << view_height_old_ << "\n";

	if (view_width_old_ != view_width_ || view_height_old_ != view_height_)
	{
		m_info.SetWindowSize({ view_width_, view_height_ });

		if (m_browser)
		{
			m_browser->GetHost()->WasResized();
		}

		view_width_old_ = view_width_;
		view_height_old_ = view_height_;
	}

	//UpdateHittestAera();

	return 0;
}

LRESULT EasyLayeredWindow::OnMove(UINT msg, WPARAM wp, LPARAM lp, BOOL& handle)
{
	//auto ret = DefWindowProc(msg, wp, lp);

	if (m_browser)
	{

		m_browser->GetHost()->NotifyMoveOrResizeStarted();
	}
	//handle = 0;
	return 0;
}

LRESULT EasyLayeredWindow::OnFocus(UINT msg, WPARAM, LPARAM, BOOL&)
{
	if (m_browser)
		m_browser->GetHost()->SendFocusEvent(WM_SETFOCUS == msg);
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
		SHORT scan_res = ::VkKeyScanExW(wp, current_layout);
		if (((scan_res >> 8) & 0xFF) == (2 | 4)) {  // ctrl-alt pressed
			event.modifiers &= ~(EVENTFLAG_CONTROL_DOWN | EVENTFLAG_ALT_DOWN);
			event.modifiers |= EVENTFLAG_ALTGR_DOWN;
		}
	}

	m_browser->GetHost()->SendKeyEvent(event);

	if (msg == WM_KEYUP && g_BrowserGlobalVar.Debug)
	{
		if (wp == VK_F12)
		{
			constexpr int nTestValue = EVENTFLAG_SHIFT_DOWN | EVENTFLAG_CONTROL_DOWN | EVENTFLAG_ALT_DOWN;

			if (!(event.modifiers & nTestValue))
			{
				if (m_browser && !m_browser->GetHost()->HasDevTools())
				{
					CefWindowInfo windowInfo;
					windowInfo.SetAsPopup(nullptr, L"dev");
					m_browser->GetHost()->ShowDevTools(windowInfo, nullptr, CefBrowserSettings(), CefPoint());
				}
			}
		}
	}


	return 0;
}

LRESULT EasyLayeredWindow::OnCreate(UINT, WPARAM, LPARAM lp, BOOL&)
{
	if (g_BrowserGlobalVar.FunctionFlag.bUIImeFollow)
		ime_handler_.reset(new client::OsrImeHandlerWin(m_hWnd));

	auto pCREATESTRUCT = (CREATESTRUCT*)lp;

	m_info.SetWindowPos({ pCREATESTRUCT->x, pCREATESTRUCT->y });
	m_info.SetWindowSize({ pCREATESTRUCT->cx, pCREATESTRUCT->cy });
	m_bitmap = std::make_unique<GdiBitmap>(pCREATESTRUCT->cx, pCREATESTRUCT->cy);

	//LOG(INFO) << GetCurrentProcessId() << "] OnCreate new:" << pCREATESTRUCT->x << pCREATESTRUCT->y << pCREATESTRUCT->cx << pCREATESTRUCT->cy << "\n";

	//SetWindowThemeNonClientAttributes(m_hWnd, WTNCA_NODRAWCAPTION, WTNCA_NODRAWCAPTION);

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



LRESULT EasyLayeredWindow::OnDwmCompositionChanged(UINT, WPARAM wp, LPARAM lp, BOOL&)
{
	MARGINS margins = { 0,0,1,0 };
	HRESULT hr = S_OK;

	// Extend the frame across the entire window.
	hr = DwmExtendFrameIntoClientArea(m_hWnd, &margins);
	VERIFYHR(hr);

	DWORD dw = DWMNCRP_ENABLED;

	DwmSetWindowAttribute(m_hWnd, DWMWA_NCRENDERING_POLICY,
		&dw, sizeof(DWORD));


	return 0;
}



LRESULT EasyLayeredWindow::OnNcActivate(UINT, WPARAM wp, LPARAM lp, BOOL& h)
{
	if (::IsIconic(*this)) h = FALSE;
	return (wp == 0) ? TRUE : FALSE;
}

LRESULT EasyLayeredWindow::OnActivate(UINT, WPARAM wp, LPARAM lp, BOOL& h)
{
	return 0;
}

LRESULT EasyLayeredWindow::OnPaint(UINT, WPARAM, LPARAM, BOOL& handle)
{
	Render();
	return 0;
}

void EasyLayeredWindow::SetBitmapData(const void* pData, int width, int height)
{
	if (!m_bitmap || m_bitmap->GetWidth() != width || m_bitmap->GetHeight() != height)
	{
		m_bitmap = std::make_unique<GdiBitmap>(width, height);
	}

	view_width_ = width;
	view_height_ = height;

	memcpy(m_bitmap->GetBits(), pData, width * height * 4);

	RECT rc = { 0, 0, width, height };
	m_info.SetDirtyRect(&rc);
}

void EasyLayeredWindow::SetBitmapData(const BYTE* pData, int x, int y, int width, int height, bool SameSize)
{
	ASSERT(x + width <= m_bitmap->GetWidth());
	ASSERT(y + height <= m_bitmap->GetHeight());

	if (SameSize)
	{
		for (int iLine = 0; iLine < height; iLine++)
		{
			int iPos = ((y + iLine) * m_bitmap->GetWidth() + x) * 4;
			memcpy(m_bitmap->GetBits() + iPos, pData + iPos, width * 4);
		}
	}
	else
	{
		for (int iLine = 0; iLine < height; iLine++)
		{
			memcpy(m_bitmap->GetBits() + ((y + iLine) * m_bitmap->GetWidth() + x) * 4, pData + iLine * width * 4, width * 4);
		}
	}

	RECT rc = { x,y, x + width,y + height };
	m_info.SetDirtyRect(&rc);
}

void EasyLayeredWindow::Render()
{
 	if (m_hWnd)
		m_info.Update(m_hWnd, m_bitmap->GetDC());
}

void EasyLayeredWindow::OnFinalMessage(HWND)
{
	//DEBUG版容易出现内部错误，为了方便调试，就先屏蔽了
#ifndef _DEBUG
	if (m_browser)
	{
		auto host = m_browser->GetHost();
		m_browser = nullptr;

		host->CloseBrowser(true);
	}
#endif
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
	if (!m_hToolTip)
	{
		m_hToolTip = CreateWindowExW(WS_EX_TRANSPARENT, TOOLTIPS_CLASS, nullptr,
			WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP | TTS_NOFADE,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
			m_hWnd, nullptr, g_BrowserGlobalVar.hInstance, nullptr);
	}

	TOOLINFOW ti = { sizeof(TOOLINFOW) };
	ti.uFlags = TTF_SUBCLASS;
	ti.hwnd = m_hWnd;
	ti.hinst = g_BrowserGlobalVar.hInstance;
	std::wstring strtmp = str.ToWString();
	ti.lpszText = (LPWSTR)strtmp.c_str();

	GetClientRect(&ti.rect);


	// Associate the tooltip with the "tool" window.
	::SendMessage(m_hToolTip, TTM_ADDTOOLW, 0, (LPARAM)(LPTOOLINFO)&ti);
	//::SendMessage(m_hToolTip, TTM_SETMAXTIPWIDTH, 0, -1);
}
