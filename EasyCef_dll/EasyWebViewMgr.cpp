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
	
	for (auto it = m_WebViewIndex.begin(); it != m_WebViewIndex.end(); ++it)
	{
		if (it->second.hwnd == hWnd)
		{
			auto it2 = m_WebViewList.find(it->first);

			if (it2 != m_WebViewList.end())
			{
				return it2->second;
			}
			else
			{
				m_WebViewIndex.erase(it);
			}
			break;
		}
	}

	for (auto &itAgain : m_WebViewList)
	{
		if (itAgain.second->GetHWND() == hWnd)
		{
			m_WebViewIndex.insert(std::make_pair(itAgain.second->GetItemHandle(), WVINDEX{ itAgain.second->GetBrowserId(), hWnd }));
			return itAgain.second;
		}
	}

	return nullptr;
}

CefRefPtr<WebViewControl> EasyWebViewMgr::GetItemBrowserById(int id)
{
	for (auto it = m_WebViewIndex.begin(); it != m_WebViewIndex.end(); ++it)
	{
		if (it->second.id == id)
		{
			auto it2 = m_WebViewList.find(it->first);

			if (it2 != m_WebViewList.end())
			{
				return it2->second;
			}
			else
			{
				m_WebViewIndex.erase(it);
			}
			break;
		}
	}

	for (auto& itAgain : m_WebViewList)
	{
		if (itAgain.second->GetBrowserId() == id)
		{
			m_WebViewIndex.insert(std::make_pair(itAgain.second->GetItemHandle(), WVINDEX{ id, itAgain.second->GetHWND() }));
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
	if (pExt && pExt->transparent && g_BrowserGlobalVar.SupportLayerWindow)
	{
		pItem = new WebViewTransparentUIControl;
	}
	else
	{
		pItem = new WebViewOpaqueUIControl;
	}

	bool bSucc = pItem->InitBrowser(id, hParent, rc, lpszUrl, lpszCookie, pExt, false);

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

	m_WebViewIndex.erase(id);

	m_WebViewList.erase(it);

	m_mutex.unlock();

}

void EasyWebViewMgr::RemoveWebViewByBrowserId(int id)
{
	auto item = GetItemBrowserById(id);
	if (item)
	{
		RemoveWebView(item->GetItemHandle());
	}
}

void EasyWebViewMgr::RemoveAllItems()
{
	////这样粗暴清理好像会导致后续调用流程中一个个销毁出问题？
	//m_mutex.lock();
	//m_WebViewList.clear();
	//m_WebViewIndex.clear();
	//m_WebViewHWNDIndex.clear();
	//m_mutex.unlock();


	//按新开的窗口先关闭处理
	std::list<CefRefPtr<WebViewControl>> listCopy;
	for (const auto it : m_WebViewList)
	{
		listCopy.push_front(it.second);
	}

	for (auto it = listCopy.begin(); it != listCopy.end(); )
	{
		(*it)->CloseBrowser();
		listCopy.erase(it++);
	}
}

void EasyWebViewMgr::AsyncSetIndexInfo(wvhandle handle, int index, HWND hWnd)
{
	const auto& it = m_WebViewList.find(handle);

	if (it != m_WebViewList.end())
	{
		m_WebViewIndex[handle] = WVINDEX{ index, hWnd };
	}
}

void EasyWebViewMgr::AddDelayItem(CefRefPtr<WebViewControl> item)
{
	if (item)
	{
		m_DelayCleanList.insert(std::make_pair(item->GetHWND(), item));
	}
}

void EasyWebViewMgr::CleanDelayItem(HWND hWnd)
{
	if (hWnd)
	{
		m_DelayCleanList.erase(hWnd);
	}
	else
	{
		auto it = m_DelayCleanList.begin();
		auto nLastCount = m_DelayCleanList.size();
		while (it != m_DelayCleanList.end())
		{
			//win7下非透明窗口销毁beforeclose时不会进入正常销毁流程？目前看可以在beforeclose结束之后正常退出且不会出现有引用的问题
			if (IsSystemWindows7OrOlder() && it->second->IsUIControl() && !it->second->IsTransparentUI())
			{
				++it;
				continue;
			}

			DestroyWindow(it->second->GetHWND());
			it = m_DelayCleanList.begin();

			auto nNowCount = m_DelayCleanList.size();
			if (nNowCount == nLastCount)
			{
				//如果不崩的情况下防止死循环...
				ASSERT(0 && "Not intend...Crash...");
				m_DelayCleanList.erase(it);
				it = m_DelayCleanList.begin();
			}

			nLastCount = nNowCount;
		}

	}
	
}

bool EasyWebViewMgr::HaveDelayItem()
{
	return !m_DelayCleanList.empty();
}
