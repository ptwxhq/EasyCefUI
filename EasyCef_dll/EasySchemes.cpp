#include "pch.h"
#include "EasySchemes.h"
#include <fstream>

#include "extlib/pack.h"
#pragma comment(lib, "packlib.lib")

using namespace std;


void EasyRegisterCustomSchemes(CefRawPtr<CefSchemeRegistrar> registrar)
{
	registrar->AddCustomScheme("xpack", CEF_SCHEME_OPTION_LOCAL | CEF_SCHEME_OPTION_CORS_ENABLED /*| CEF_SCHEME_OPTION_CSP_BYPASSING*/);

	registrar->AddCustomScheme(EASYCEFSCHEMES, CEF_SCHEME_OPTION_DISPLAY_ISOLATED);

//	registrar->AddCustomScheme("file", CEF_SCHEME_OPTION_LOCAL);

	//其他值： CEF_SCHEME_OPTION_CSP_BYPASSING	 CEF_SCHEME_OPTION_FETCH_ENABLED
	//registrar->AddCustomScheme("xpack", CEF_SCHEME_OPTION_LOCAL| CEF_SCHEME_OPTION_SECURE| CEF_SCHEME_OPTION_CORS_ENABLED);
}

void RegEasyCefSchemes()
{
	CefRefPtr<EasySchemesHandlerFactory> factory = new EasySchemesHandlerFactory;
	CefRegisterSchemeHandlerFactory("xpack", "", factory);
	CefRegisterSchemeHandlerFactory("http", "ui.pack", factory);

	//为了让xpack可以读取file协议，只能自己重写实现读取并注册了
	CefRegisterSchemeHandlerFactory("file", "", factory);
	CefRegisterSchemeHandlerFactory("disk", "", factory);

	CefRegisterSchemeHandlerFactory(EASYCEFSCHEMES, "", factory);

}

CefRefPtr<CefResourceHandler> EasySchemesHandlerFactory::Create(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString& scheme_name, CefRefPtr<CefRequest> request)
{
	CEF_REQUIRE_IO_THREAD();

	auto type = EasyResourceHandler::RESTYPE::UNKNOWN;

	if (scheme_name.compare("xpack") == 0 || scheme_name.compare("http") == 0)
	{
		type = EasyResourceHandler::RESTYPE::XPACK;
	}
	else if (scheme_name.compare("file") == 0 || scheme_name.compare("disk") == 0)
	{
		type = EasyResourceHandler::RESTYPE::FILE;
	}
	else if (scheme_name.compare(EASYCEFSCHEMES) == 0)
	{
		type = EasyResourceHandler::RESTYPE::INTERNALUI;
	}
	

	return new EasyResourceHandler(type);
}

bool EasyResourceHandler::Open(CefRefPtr<CefRequest> request, bool& handle_request, CefRefPtr<CefCallback> callback)
{
	DCHECK(!CefCurrentlyOn(TID_UI) && !CefCurrentlyOn(TID_IO));

	handle_request = true;

	bool handled = false;

	//如果没有被注册为正常域名则以下将无法解析，故不能使用下面的，得自己解析
	//CefURLParts url_parts;
	//if (!CefParseURL(request->GetURL(), url_parts))
	//{
	//	return false;
	//}

	DomainPackInfo::Uri url_parts(request->GetURL());

	const std::wstring strDecodedPath = CefURIDecode(url_parts.Path_, false, (cef_uri_unescape_rule_t)(UU_PATH_SEPARATORS | UU_SPACES | UU_URL_SPECIAL_CHARS_EXCEPT_PATH_SEPARATORS));


	enum class STATUSCODE {
		E_UNSET,
		E400 = 400,
		E403 = 403,
		E404 = 404
	};

	STATUSCODE sCurrent = STATUSCODE::E_UNSET;

	do 
	{
		handled = true;

		bool bHaveFile = false;

		switch (m_type)
		{
		case EasyResourceHandler::RESTYPE::UNKNOWN:
			break;
		case EasyResourceHandler::RESTYPE::XPACK:
			{
				//检查域名是否已注册，无效返回400
				auto strPackFilePath = DomainPackInfo::GetInstance().GetDomainPath(url_parts.Host_.c_str());
				if (strPackFilePath.empty())
				{
					sCurrent = STATUSCODE::E400;
					break;
				}

				std::wstring strPathInZip = strDecodedPath;

				if (strPathInZip.empty())
				{
					sCurrent = STATUSCODE::E404;
					break;
				}

				//首页未定义的情况，自动标记为index.html
				if (strPathInZip == L"/")
				{
					strPathInZip = LR"(\index.html)";
				}

				std::replace(strPathInZip.begin(), strPathInZip.end(), L'/', L'\\');

				//因路径本身错误导致的不进行修正，直接判断

				XPackData *pPackData;
				int packstatus = UnzipExistPackFile(strPackFilePath.c_str(), strPathInZip.c_str(), &pPackData);

				if (0 == packstatus)
				{
					data_.assign(*pPackData, pPackData->GetLen());
					pPackData->Free();
					DCHECK(!data_.empty());
					bHaveFile = true;
				}
				else if (-1 == packstatus)
				{
					unsigned char* databuf = 0;
					unsigned long data_len = 0;
					if (exZipFile(strPackFilePath.c_str(), strPathInZip.c_str(), &databuf, &data_len)) {
						data_.assign(reinterpret_cast<char*>(databuf), data_len);
						freeBuf(databuf);
						DCHECK(!data_.empty());
						bHaveFile = true;
					}
					else
					{
						sCurrent = STATUSCODE::E404;
						break;
					}
				}
				else
				{
					sCurrent = STATUSCODE::E404;
					break;
				}
			
			}
			break;
		case EasyResourceHandler::RESTYPE::FILE:
			{
				std::wstring strPath = strDecodedPath;
				
				std::replace(strPath.begin(), strPath.end(), L'/', L'\\');
				if (strPath.front() == '\\')
					strPath.erase(0, 1);
				
				if (!PathFileExistsW(strPath.c_str()))
				{
					sCurrent = STATUSCODE::E404;
					break;
				}

				std::ifstream fs;
				fs.open(strPath, std::ios::binary | std::ios::ate);
				if (!fs.is_open())
				{
					sCurrent = STATUSCODE::E403;
					break;
				}

				//这边懒得处理文件过大的情况了，直接读取

				const auto size = fs.tellg();
				fs.seekg(std::ios::beg);
				data_.resize(size);
				fs.read(data_.data(), size);

				bHaveFile = true;
			}
			break;
		case EasyResourceHandler::RESTYPE::INTERNALUI:
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
						data_.resize(binhtml->GetSize());
						binhtml->GetData(data_.data(), binhtml->GetSize(), 0);

						bHaveFile = true;
					}
		
				}
			}
			break;
		default:
			break;
		}

		switch (sCurrent)
		{
		case STATUSCODE::E_UNSET:
			break;
		case STATUSCODE::E400:
			data_ = "invalid xpack host";
			break;
		case STATUSCODE::E403:
			data_ = "cannot open file";
			break;
		case STATUSCODE::E404:
			data_ = "404 file not exist";

			break;
		default:
			break;
		}

		if (!bHaveFile)
		{
			data_ = webinfo::GetErrorPage("", data_);
			statuscode_ = (int)sCurrent;
			mime_type_ = "text/html";
			break;
		}

		//处理
		std::string ansipath = CefString(url_parts.Path_);
		const size_t sep = ansipath.find_last_of(".");
		mime_type_ = CefGetMimeType(ansipath.substr(sep + 1)).ToString();
		if (mime_type_.empty())
		{
			mime_type_ = "text/html";
		}

		statuscode_ = 200;
		break;
	}while (false);

	return handled;
}

void EasyResourceHandler::GetResponseHeaders(CefRefPtr<CefResponse> response, int64& response_length, CefString& redirectUrl)
{
	CEF_REQUIRE_IO_THREAD();

	DCHECK(!data_.empty());

	response->SetMimeType(mime_type_);
	response->SetStatus(statuscode_);

	// Set the resulting response length
	response_length = data_.length();
}

bool EasyResourceHandler::Read(void* data_out, int bytes_to_read, int& bytes_read, CefRefPtr<CefResourceReadCallback> callback)
{
	DCHECK(!CefCurrentlyOn(TID_UI) && !CefCurrentlyOn(TID_IO));

	bool has_data = false;
	bytes_read = 0;

	if (offset_ < data_.length()) {
		// Copy the next block of data into the buffer.
		int transfer_size =
			std::min(bytes_to_read, static_cast<int>(data_.length() - offset_));
		memcpy(data_out, data_.c_str() + offset_, transfer_size);
		offset_ += transfer_size;

		bytes_read = transfer_size;
		has_data = true;
	}

	return has_data;
}

void EasyResourceHandler::Cancel()
{
	CEF_REQUIRE_IO_THREAD();
}

bool DomainPackInfo::RegisterPackDomain(LPCWSTR lpszDomain, LPCWSTR lpszFilePath)
{
	if (!lpszFilePath || !lpszFilePath[0])
		return false;

	auto Domain = GetFormatedDomain(lpszDomain);

	if (!Domain.empty())
	{
		m_DomainMap.insert(std::make_pair(std::move(Domain), lpszFilePath));
		PreLoadPackFile(lpszFilePath);
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
			CleanLoadedPacks(it->second.c_str());
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
