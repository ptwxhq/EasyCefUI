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
		} type = BROWSER_NONE;
		CefRefPtr<CefBrowser> browser;
		HWND hUIWnd = nullptr;

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
	MYDISALLOW_COPY_AND_ASSIGN(EasyRenderBrowserInfo);
	EasyRenderBrowserInfo() = default;
	std::unordered_map<int, std::unique_ptr<BrsData>> m_mapBrowsers;

};

