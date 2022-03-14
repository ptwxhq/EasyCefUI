#pragma once

#include "include/cef_scheme.h"
#include <unordered_map>


class EasyResourceHandler : public CefResourceHandler
{
public:

	enum class RESTYPE
	{
		UNKNOWN,
		XPACK,
		FILE,
		INTERNALUI,
//		PROXY,
		FAKEHTTP,//XPACK
	};


	EasyResourceHandler(RESTYPE type) :m_type(type) {
	}

	//代替原有的ProcessRequest
	bool Open(CefRefPtr<CefRequest> request,
		bool& handle_request,
		CefRefPtr<CefCallback> callback) override;

	void GetResponseHeaders(CefRefPtr<CefResponse> response,
		int64& response_length,
		CefString& redirectUrl) override;


	//和Skip一起替代原有ReadResponse
	bool Read(void* data_out,
		int bytes_to_read,
		int& bytes_read,
		CefRefPtr<CefResourceReadCallback> callback) override;

	void Cancel() override;

	IMPLEMENT_REFCOUNTING(EasyResourceHandler);
	DISALLOW_COPY_AND_ASSIGN(EasyResourceHandler);

private:
	RESTYPE m_type;
	int statuscode_ = 0;
	size_t offset_ = 0;
	std::string data_;
	std::string mime_type_;


};


class EasySchemesHandlerFactory : public CefSchemeHandlerFactory
{
public:
	EasySchemesHandlerFactory();
	CefRefPtr<CefResourceHandler> Create(
		CefRefPtr<CefBrowser> browser,
		CefRefPtr<CefFrame> frame,
		const CefString& scheme_name,
		CefRefPtr<CefRequest> request) override;

	IMPLEMENT_REFCOUNTING(EasySchemesHandlerFactory);
private:
	std::unordered_map<std::string, EasyResourceHandler::RESTYPE> mapSchemes;
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