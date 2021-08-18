#include "pch.h"
#include "EasyRenderBrowserInfo.h"
#include "EasyRenderWorks.h"



void EasyRenderBrowserInfo::AddBrowser(int id, CefRefPtr<CefBrowser> browser, EasyRenderBrowserInfo::BrsData::TYPE type)
{
	m_mapBrowsers.insert(std::make_pair(id, std::make_unique<BrsData>(type, browser)));
}

bool EasyRenderBrowserInfo::RemoveBrowser(int id)
{
	auto item = m_mapBrowsers.find(id);
	if (item != m_mapBrowsers.end())
	{
		m_mapBrowsers.erase(item);
		return true;
	}

	return false;
}

EasyRenderBrowserInfo::BrsData::TYPE EasyRenderBrowserInfo::GetType(int id) const
{
	auto item = m_mapBrowsers.find(id);
	if (item != m_mapBrowsers.end())
	{
		return item->second->type;
	}

	return BrsData::BROWSER_NONE;
}

CefRefPtr<CefBrowser> EasyRenderBrowserInfo::GetBrowser(int id)
{
	auto item = m_mapBrowsers.find(id);
	if (item != m_mapBrowsers.end())
	{
		return item->second->browser;
	}

	return nullptr;
}

