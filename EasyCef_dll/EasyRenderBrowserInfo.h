#pragma once

#include <unordered_map>


class EasyRenderBrowserInfo 
{

public:

	struct BrsData
	{
		enum TYPE
		{
			BROWSER_NONE = 0,
			BROWSER_UI = 1,
			BROWSER_WEB = 2,
			BROWSER_ALL = BROWSER_UI | BROWSER_WEB
		} type = BROWSER_NONE;
		HWND hUIWnd = nullptr;
		CefRefPtr<CefBrowser> browser;

		BrsData(TYPE t, CefRefPtr<CefBrowser> b, HWND h = nullptr) :type(t), browser(b), hUIWnd(h) {}
	};



	static EasyRenderBrowserInfo& GetInstance() {
		static EasyRenderBrowserInfo obj;
		return obj;
	}

	void AddBrowser(int id, CefRefPtr<CefBrowser> browser, EasyRenderBrowserInfo::BrsData::TYPE type, HWND hwnd = nullptr);
	bool RemoveBrowser(int id);

	BrsData::TYPE GetType(int id) const;

	CefRefPtr<CefBrowser> GetBrowser(int id);

	HWND GetHwnd(int id) const;

private:
	DISALLOW_COPY_AND_ASSIGN(EasyRenderBrowserInfo);
	EasyRenderBrowserInfo() = default;
	std::unordered_map<int, std::unique_ptr<BrsData>> m_mapBrowsers;

};

