#include "pch.h"
#include "EasyUIWindow.h"
#include "EasyWebViewMgr.h"
#include "Utility.h"
#include <Windowsx.h>
#undef SubclassWindow



namespace {

LPCWSTR kParentWndProc = L"CefParentWndProc";
LPCWSTR kEdgeRegions = L"CefEdgeRegions";

LRESULT CALLBACK SubclassedWindowProc(HWND hWnd,
	UINT message,
	WPARAM wParam,
	LPARAM lParam) {
	WNDPROC hParentWndProc =
		reinterpret_cast<WNDPROC>(::GetPropW(hWnd, kParentWndProc));
	HRGN* hEdgeRegions = reinterpret_cast<HRGN*>(::GetPropW(hWnd, kEdgeRegions));

	if (message == WM_NCHITTEST) {
		LRESULT hit = CallWindowProc(hParentWndProc, hWnd, message, wParam, lParam);
		if (hit == HTCLIENT) {
			POINTS points = MAKEPOINTS(lParam);
			POINT point = { points.x, points.y };
			::ScreenToClient(hWnd, &point);

			//顺序先从边角开始
			static int nRound[] = {
				EasyUIWindowBase::E_HTTOPLEFT,EasyUIWindowBase::E_HTTOPRIGHT,EasyUIWindowBase::E_HTBOTTOMLEFT,EasyUIWindowBase::E_HTBOTTOMRIGHT,
				EasyUIWindowBase::E_HTTOP,EasyUIWindowBase::E_HTLEFT,EasyUIWindowBase::E_HTRIGHT,EasyUIWindowBase::E_HTBOTTOM,
				 EasyUIWindowBase::E_HTMAXBUTTON,
				EasyUIWindowBase::E_CAPTION
			};

			for (int i = 0; i < _countof(nRound); i++)
			{
				if (PtInRegion(hEdgeRegions[nRound[i]], point.x, point.y))
				{
					// Let the parent window handle WM_NCHITTEST by returning HTTRANSPARENT
					// in child windows.
					return HTTRANSPARENT;
				}
			}
		}
		return hit;
	}

	return CallWindowProc(hParentWndProc, hWnd, message, wParam, lParam);
}

void SubclassWindow(HWND hWnd, HRGN* hRegions) {
	HANDLE hParentWndProc = ::GetPropW(hWnd, kParentWndProc);
	if (hParentWndProc) {
		return;
	}

	SetLastError(0);
	LONG_PTR hOldWndProc = SetWindowLongPtr(
		hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(SubclassedWindowProc));
	if (hOldWndProc == 0 && GetLastError() != ERROR_SUCCESS) {
		return;
	}

	::SetPropW(hWnd, kParentWndProc, reinterpret_cast<HANDLE>(hOldWndProc));
	::SetPropW(hWnd, kEdgeRegions, reinterpret_cast<HANDLE*>(hRegions));
}

void UnSubclassWindow(HWND hWnd) {
	LONG_PTR hParentWndProc =
		reinterpret_cast<LONG_PTR>(::GetPropW(hWnd, kParentWndProc));
	if (hParentWndProc) {
		[[maybe_unused]] LONG_PTR hPreviousWndProc =
			SetWindowLongPtr(hWnd, GWLP_WNDPROC, hParentWndProc);
		DCHECK_EQ(hPreviousWndProc,
			reinterpret_cast<LONG_PTR>(SubclassedWindowProc));
	}

	::RemovePropW(hWnd, kParentWndProc);
	::RemovePropW(hWnd, kEdgeRegions);
}

BOOL CALLBACK SubclassWindowsProc(HWND hwnd, LPARAM lParam) {
	SubclassWindow(hwnd, reinterpret_cast<HRGN*>(lParam));
	return TRUE;
}

BOOL CALLBACK UnSubclassWindowsProc(HWND hwnd, LPARAM lParam) {
	UnSubclassWindow(hwnd);
	return TRUE;
}

}

void EasyUIWindowBase::SetDraggableRegion(const std::vector<CefDraggableRegion>& regions)
{
	::SetRectRgn(m_EdgeRegions[E_CAPTION], 0, 0, 0, 0);

	m_bEdgeRegionExist[E_CAPTION] = !regions.empty();

	// Determine new draggable region.
	std::vector<CefDraggableRegion>::const_iterator it = regions.begin();
	for (; it != regions.end(); ++it) {
		const auto realRegion = LogicalToDevice({ it->bounds.x, it->bounds.y, it->bounds.width, it->bounds.height }, device_scale_factor_);
		HRGN region = ::CreateRectRgn(realRegion.x, realRegion.y,
			realRegion.x + realRegion.width,
			realRegion.y + realRegion.height);
		::CombineRgn(m_EdgeRegions[E_CAPTION], m_EdgeRegions[E_CAPTION], region,
			it->draggable ? RGN_OR : RGN_DIFF);


		//LOG(INFO) << "Range:" << std::format("{},{},{},{}\n", it->bounds.x , it->bounds.y ,it->bounds.width, it->bounds.height);

		::DeleteObject(region);
	}

}

void EasyUIWindowBase::SetEdgeNcAera(HT_INFO ht, const std::vector<RECT>& vecRc)
{
	::SetRectRgn(m_EdgeRegions[ht], 0, 0, 0, 0);

	m_bEdgeRegionExist[ht] = !vecRc.empty();

	for (auto& it : vecRc)
	{
		const auto realRegion = LogicalToDevice({ it.left, it.top, it.right, it.bottom }, device_scale_factor_);

		HRGN region = ::CreateRectRgn(realRegion.x, realRegion.y, realRegion.width, realRegion.height);
		::CombineRgn(m_EdgeRegions[ht], m_EdgeRegions[ht], region, true ? RGN_OR : RGN_DIFF);
		::DeleteObject(region);
	}
}

bool EasyUIWindowBase::IsNcAeraExist()
{
	for (int i = E_CAPTION; i > -1; i--)
	{
		if (m_bEdgeRegionExist[i])
			return true;
	}
	
	return false;
}

void EasyUIWindowBase::SubclassChildHitTest(bool bSet)
{
	//// Subclass child window procedures in order to do hit-testing.
	//// This will be a no-op, if it is already subclassed.
	if (m_hWnd) {
		WNDENUMPROC proc =
			bSet ? SubclassWindowsProc : UnSubclassWindowsProc;
		::EnumChildWindows(m_hWnd, proc,
			reinterpret_cast<LPARAM>(m_EdgeRegions));
	}
}

EasyUIWindowBase::EasyUIWindowBase()
{
	for (int i = 0; i < _countof(m_EdgeRegions); i++)
	{
		m_EdgeRegions[i] = CreateRectRgn(0, 0, 0, 0);
	}
}

EasyUIWindowBase::~EasyUIWindowBase()
{
	if (IsWindow())
	{
		CWindow::DestroyWindow();
	}

	for (int i = 0; i < _countof(m_EdgeRegions); i++)
	{
		DeleteObject(m_EdgeRegions[i]);
		m_EdgeRegions[i] = nullptr;
	}
}

void EasyUIWindowBase::OnFinalMessage(HWND h)
{
	if (m_browser)
	{
		m_browser->GetHost()->CloseBrowser(true);
	}

	EasyWebViewMgr::GetInstance().CleanDelayItem(h);
}


UINT EasyUIWindowBase::Cls_OnNCHitTest(HWND hwnd, int x, int y)
{
	const int hit = DefWindowProc(WM_NCHITTEST, 0, MAKELONG(x, y)) & UINT32_MAX;
	//bool bIsSize = false && (hit >= HTSIZEFIRST && hit <= HTSIZELAST);
	if (hit == HTCLIENT)
	{
		POINT point = { x, y };
		ScreenToClient(&point);

		//顺序先从边角开始
		static int nRound[] = {
			E_HTMAXBUTTON,E_HTTOPLEFT,E_HTTOPRIGHT,E_HTBOTTOMLEFT,E_HTBOTTOMRIGHT,
			E_HTTOP,E_HTLEFT,E_HTRIGHT,E_HTBOTTOM
		};

		for (int i = 0; i < _countof(nRound); i++)
		{
			if (PtInRegion(m_EdgeRegions[nRound[i]], point.x, point.y))
			{
				return E_HTBASE + nRound[i];
			}
		}

		if (PtInRegion(m_EdgeRegions[E_CAPTION], point.x, point.y)) {
			// If cursor is inside a draggable region return HTCAPTION to allow
			// dragging.
			return HTCAPTION;
		}

		//if (bIsSize)
		//{
		//	return HTCLIENT;
		//}

	}

	return hit;
}

void EasyUIWindowBase::Cls_OnGetMinMaxInfo(HWND hWnd, LPMINMAXINFO lpMinMaxInfo)
{
	RECT rcScr;
	auto hMoni = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);

	MONITORINFO MoInfo = { sizeof(MoInfo) };

	if (GetMonitorInfoW(hMoni, &MoInfo))
	{
		CopyRect(&rcScr, &MoInfo.rcWork);
	}
	else
	{
		SystemParametersInfo(SPI_GETWORKAREA, 0, &rcScr, 0);
	}

	lpMinMaxInfo->ptMaxSize.x = rcScr.right - rcScr.left;
	lpMinMaxInfo->ptMaxSize.y = rcScr.bottom - rcScr.top;
	//以下值是基于主显示器的，不能用虚拟坐标
	lpMinMaxInfo->ptMaxPosition.x = 0;
	lpMinMaxInfo->ptMaxPosition.y = 0;
}

LRESULT EasyUIWindowBase::Cls_OnDpiChanged(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	if (!g_BrowserGlobalVar.FunctionFlag.bEnableHignDpi)
	{
		return 0;
	}

	if (LOWORD(wParam) != HIWORD(wParam)) {
		NOTIMPLEMENTED() << "Received non-square scaling factors";
		return 0;
	}

	// Scale factor for the new display.
	const float display_scale_factor =
		static_cast<float>(LOWORD(wParam)) / DPI_1X;

	device_scale_factor_ = display_scale_factor;

	DpiChangeWork();

	// Suggested size and position of the current window scaled for the new DPI.
	const RECT* rect = reinterpret_cast<RECT*>(lParam);
	SetWindowPos(nullptr, rect->left, rect->top, rect->right - rect->left,
		rect->bottom - rect->top, SWP_NOZORDER);



	return 1;
}

void EasyUIWindowBase::Cls_OnMove(HWND hwnd, int x, int y)
{
	if (m_browser)
		m_browser->GetHost()->NotifyMoveOrResizeStarted();
}

BOOL EasyUIWindowBase::Cls_OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
	device_scale_factor_ = GetWindowScaleFactor(hwnd);

	if (!called_enable_non_client_dpi_scaling_ && IsProcessPerMonitorDpiAware()) {
		// This call gets Windows to scale the non-client area when WM_DPICHANGED
		// is fired on Windows versions < 10.0.14393.0.
		// Derived signature; not available in headers.
		typedef LRESULT(WINAPI* EnableChildWindowDpiMessagePtr)(HWND, BOOL);
		static EnableChildWindowDpiMessagePtr func_ptr =
			reinterpret_cast<EnableChildWindowDpiMessagePtr>(GetProcAddress(
				GetModuleHandle(L"user32.dll"), "EnableChildWindowDpiMessage"));
		if (func_ptr)
			func_ptr(hwnd, TRUE);
	}

	return TRUE;
}

BOOL EasyUIWindowBase::Cls_OnNCCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
	if (IsProcessPerMonitorDpiAware()) {
		// This call gets Windows to scale the non-client area when WM_DPICHANGED
		// is fired on Windows versions >= 10.0.14393.0.
		typedef BOOL(WINAPI* EnableNonClientDpiScalingPtr)(HWND);
		static EnableNonClientDpiScalingPtr func_ptr =
			reinterpret_cast<EnableNonClientDpiScalingPtr>(GetProcAddress(
				GetModuleHandle(L"user32.dll"), "EnableNonClientDpiScaling"));
		called_enable_non_client_dpi_scaling_ = !!(func_ptr && func_ptr(hwnd));
	}

	return TRUE;
}


BOOL EasyUIWindowBase::ProcessWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult, DWORD dwMsgMapID)
{
#define MYHANDLE_MSG(message, fn)    \
    case (message): lResult = HANDLE_##message((hWnd), (wParam), (lParam), (fn)); return TRUE


	switch (uMsg)
	{
		MYHANDLE_MSG(WM_NCHITTEST, Cls_OnNCHitTest);
		MYHANDLE_MSG(WM_GETMINMAXINFO, Cls_OnGetMinMaxInfo);
		MYHANDLE_MSG(WM_MOVE, Cls_OnMove);
		MYHANDLE_MSG(WM_CREATE, Cls_OnCreate);
		MYHANDLE_MSG(WM_NCCREATE, Cls_OnNCCreate);
		
		
	case WM_DPICHANGED:
		lResult = Cls_OnDpiChanged(hWnd, wParam, lParam);
		return TRUE;
	case WM_NCCALCSIZE:
		lResult = 0;

		/*if (wParam)
		{
			auto pNcP = (NCCALCSIZE_PARAMS*)lParam;
			RECT aRect;
			RECT bRect;
			RECT bcRect;
			CopyRect(&aRect, &pNcP->rgrc[1]);
			CopyRect(&bRect, &pNcP->rgrc[0]);

			CopyRect(&bcRect, &bRect);
			bcRect.left = bRect.left ;
			bcRect.top = bRect.top;
			bcRect.right = bRect.right;
			bcRect.bottom = bRect.bottom- 1;

			CopyRect(&pNcP->rgrc[0], &bcRect);
			CopyRect(&pNcP->rgrc[1], &bRect);
			CopyRect(&pNcP->rgrc[2], &aRect);
			return TRUE;
		}*/

		return TRUE;
	case WM_ENTERMENULOOP:
		if (!wParam) {
			// Entering the menu loop for the application menu.
			CefSetOSModalLoop(true);
		}
		break;

	case WM_EXITMENULOOP:
		if (!wParam) {
			// Exiting the menu loop for the application menu.
			CefSetOSModalLoop(false);
		}
		break;
	default:
		return FALSE;
	}


	return FALSE;
}


LRESULT EasyOpaqueWindow::OnSize(UINT msg, WPARAM wParam, LPARAM lParam, BOOL& handle)
{
	const int width = LOWORD(lParam);
	const int height = HIWORD(lParam);
	if (m_browser && width && height)
	{
		::SetWindowPos(m_browser->GetHost()->GetWindowHandle(), nullptr, 0, 0, width, height, SWP_NOZORDER | SWP_NOMOVE | SWP_ASYNCWINDOWPOS);
	}

	handle = FALSE;

	return 0;
}

LRESULT EasyOpaqueWindow::OnNcActivate(UINT msg, WPARAM wParam, LPARAM lParam, BOOL& handle)
{
	if (IsSystemWindows7OrOlder())
	{
		handle = TRUE;
		return 0;
	}

	handle = FALSE;

	return 0;
}

LRESULT EasyOpaqueWindow::OnNcPaint(UINT msg, WPARAM wParam, LPARAM lParam, BOOL& handle)
{
	if (IsSystemWindows7OrOlder())
	{
		handle = TRUE;
		return 0;
	}

	handle = FALSE;

	return 0;
}

void EasyOpaqueWindow::SetAlpha(BYTE alpha, bool)
{
	const bool bIsLayered = GetExStyle() & WS_EX_LAYERED;
	if (alpha == 255)
	{
		if (bIsLayered)
			ModifyStyleEx(WS_EX_LAYERED, 0, SWP_FRAMECHANGED);
	}
	else
	{
		if (!bIsLayered)
			ModifyStyleEx(0, WS_EX_LAYERED, SWP_FRAMECHANGED);

		SetLayeredWindowAttributes(m_hWnd, 0, alpha, LWA_ALPHA);
	}
}
