﻿#pragma once

#include "include/cef_scheme.h"
#include <unordered_map>
#include <include/wrapper/cef_resource_manager.h>
#include <mutex>


struct EasyMemoryFile
{
	size_t id = 0;
	CefString url;
	std::string data;
};

class EasyMemoryFileMgr
{
	std::mutex m_mutex;
	std::unordered_map<size_t, EasyMemoryFile> mapUserData;
public:
	bool AddMemoryFile(const void* pData, unsigned int nDataLen, size_t& id, LPCWSTR lpszDomain, LPCWSTR lpszExtName);
	void DelMemoryFile(size_t id);
	bool GetMemoryFileUrl(size_t id, CefString& lpszUrl);
	bool GetDataByUrl(const CefString& strUrl, std::string& data);
	bool GetData(size_t id, std::string& data);
};

class EasySchemesHandlerFactory : public CefSchemeHandlerFactory
{
	enum class RESTYPE
	{
		UNKNOWN,
		XPACK,
		FILE,
		INTERNALUI,
		FAKEHTTP,//XPACK
		MEMORY,
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
	class Uri
	{
		std::wstring QueryString_, Path_, Protocol_, Host_, Port_;
	public:

		Uri() = default;
		Uri(const std::wstring& uri) { Parse(uri); }

		void Parse(const std::wstring& uri);

		std::wstring Formated();

		const std::wstring& Path() const {
			return Path_;
		}

		const std::wstring& Host() const {
			return Host_;
		}

		const std::wstring& Protocol() const {
			return Protocol_;
		}
	};


	bool RegisterPackDomain(LPCWSTR lpszDomain, LPCWSTR lpszFilePath);
	void UnregisterPackDomain(LPCWSTR lpszDomain);

	//如果域名格式是无效的则返回ui.pack，有效格式则返回具体值
	std::wstring GetDomainPath(LPCWSTR lpszDomain);
	static DomainPackInfo& GetInstance();
	static std::wstring GetFormatedDomain(LPCWSTR lpszDomain);
private:
	MYDISALLOW_COPY_AND_ASSIGN(DomainPackInfo);
	DomainPackInfo() = default;
	std::unordered_map<std::wstring, std::wstring> m_DomainMap;
};


class SaveFileToDiskProvider : public CefResourceManager::Provider {
public:

	SaveFileToDiskProvider() = default;

	bool OnRequest(scoped_refptr<CefResourceManager::Request> request) override;

private:
	CefRefPtr<CefStreamReader> SaveContents(CefRefPtr<CefRequest> request, int& code, std::string& status);

	MYDISALLOW_COPY_AND_ASSIGN(SaveFileToDiskProvider);
};


void EasyRegisterCustomSchemes(CefRawPtr<CefSchemeRegistrar> registrar);

void RegEasyCefSchemes(CefRefPtr<CefRequestContext> requestContext);