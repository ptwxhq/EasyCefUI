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
		const WebViewExtraAttr* pExt = nullptr;
		RECT rc = {};
		CefString url;
		CefString cookie;
		std::promise<void> signal;
		CefRefPtr<CefClient> clientHandler;
	};

	bool SetBrowser(CefRefPtr<CefBrowser> browser);

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

	virtual void CloseBrowser();

protected:
	virtual void InitBrowserImpl(std::shared_ptr<BrowserInitParams> pParams) = 0;



	wvhandle m_itemHandle = 0;

	CefRefPtr<CefBrowser> m_browser;

	//实际是EasyClientHandler
	CefRefPtr<CefClient> m_clientHandler;
};


//如果没有父窗口则作为弹出窗口
class WebViewBrowserControl : public WebViewControl
{
	IMPLEMENT_REFCOUNTING(WebViewBrowserControl);
public:
	void InitBrowserImpl(std::shared_ptr<BrowserInitParams> pParams) override;

	bool IsUIControl() override {
		return false;
	}

	bool CreatePopup(wvhandle hWebview, HWND hParent, const RECT& rc, const CefString& url, CefRefPtr<CefClient> clientHandler);

};

class WebViewOpaqueUIControl : public WebViewBrowserControl
{
	IMPLEMENT_REFCOUNTING(WebViewOpaqueUIControl);
public:
	bool IsUIControl() override {
		return true;
	}
};

class WebViewTransparentUIControl : public WebViewControl
{
	friend class EasyClientHandler;
	IMPLEMENT_REFCOUNTING(WebViewTransparentUIControl);
public:
	WebViewTransparentUIControl() = default;
	void InitBrowserImpl(std::shared_ptr<BrowserInitParams> pParams) override;

	bool IsUIControl() override {
		return true;
	}

	virtual HWND GetHWND() override;

//private:
	std::unique_ptr<EasyLayeredWindow> m_pWindow;
};