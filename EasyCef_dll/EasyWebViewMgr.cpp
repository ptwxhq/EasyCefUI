#include "pch.h"
#include "EasyWebViewMgr.h"
#include "WebViewControl.h"


wvhandle EasyWebViewMgr::GetNewHandleId()
{
	auto id = ++m_IdGen;
	if (id <= 0)
		m_IdGen = 1;

	while (m_WebViewList.find(id) != m_WebViewList.end())
	{
		id = ++m_IdGen;
	}

	return id;
}

EasyWebViewMgr& EasyWebViewMgr::GetInstance()
{
	static EasyWebViewMgr obj;
	return obj;
}

CefRefPtr<WebViewControl> EasyWebViewMgr::GetItemByHwnd(HWND hWnd)
{
	auto it = m_WebViewHWNDIndex.find(hWnd);
	if (it != m_WebViewHWNDIndex.end())
	{
		auto it2 = m_WebViewList.find(it->second);

		if (it2 != m_WebViewList.end())
		{
			return it2->second;
		}
		else
		{
			m_WebViewHWNDIndex.erase(it);
		}
	}

	for (auto &itAgain : m_WebViewList)
	{
		if (itAgain.second->GetHWND() == hWnd)
		{
			m_WebViewHWNDIndex.insert(std::make_pair(hWnd, itAgain.first));
			return itAgain.second;
		}
	}

	return nullptr;
}

CefRefPtr<WebViewControl> EasyWebViewMgr::GetItemBrowserById(int id)
{
	auto it = m_WebViewIndex.find(id);
	if (it != m_WebViewIndex.end())
	{
		auto it2 = m_WebViewList.find(it->second);

		if (it2 != m_WebViewList.end())
		{
			return it2->second;
		}
		else
		{
			m_WebViewIndex.erase(it);
		}
	}

	for (auto& itAgain : m_WebViewList)
	{
		if (itAgain.second->GetBrowserId() == id)
		{
			m_WebViewIndex.insert(std::make_pair(id, itAgain.first));
			return itAgain.second;
		}
	}

	return nullptr;
}

CefRefPtr<WebViewControl> EasyWebViewMgr::GetItemBrowserByHandle(wvhandle handle)
{
	auto it = m_WebViewList.find(handle);

	if (it != m_WebViewList.end())
		return it->second;

	return nullptr;
}


wvhandle EasyWebViewMgr::CreatePopWebViewControl(HWND hParent, const RECT& rc, LPCWSTR lpszUrl, CefRefPtr<CefClient> clientHandler)
{
	auto id = GetNewHandleId();
	CefRefPtr<WebViewBrowserControl> pItem = new WebViewBrowserControl;

	if (pItem->CreatePopup(id, hParent, rc, lpszUrl, clientHandler))
	{
		m_mutex.lock();
		m_WebViewList.insert(std::make_pair(id, pItem));

		m_mutex.unlock();

		return id;
	}

	return 0;
}

wvhandle EasyWebViewMgr::CreateWebViewControl(HWND hParent, const RECT& rc, LPCWSTR lpszUrl, LPCWSTR lpszCookie, const WebViewExtraAttr* pExt)
{
	auto id = GetNewHandleId();
	CefRefPtr<WebViewBrowserControl> pItem = new WebViewBrowserControl;

	if (pItem->InitBrowser(id, hParent, rc, lpszUrl, lpszCookie, pExt, false))
	{
		m_mutex.lock();
		m_WebViewList.insert(std::make_pair(id, pItem));
	//	m_WebViewIndex.insert(std::make_pair(pItem->GetBrowserId(), id));
	//	m_WebViewHWNDIndex.insert(std::make_pair(pItem->GetHWND(), id));
		m_mutex.unlock();
	}
	else
	{
		id = 0;
	}

	return id;
}

wvhandle EasyWebViewMgr::CreateWebViewUI(HWND hParent, const RECT& rc, LPCWSTR lpszUrl, LPCWSTR lpszCookie, const WebViewExtraAttr* pExt)
{
	auto id = GetNewHandleId();
	CefRefPtr<WebViewControl> pItem/* = nullptr*/;

	bool bUseTransparent = false;
	auto pExtCopy = const_cast<WebViewExtraAttr*>(pExt);
	if (pExt && pExt->transparent && g_BrowserGlobalVar.SupportLayerWindow)
	{
		if (g_BrowserGlobalVar.SupportLayerWindow)
		{
			bUseTransparent = true;
		}
		else
		{
			pExtCopy = new WebViewExtraAttr(*pExt);
			pExtCopy->transparent = false;
		}
	}

	if (bUseTransparent)
	{
		pItem = new WebViewTransparentUIControl;
	}
	else
	{
		pItem = new WebViewOpaqueUIControl;
	}

	bool bSucc = pItem->InitBrowser(id, hParent, rc, lpszUrl, lpszCookie, pExt, false);

	if (pExtCopy != pExt)
	{
		delete pExtCopy;
	}


	if (bSucc)
	{
		m_mutex.lock();
		m_WebViewList.insert(std::make_pair(id, pItem));
	//	m_WebViewIndex.insert(std::make_pair(pItem->GetBrowserId(), id));
	//	m_WebViewHWNDIndex.insert(std::make_pair(pItem->GetHWND(), id));
		m_mutex.unlock();
	}
	else
	{
		//delete pItem;
		id = 0;
	}

	return id;
}

void EasyWebViewMgr::RemoveWebView(wvhandle id)
{
	auto it = m_WebViewList.find(id);

	if (it == m_WebViewList.end() || !it->second)
		return;
	
	m_mutex.lock();

	m_WebViewIndex.erase(it->second->GetBrowserId());

	m_WebViewHWNDIndex.erase(it->second->GetHWND());

	m_WebViewList.erase(it);

	m_mutex.unlock();

}

void EasyWebViewMgr::RemoveWebViewByBrowserId(int id)
{
	auto item = GetItemBrowserById(id);
	if (item)
	{
		m_mutex.lock();
		m_WebViewHWNDIndex.erase(item->GetHWND());
		m_WebViewIndex.erase(id);
		m_WebViewList.erase(item->GetItemHandle());
		m_mutex.unlock();
	}
}

bool EasyWebViewMgr::ForEachDoWork(WEBVIEWMGRWORK work)
{
	const bool bRes = !m_WebViewList.empty();
	for (auto& it : m_WebViewList)
	{
		if (!work(it.second))
			break;
	}

	return bRes;
}

void EasyWebViewMgr::RemoveAllItems()
{
	////这样粗暴清理好像会导致后续调用流程中一个个销毁出问题？
	//m_mutex.lock();
	//m_WebViewList.clear();
	//m_WebViewIndex.clear();
	//m_WebViewHWNDIndex.clear();
	//m_mutex.unlock();

	auto listCopy = m_WebViewList;
	for (auto it = listCopy.begin(); it != listCopy.end(); )
	{
		it->second->CloseBrowser();
		listCopy.erase(it++);
	}
}

void EasyWebViewMgr::AsyncSetIndexInfo(wvhandle handle, int index, HWND hWnd)
{
	auto it = m_WebViewList.find(handle);

	if (it != m_WebViewList.end())
	{
		m_WebViewIndex[index] = handle;
		m_WebViewHWNDIndex[hWnd] = handle;
	}
}
