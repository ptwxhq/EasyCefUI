#pragma once

#include "EasyLayeredWindow.h"
#include <future>

class WebViewControl : public CefBaseRefCounted
{
public:
	struct BrowserInitParams
	{
		bool bRet = false;
		bool bSyncCreate = false;
		wvhandle hWebview = 0;
		HWND hParent = nullptr;
		//const WebViewExtraAttr* pExt = nullptr;
		RECT rc = {};
		CefString url;
		CefString cookie;
		std::promise<void> signal;
		CefRefPtr<CefClient> clientHandler;
		std::unique_ptr<WebViewExtraAttr> pExt;
	};

	virtual bool SetBrowser(CefRefPtr<CefBrowser> browser);

	CefRefPtr<CefBrowser> GetBrowser() {
		return m_browser;
	}

	wvhandle GetItemHandle() {
		return m_itemHandle;
	}

	virtual bool LoadUrl(const CefString& url, bool cleanload = false);

	virtual void GoBack();

	virtual void GoForward();

	virtual void Reload();

	virtual void ReloadIgnoreCache();

	virtual void StopLoad();

	//要求UI线程调用，否则返回false
	virtual bool IsMuteAudio();

	virtual void MuteAudio(bool mute);

	//要求UI线程调用
	virtual double GetZoomLevel();

	virtual void SetZoomLevel(double zoomLevel);

	virtual bool InitBrowser(wvhandle hWebview, HWND hParent, const RECT& rc, const CefString& url, const CefString& cookie, const WebViewExtraAttr* pExt, bool Sync);

	virtual HWND GetHWND();

	virtual int GetBrowserId();

	virtual bool IsUIControl() = 0;

	//这里透明指的是使用cef windowless的窗口
	virtual bool IsTransparentUI() = 0;

	virtual void CloseBrowser();

protected:
	virtual void InitBrowserImpl(std::shared_ptr<BrowserInitParams> pParams) = 0;



	wvhandle m_itemHandle = 0;

	CefRefPtr<CefBrowser> m_browser;

	//实际是EasyClientHandler
	//CefRefPtr<CefClient> m_clientHandler;
};


//如果没有父窗口则作为弹出窗口
class WebViewBrowserControl : public WebViewControl
{
	IMPLEMENT_REFCOUNTING(WebViewBrowserControl);
public:
	void InitBrowserImpl(std::shared_ptr<BrowserInitParams> pParams) override;

	bool IsUIControl() final {
		return false;
	}

	bool IsTransparentUI() final {
		return false;
	}

	bool CreatePopup(wvhandle hWebview, HWND hParent, const RECT& rc, const CefString& url, CefRefPtr<CefClient> clientHandler);

};


class WebViewUIControl : public WebViewControl
{
	IMPLEMENT_REFCOUNTING(WebViewUIControl);
public:
	bool IsUIControl() final {
		return true;
	}

	HWND GetHWND() override;

	void SetDraggableRegion(const std::vector<CefDraggableRegion>& regions);

	bool SetBrowser(CefRefPtr<CefBrowser> browser) override;

	void SetEdgeNcAera(EasyUIWindowBase::HT_INFO ht, const std::vector<RECT>& vecRc);

	void SetAlpha(BYTE alpha);

	bool IsAllowDragFiles() {
		return m_bAllowDragFiles;
	}

protected:
	virtual EasyUIWindowBase* GetWindowPtr() = 0;

	void InitBrowserImpl(std::shared_ptr<BrowserInitParams> pParams) override;

	bool m_bAllowDragFiles = false;

};


class WebViewOpaqueUIControl : public WebViewUIControl
{
	IMPLEMENT_REFCOUNTING(WebViewOpaqueUIControl);
public:
	void InitBrowserImpl(std::shared_ptr<BrowserInitParams> pParams) override;

	bool IsTransparentUI() final {
		return false;
	}
private:

	EasyUIWindowBase* GetWindowPtr() override {
		return m_pWindow.get();
	}

	std::unique_ptr<EasyOpaqueWindow> m_pWindow;
};

class WebViewTransparentUIControl : public WebViewUIControl, public CefRenderHandler
{
	IMPLEMENT_REFCOUNTING(WebViewTransparentUIControl);
public:
	
	bool IsTransparentUI() final {
		return true;
	}

	void InitBrowserImpl(std::shared_ptr<BrowserInitParams> pParams) override;

	void GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) override;

	void OnPaint(CefRefPtr<CefBrowser> browser,
		PaintElementType type,
		const RectList& dirtyRects,
		const void* buffer,
		int width,
		int height) override;

	bool GetScreenPoint(CefRefPtr<CefBrowser> browser,
		int viewX,
		int viewY,
		int& screenX,
		int& screenY) override;

	void OnPopupShow(CefRefPtr<CefBrowser> browser, bool show) override;

	void OnPopupSize(CefRefPtr<CefBrowser> browser, const CefRect& rect) override;

	void OnImeCompositionRangeChanged(CefRefPtr<CefBrowser> browser,
		const CefRange& selected_range,
		const RectList& character_bounds) override;

	bool StartDragging(CefRefPtr<CefBrowser> browser,
		CefRefPtr<CefDragData> drag_data,
		DragOperationsMask allowed_ops,
		int x,
		int y) override;

	void UpdateDragCursor(CefRefPtr<CefBrowser> browser,
		DragOperation operation) override;

	void SetToolTip(const CefString& str);

private:

	EasyUIWindowBase* GetWindowPtr() override {
		return m_pWindow.get();
	}

	std::unique_ptr<EasyLayeredWindow> m_pWindow;
};