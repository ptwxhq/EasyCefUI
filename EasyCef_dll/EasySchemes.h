#pragma once

#include "include/cef_scheme.h"
#include <unordered_map>


class EasySchemesHandlerFactory : public CefSchemeHandlerFactory
{
	enum class RESTYPE
	{
		UNKNOWN,
		XPACK,
		FILE,
		INTERNALUI,
		FAKEHTTP,//XPACK
	};
public:
	EasySchemesHandlerFactory();
	CefRefPtr<CefResourceHandler> Create(
		CefRefPtr<CefBrowser> browser,
		CefRefPtr<CefFrame> frame,
		const CefString& scheme_name,
		CefRefPtr<CefRequest> request) override;

	IMPLEMENT_REFCOUNTING(EasySchemesHandlerFactory);
private:
	std::unordered_map<std::string, RESTYPE> mapSchemes;
};


class DomainPackInfo {
public:
	struct Uri
	{
	public:
		std::wstring QueryString_, Path_, Protocol_, Host_, Port_;

		Uri() = default;
		Uri(const std::wstring& uri) { Parse(uri); }

		void Parse(const std::wstring& uri);
	};


	bool RegisterPackDomain(LPCWSTR lpszDomain, LPCWSTR lpszFilePath);
	void UnregisterPackDomain(LPCWSTR lpszDomain);

	//如果域名格式是无效的则返回ui.pack，有效格式则返回具体值
	std::wstring GetDomainPath(LPCWSTR lpszDomain);
	static DomainPackInfo& GetInstance();
	static std::wstring GetFormatedDomain(LPCWSTR lpszDomain);
private:
	DISALLOW_COPY_AND_ASSIGN(DomainPackInfo);
	DomainPackInfo() = default;
	std::unordered_map<std::wstring, std::wstring> m_DomainMap;
};


void EasyRegisterCustomSchemes(CefRawPtr<CefSchemeRegistrar> registrar);

void RegEasyCefSchemes();