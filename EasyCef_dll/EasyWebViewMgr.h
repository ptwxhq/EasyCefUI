﻿#pragma once

#include <unordered_map>
#include <mutex>

class WebViewControl;

typedef std::unordered_map<wvhandle, CefRefPtr<WebViewControl>> WebViewList;
typedef std::unordered_map<int, wvhandle> WebViewIndex;
typedef std::unordered_map<HWND, wvhandle> WebViewHWNDIndex;

typedef std::unordered_map<HWND, CefRefPtr<WebViewControl>> DelayCleanList;

class EasyWebViewMgr
{
	DISALLOW_COPY_AND_ASSIGN(EasyWebViewMgr);
	EasyWebViewMgr() = default;

	wvhandle GetNewHandleId();

public:

	static EasyWebViewMgr& GetInstance();

	CefRefPtr<WebViewControl> GetItemByHwnd(HWND hWnd);
	CefRefPtr<WebViewControl> GetItemBrowserById(int id);
	CefRefPtr<WebViewControl> GetItemBrowserByHandle(wvhandle handle);

	wvhandle CreatePopWebViewControl(HWND hParent, const RECT& rc, LPCWSTR lpszUrl, CefRefPtr<CefClient> clientHandler);
	wvhandle CreateWebViewControl(HWND hParent, const RECT& rc, LPCWSTR lpszUrl, LPCWSTR lpszCookie, const WebViewExtraAttr* pExt);
	wvhandle CreateWebViewUI(HWND hParent, const RECT& rc, LPCWSTR lpszUrl, LPCWSTR lpszCookie, const WebViewExtraAttr* pExt);
	void RemoveWebView(wvhandle id);
	void RemoveWebViewByBrowserId(int id);


	//只有清理时才能调用
	void RemoveAllItems();
	void AsyncSetIndexInfo(wvhandle handle, int index, HWND hWnd);

	void AddDelayItem(CefRefPtr<WebViewControl> item);
	void CleanDelayItem(HWND hWnd);

private:
	wvhandle m_IdGen = 0;
	WebViewList m_WebViewList;
	WebViewIndex m_WebViewIndex;  //OnAfterCreated中添加，为了快速查找
	WebViewHWNDIndex m_WebViewHWNDIndex;
	DelayCleanList m_DelayCleanList;//待清理内容，仅限UI
	std::mutex m_mutex;
};

