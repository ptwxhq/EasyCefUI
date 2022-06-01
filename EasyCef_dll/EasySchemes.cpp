﻿#include "pch.h"
#include "EasySchemes.h"
#include <fstream>
#include <algorithm>
#include "include/wrapper/cef_stream_resource_handler.h"

CefRefPtr<CefResourceManager> g_resource_manager;
EasyMemoryFileMgr g_MemFileMgr;

void EasyRegisterCustomSchemes(CefRawPtr<CefSchemeRegistrar> registrar)
{
	//xpack改为CEF_SCHEME_OPTION_STANDARD之后没有禁用安全设置的话不允许直接读取file协议
	registrar->AddCustomScheme(PACKSCHEME, CEF_SCHEME_OPTION_STANDARD | CEF_SCHEME_OPTION_CORS_ENABLED | CEF_SCHEME_OPTION_CSP_BYPASSING | CEF_SCHEME_OPTION_FETCH_ENABLED);

	registrar->AddCustomScheme(EASYCEFSCHEME, CEF_SCHEME_OPTION_DISPLAY_ISOLATED);
}

void RegEasyCefSchemes()
{
	CefRefPtr<EasySchemesHandlerFactory> factory = new EasySchemesHandlerFactory;
	CefRegisterSchemeHandlerFactory(PACKSCHEME, "", factory);
	CefRegisterSchemeHandlerFactory("http", "ui.pack", factory);

	//为了让xpack可以读取file协议，只能自己重写实现读取并注册了
	CefRegisterSchemeHandlerFactory("file", "", factory);
	CefRegisterSchemeHandlerFactory("disk", "", factory);
	CefRegisterSchemeHandlerFactory("memory", "", factory);

	CefRegisterSchemeHandlerFactory("http", "todisk", factory);

	CefRegisterSchemeHandlerFactory(EASYCEFSCHEME, "", factory);

}

class SaveFileToDiskProvider : public CefResourceManager::Provider {

	#define ToDiskUrl "http://todisk/"
	const int LenOfUrl = _countof(ToDiskUrl) - 1;
public:

	SaveFileToDiskProvider() = default;

	bool OnRequest(scoped_refptr<CefResourceManager::Request> request) override {
		CEF_REQUIRE_IO_THREAD();

		const std::string& url = request->url();
		const int len = LenOfUrl;
		if (url.compare(0, len, ToDiskUrl) != 0) {
			return false;
		}

		std::string status;

		CefResponse::HeaderMap response_headers;
		response_headers.insert(std::make_pair("Access-Control-Allow-Origin", "*"));

		int code = 400;
		auto response = SaveContents(request->request(), code, status);

		request->Continue(new CefStreamResourceHandler(code, status, "text/plain; charset=utf-8",
			response_headers, response));

		return true;
	}

private:
	CefRefPtr<CefStreamReader> SaveContents(CefRefPtr<CefRequest> request, int& code, std::string& status)
	{
		code = 403;
		status = "Path Invalid";

		auto url = request->GetURL().ToWString();
		auto encodedpath = url.substr(LenOfUrl);

		std::wstring localpath = CefURIDecode(encodedpath, false, (cef_uri_unescape_rule_t)(UU_PATH_SEPARATORS | UU_SPACES | UU_URL_SPECIAL_CHARS_EXCEPT_PATH_SEPARATORS));

		std::replace(localpath.begin(), localpath.end(), L'/', L'\\');

		do
		{
			if (PathIsRelativeW(localpath.c_str()))
			{
				break;
			}

			if (_waccess_s(localpath.c_str(), 0) == 0)
			{
				status = "File is exist";
				break;
			}

			status = "No post data";
			CefRefPtr<CefPostData> postData = request->GetPostData();
			if (!postData)
				break;
			
			CefPostData::ElementVector elements;
			postData->GetElements(elements);
			if (elements.size() == 0)
				break;
			
			for (auto element : elements)
			{
				auto type = element->GetType();

				if (element->GetType() == PDE_TYPE_BYTES) {

					auto write = CefStreamWriter::CreateForFile(localpath);
					if (write)
					{
						size_t size = element->GetBytesCount();
						char* bytes = new char[size];
						element->GetBytes(size, bytes);
						auto written = write->Write(bytes, size, 1);
						delete[] bytes;

						if (written == 1)
						{
							code = 200;
							status = "OK";
						}
						else
						{
							code = 500;
							status = "Write size error";
						}
					}
					else
					{
						code = 500;
						status = "Fail to create file. errno:" + std::to_string(errno);
					}

				}
			}

		} while (false);


		return CefStreamReader::CreateForData(static_cast<void*>(const_cast<char*>(status.c_str())), status.size());
	}

	MYDISALLOW_COPY_AND_ASSIGN(SaveFileToDiskProvider);
};

EasySchemesHandlerFactory::EasySchemesHandlerFactory()
{
	mapSchemes.insert(std::make_pair(PACKSCHEME, RESTYPE::XPACK));
	mapSchemes.insert(std::make_pair("http", RESTYPE::FAKEHTTP));
	mapSchemes.insert(std::make_pair("file", RESTYPE::FILE));
	mapSchemes.insert(std::make_pair("disk", RESTYPE::FILE));
	mapSchemes.insert(std::make_pair("memory", RESTYPE::MEMORY));
	mapSchemes.insert(std::make_pair(EASYCEFSCHEME, RESTYPE::INTERNALUI));


	g_resource_manager = new CefResourceManager();

	g_resource_manager->AddProvider(new SaveFileToDiskProvider, 0, std::string());
}

CefRefPtr<CefResourceHandler> EasySchemesHandlerFactory::Create(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString& scheme_name, CefRefPtr<CefRequest> request)
{
	CEF_REQUIRE_IO_THREAD();

	//LOG(INFO) << GetCurrentProcessId() << "] EasySchemesHandlerFactory url:" << request->GetURL() << "\n";

	auto type = RESTYPE::UNKNOWN;

	auto it = mapSchemes.find(scheme_name.ToString());
	if (it != mapSchemes.end())
	{
		type = it->second;
	}

	DomainPackInfo::Uri url_parts(request->GetURL());

#ifdef _DEBUG

	if (url_parts.Path_.empty())
	{
		url_parts.Path_ = L" ";
	}

#endif

	enum class STATUSCODE : int {
		E_UNSET,
		E200 = 200,
		E400 = 400,
		E404 = 404,
	};

	std::string strStatusText = "UNDEFINE";

	STATUSCODE sCurrent = STATUSCODE::E_UNSET;

	CefRefPtr<CefStreamReader> stream;

	CefResponse::HeaderMap header;

	const std::wstring strDecodedPath = CefURIDecode(url_parts.Path_, false, (cef_uri_unescape_rule_t)(UU_PATH_SEPARATORS | UU_SPACES | UU_URL_SPECIAL_CHARS_EXCEPT_PATH_SEPARATORS));

	std::string ansipath = CefString(url_parts.Path_);
	const size_t sep = ansipath.find_last_of(".");
	auto mime_type_ = CefGetMimeType(ansipath.substr(sep + 1)).ToString();
	if (mime_type_.empty())
	{
		mime_type_ = MIME_HTML;
	}


	switch (type)
	{
	case RESTYPE::UNKNOWN:
		break;
	case RESTYPE::XPACK:
	case RESTYPE::FAKEHTTP:
		{
			//检查域名是否已注册，无效返回400
			auto strPackFilePath = DomainPackInfo::GetInstance().GetDomainPath(url_parts.Host_.c_str());
			if (!g_BrowserGlobalVar.funcXpackExtract || strPackFilePath.empty())
			{
				sCurrent = STATUSCODE::E400;
				strStatusText = "UNREGISTERED";
				break;
			}

			if (strDecodedPath.empty())
			{
				sCurrent = STATUSCODE::E404;
				break;
			}

			//过滤?和#
			std::wstring strPathInZip = GetUrlWithoutQueryOrFragment(strDecodedPath);

			std::replace(strPathInZip.begin(), strPathInZip.end(), L'/', L'\\');

			//尾部未定义的情况，自动标记为index.html
			if (strPathInZip.back() == '\\')
			{
				strPathInZip += L"index.html";
			}

			//因路径本身错误导致的不进行修正，直接判断
			BYTE* pData = nullptr;
			DWORD dwSize = 0;
			if (g_BrowserGlobalVar.funcXpackExtract(strPackFilePath.c_str(), strPathInZip.c_str(), &pData, &dwSize))
			{
				stream = CefStreamReader::CreateForData(pData, dwSize);
				g_BrowserGlobalVar.funcXpackFreeData(pData);
				sCurrent = STATUSCODE::E200;
			}
			else
			{
				sCurrent = STATUSCODE::E404;
			}
		}
		break;
	case RESTYPE::FILE:
		{
			std::wstring strPath = GetUrlWithoutQueryOrFragment(strDecodedPath);

			std::replace(strPath.begin(), strPath.end(), L'/', L'\\');
			if (strPath.front() == '\\')
				strPath.erase(0, 1);

			stream = CefStreamReader::CreateForFile(strPath);
			if (!stream)
			{
				sCurrent = STATUSCODE::E404;
				break;
			}

			sCurrent = STATUSCODE::E200;
		}
		break;
	case RESTYPE::INTERNALUI:
	{
		if (url_parts.Host_ == L"info")
		{
			std::wstring strData = strDecodedPath;
			if (strData[0] == '/')
			{
				strData.erase(0, 1);
			}

			auto binhtml = CefBase64Decode(strData);
			if (binhtml)
			{
				auto pBuf = std::make_unique<char[]>(binhtml->GetSize());
				binhtml->GetData(pBuf.get(), binhtml->GetSize(), 0);
				stream = CefStreamReader::CreateForData(pBuf.get(), binhtml->GetSize());
			}
			else
			{
				//异常内容

			}

			sCurrent = STATUSCODE::E200;
		}
	}
	break;
	case RESTYPE::MEMORY:
	{
		std::wstring strUrl = GetUrlWithoutQueryOrFragment(strDecodedPath);
		std::string data;
		if (g_MemFileMgr.GetDataByUrl(strUrl, data))
		{
			sCurrent = STATUSCODE::E200;
			stream = CefStreamReader::CreateForData(data.data(), data.size());
		}
		else
		{
			sCurrent = STATUSCODE::E404;
		}
		
	}
	break;
	//case RESTYPE::TODISK:
	//	{
	//		std::wstring strPath = GetUrlWithoutQueryOrFragment(strDecodedPath);

	//		std::replace(strPath.begin(), strPath.end(), L'/', L'\\');
	//		if (strPath.front() == '\\')
	//			strPath.erase(0, 1);

	//		if (request->GetResourceType() == RT_XHR)
	//		{
	//			int a;
	//				a = 1;
	//		}

	//		//为了安全可以过滤可执行文件
	//		sCurrent = STATUSCODE::E200;
	//		header.insert(std::make_pair("Access-Control-Allow-Origin", "*") );
	//		assert(request->GetPostData());

	//		CefResponse::HeaderMap response_headers;			
	//		GetDumpResponse(request, response_headers);

	//	}
	//	break;

	default:
		break;
	}

	if (!stream && sCurrent == STATUSCODE::E_UNSET)
	{
		sCurrent = STATUSCODE::E404;
	}

	std::string ErrorInfo = "Unknow";
	switch (sCurrent)
	{
	case STATUSCODE::E_UNSET:
		assert(0);
		break;
	case STATUSCODE::E404:
		strStatusText = "NOT FOUND";
		ErrorInfo = "404 file " + CefString(url_parts.Path_).ToString() + " not exist";

		break;
	default:
		break;
	}

	if (!stream)
	{
		mime_type_ = MIME_HTML;
		auto data = webinfo::GetErrorPage("", ErrorInfo);
		stream = CefStreamReader::CreateForData(data.data(), data.size());
	}

	return new CefStreamResourceHandler(static_cast<int>(sCurrent), strStatusText, mime_type_, header, stream);
}

bool DomainPackInfo::RegisterPackDomain(LPCWSTR lpszDomain, LPCWSTR lpszFilePath)
{
	if (!lpszFilePath || !lpszFilePath[0])
		return false;

	auto Domain = GetFormatedDomain(lpszDomain);

	if (!Domain.empty())
	{
		m_DomainMap.insert(std::make_pair(std::move(Domain), lpszFilePath));

		return true;
	}

	return false;
}

void DomainPackInfo::UnregisterPackDomain(LPCWSTR lpszDomain)
{
	auto Domain = GetFormatedDomain(lpszDomain);

	if (!Domain.empty())
	{
		auto it = m_DomainMap.find(Domain);
		if (it != m_DomainMap.end())
		{
			m_DomainMap.erase(it);
		}

	}
}

std::wstring DomainPackInfo::GetDomainPath(LPCWSTR lpszDomain)
{
	auto Domain = GetFormatedDomain(lpszDomain);

	//当作ui.pack
	if (Domain.empty())
	{
		return L"";
	}

	auto it = m_DomainMap.find(Domain);
	if (it != m_DomainMap.end())
	{
		return it->second;
	}

	if (!wcscmp(lpszDomain, L"ui.pack"))
	{
		return g_BrowserGlobalVar.FileDir + L"ui.pack";
	}

	return L"";
}

DomainPackInfo& DomainPackInfo::GetInstance()
{
	static DomainPackInfo obj;
	return obj;
}

std::wstring DomainPackInfo::GetFormatedDomain(LPCWSTR lpszDomain)
{
	if (!lpszDomain || !lpszDomain[0])
	{
		return L"";
	}

	Uri url(lpszDomain);

	return std::move(url.Host_);
}

void DomainPackInfo::Uri::Parse(const std::wstring& uri)
{
	if (uri.length() == 0)
		return;

	const auto uriEnd = uri.end();

	// get query start
	const auto queryStart = std::find(uri.begin(), uriEnd, L'?');

	// protocol
	const auto protocolStart = uri.begin();
	auto protocolEnd = std::find(protocolStart, uriEnd, L':');            //"://");

	if (protocolEnd != uriEnd)
	{
		std::wstring prot = &*(protocolEnd);
		if ((prot.length() > 3) && (prot.substr(0, 3) == L"://"))
		{
			Protocol_ = std::wstring(protocolStart, protocolEnd);
			protocolEnd += 3;   //      ://
		}
		else
			protocolEnd = uri.begin();  // no protocol
	}
	else
		protocolEnd = uri.begin();  // no protocol

	transform(Protocol_.begin(), Protocol_.end(), Protocol_.begin(), tolower);

	// host
	auto hostStart = protocolEnd;
	auto pathStart = std::find(hostStart, uriEnd, L'/');  // get pathStart

	auto hostEnd = std::find(protocolEnd,
		(pathStart != uriEnd) ? pathStart : queryStart,
		L':');  // check for port

	Host_.assign(hostStart, hostEnd);
	transform(Host_.begin(), Host_.end(), Host_.begin(), tolower);

	// port
	if ((hostEnd != uriEnd) && ((&*(hostEnd))[0] == L':'))  // we have a port
	{
		hostEnd++;
		auto portEnd = (pathStart != uriEnd) ? pathStart : queryStart;
		Port_.assign(hostEnd, portEnd);
	}

	// path
	if (pathStart != uriEnd)
		Path_.assign(pathStart, queryStart);

	// query
	if (queryStart != uriEnd)
		QueryString_.assign(queryStart, uri.end());

}

bool EasyMemoryFileMgr::AddMemoryFile(const void* pData, unsigned int nDataLen, size_t& id, LPCWSTR lpszDomain)
{
	EasyMemoryFile file;
	file.data.assign(static_cast<const char*>(pData), nDataLen);
	if (file.data.empty())
		return false;

	std::wstring strUrl = L"memory://";
	if (lpszDomain && lpszDomain[0])
	{
		DomainPackInfo::Uri url_parts(lpszDomain);
		strUrl += url_parts.Host_;
	}
	else
	{
		strUrl += L"localhost";
	}

	strUrl += L"/";
	strUrl += CefString(GetRandomString(32));

	file.url = strUrl;

	std::hash<std::wstring> hash;
	file.id = hash(strUrl);

	mapUserData.insert(std::make_pair(file.id, file));

	id = file.id;
	return true;
}

void EasyMemoryFileMgr::DelMemoryFile(size_t id)
{
	mapUserData.erase(id);
}

bool EasyMemoryFileMgr::GetMemoryFileUrl(size_t id, CefString& lpszUrl)
{
	auto it = mapUserData.find(id);
	if (it != mapUserData.end())
	{
		lpszUrl = it->second.url;
		return true;
	}

	return false;
}

bool EasyMemoryFileMgr::GetDataByUrl(const CefString& strUrl, std::string& data)
{
	std::hash<std::wstring> hash;
	auto id = hash(strUrl.ToWString());

	auto it = mapUserData.find(id);
	if (it != mapUserData.end())
	{
		data = it->second.data;
	}

	return false;
}
