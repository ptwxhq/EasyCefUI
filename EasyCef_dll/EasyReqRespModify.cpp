#include "pch.h"
#include "EasyReqRespModify.h"
#include <regex>
#include "include/base/cef_callback.h"
#include "include/wrapper/cef_closure_task.h"
#include "include/cef_request_context_handler.h"



bool EasyReplaceRule::CheckRegex(int type)
{
	bool bSucc = true;
	try
	{
		if (type == 1)
		{
			if (MatchType == RULE_MATCH_TYPE::REGEX)
			{
				std::regex word_regex(strUrlMathInfo);
			}
			else if (MatchType == RULE_MATCH_TYPE::HOST)
			{
				std::transform(strUrlMathInfo.begin(), strUrlMathInfo.end(), strUrlMathInfo.begin(), tolower);
			}
		}

		if (type == 2 && bReplaceUseRegex)
		{
			std::regex word_regex(strSearch);
		}
		
	}
	catch (const std::exception& e)
	{
		ModifyType = RULE_MODIFY_TYPE::NONE;
		bSucc = false;
	}

	return bSucc;
}

EasyReqRespModifyMgr& EasyReqRespModifyMgr::GetInstance()
{
	static EasyReqRespModifyMgr obj;
	return obj;
}

bool EasyReqRespModifyMgr::CheckMatchUrl(const std::string& strUrl, const std::string& strHost, const EasyReplaceRule& Rule, int RequestType)
{
	if (RequestType > 0)
	{
		switch (Rule.ModifyType)
		{
		case RULE_MODIFY_TYPE::REQUEST_HEAD:
		case RULE_MODIFY_TYPE::REQUEST_POSTDATA:
			if (RequestType != 1)
				return false;
			break;

		case RULE_MODIFY_TYPE::RESPONSE_HEAD:
		case RULE_MODIFY_TYPE::RESPONSE_DATA:
			if (RequestType != 2)
				return false;
			break;
		default:
			return false;
		}
	}

	bool bFound = false;

	switch (Rule.MatchType)
	{
	case RULE_MATCH_TYPE::HOST:
		if (strHost == Rule.strUrlMathInfo)
		{
			bFound = true;
		}
		break;
	case RULE_MATCH_TYPE::KEYWORD:
		if (strUrl.find(Rule.strUrlMathInfo) != std::string::npos)
		{
			bFound = true;
		}
		break;
	case RULE_MATCH_TYPE::REGEX:
		{
			std::regex word_regex(Rule.strUrlMathInfo, std::regex_constants::icase);
			std::smatch m;
			bFound = std::regex_search(strUrl, m, word_regex);
		}
		break;
	default:
		break;
	}

	return bFound;
}

bool EasyReqRespModifyMgr::CheckMatchUrl(const std::string& strUrl, std::vector<RuleID>* pRequestListIds, std::vector<RuleID>* pResponseListIds, int RequestType)
{
	if (pRequestListIds)
		pRequestListIds->clear();
	if (pResponseListIds)
		pResponseListIds->clear();
	if (m_mapRules.size() == 0)
		return false;


	CefURLParts UrlParts;
	CefParseURL(strUrl, UrlParts);

	std::string strHost = CefString(&UrlParts.host);

	for (auto RuleId : m_listRuleOrder)
	{
		auto item = m_mapRules.find(RuleId);
		if (item == m_mapRules.end())
			continue;

		const auto& it = item->second;
		if (CheckMatchUrl(strUrl, strHost, *it, RequestType))
		{
			switch (it->ModifyType)
			{
			case RULE_MODIFY_TYPE::REQUEST_HEAD:
			case RULE_MODIFY_TYPE::REQUEST_POSTDATA:
				pRequestListIds->push_back(RuleId);
				break;

			case RULE_MODIFY_TYPE::RESPONSE_HEAD:
			case RULE_MODIFY_TYPE::RESPONSE_DATA:
				pResponseListIds->push_back(RuleId);
				break;
			default:
				break;
			}
		}

	}

	bool bNeedWork = false;

	if (RequestType == 1)
		bNeedWork = !pRequestListIds->empty();
	else if (RequestType == 2)
		bNeedWork = !pResponseListIds->empty();
	else
		bNeedWork = !pRequestListIds->empty() || !pResponseListIds->empty();

	return bNeedWork;
}

bool EasyReqRespModifyMgr::HeaderOperation(CefRequest::HeaderMap& header, const EasyReplaceRule& Rule)
{
	bool bHaveOperation = false;

	if (Rule.ModifyType == RULE_MODIFY_TYPE::REQUEST_HEAD || Rule.ModifyType == RULE_MODIFY_TYPE::RESPONSE_HEAD)
	{
		if (!Rule.strHeadField.empty())
		{
			auto itemHead = header.find(Rule.strHeadField);
			if (itemHead == header.end())
			{
				if (Rule.bAddIfNotExist && !Rule.bReplaceUseRegex)
				{
					header.insert(std::make_pair(Rule.strHeadField, Rule.strReplace));
					bHaveOperation = true;
				}
			}
			else
			{
				auto strSrc = itemHead->second.ToString();

				if (Rule.strSearch.empty())
				{
					header.erase(itemHead);
					bHaveOperation = true;
				}
				else if (Rule.bReplaceUseRegex)
				{
					auto regex_constants = Rule.bReplaceCaseInsensitive ? std::regex_constants::icase : std::regex_constants::ECMAScript;

					std::regex word_regex(Rule.strSearch, regex_constants);
					std::smatch m;

					if (std::regex_search(strSrc, m, word_regex))
					{
						itemHead->second = std::regex_replace(strSrc, word_regex, Rule.strReplace);
						bHaveOperation = true;
					}
				}
				else
				{
					std::string strNew;

					if (ReplaceAllSubString(Rule.bReplaceCaseInsensitive, strSrc, Rule.strSearch, Rule.strReplace, strNew))
					{
						itemHead->second = strNew;
						bHaveOperation = true;
					}
				}
			}
		}
	}


	return bHaveOperation;
}


std::shared_ptr<EasyReplaceRule> EasyReqRespModifyMgr::FindRule(const RuleID id)
{
	auto it = m_mapRules.find(id);
	if (it == m_mapRules.end())
	{
		return nullptr;
	}

	return it->second;
}


std::shared_ptr<EasyReplaceRule> EasyReqRespModifyMgr::CreateNewRule()
{
	auto pNewRule = std::make_shared<EasyReplaceRule>();
	pNewRule->Id = ++m_IdGen;
	m_listRuleOrder.push_back(pNewRule->Id);
	auto res = m_mapRules.insert(std::make_pair(pNewRule->Id, pNewRule));

	return pNewRule;
}

RuleID EasyReqRespModifyMgr::AddRule(const EasyReplaceRule& Rule)
{
	auto pNewRule = std::make_shared<EasyReplaceRule>(Rule);
	auto NewId = ++m_IdGen;
	pNewRule->Id = NewId;
	m_listRuleOrder.push_back(pNewRule->Id);
	m_mapRules.insert(std::make_pair(NewId, pNewRule));

	return NewId;
}

bool EasyReqRespModifyMgr::DelRule(RuleID RuleId)
{
	auto it = m_mapRules.find(RuleId);

	if (it != m_mapRules.end())
	{
		m_mapRules.erase(it);

		size_t i = 0;
		for (auto itO : m_listRuleOrder)
		{
			if (itO == RuleId)
			{
				m_listRuleOrder.erase(m_listRuleOrder.begin() + i);
				break;
			}
			++i;
		}

		return true;
	}

	return false;
}

bool EasyReqRespModifyMgr::SetOrderInfo(RuleID id, long offset, int origin)
{
	size_t nTotalSize = m_listRuleOrder.size();
	size_t iCurrentItemPos = 0;
	for (; iCurrentItemPos < nTotalSize; ++iCurrentItemPos)
	{
		if (m_listRuleOrder[iCurrentItemPos] == id)
		{
			break;
		}
	}

	if (iCurrentItemPos == nTotalSize)
		return false;

	size_t nNewPos = 0;

	switch (origin)
	{
	case SEEK_CUR:
		nNewPos = iCurrentItemPos + offset;
		break;
	case SEEK_END:
		nNewPos = nTotalSize - 1 + offset;
		break;
	case SEEK_SET:
		nNewPos = offset;
		break;
	default:
		break;
	}

	if (nNewPos >= nTotalSize)
	{
		nNewPos = nTotalSize - 1;
	}

	if (iCurrentItemPos != nNewPos)
	{
		m_listRuleOrder.erase(m_listRuleOrder.begin() + iCurrentItemPos);

		m_listRuleOrder.insert(m_listRuleOrder.begin() + nNewPos, id);

		return true;
	}

	return false;
}

bool EasyReqRespModifyMgr::ModifyRequest(const std::vector<RuleID>& listIds, CefRefPtr<CefRequest> request)
{
	CefRequest::HeaderMap header;
	request->GetHeaderMap(header);

	auto pPostData = request->GetPostData();

	bool bHaveOperation = false;
	for (const auto NowRuleId: listIds)
	{
		auto item = m_mapRules.find(NowRuleId);
		if (item == m_mapRules.end())
			continue;

		const auto& it = item->second;
		
		if (it->ModifyType == RULE_MODIFY_TYPE::REQUEST_HEAD)
		{
			bHaveOperation = HeaderOperation(header, *it);
			if (bHaveOperation)
			{
				request->SetHeaderMap(header);
			}
		}
		else if (it->ModifyType == RULE_MODIFY_TYPE::REQUEST_POSTDATA)
		{
			if (pPostData && !it->strSearch.empty())
			{
				CefPostData::ElementVector vecEles;
				pPostData->GetElements(vecEles);
				for (auto& ele : vecEles)
				{
					if (PDE_TYPE_BYTES == ele->GetType())
					{
						std::string strSrc, strNew;
						strSrc.resize(ele->GetBytesCount());
						ele->GetBytes(strSrc.size(), strSrc.data());

						if (it->bReplaceUseRegex)
						{
							auto regex_constants = it->bReplaceCaseInsensitive ? std::regex_constants::icase : std::regex_constants::ECMAScript;
							std::regex word_regex(it->strSearch, regex_constants);
							std::smatch m;

							if (std::regex_search(strSrc, m, word_regex))
							{
								strNew = std::regex_replace(strSrc, word_regex, it->strReplace);
								bHaveOperation = true;
							}
						}
						else
						{
							if (ReplaceAllSubString(it->bReplaceCaseInsensitive, strSrc, it->strSearch, it->strReplace, strNew))
							{
								bHaveOperation = true;
							}
						}

						ele->SetToBytes(strNew.size(), strNew.c_str());
					}
				}
			}
		}


		if (bHaveOperation && !it->bContinueSearch)
		{
			break;
		}
		
	}

	return bHaveOperation;
}

bool EasyReqRespModifyMgr::ModifyResponse(const std::vector<RuleID>& listIds, CefRefPtr<CefResponse> response, std::string* pRespData)
{
	CefResponse::HeaderMap header;
	response->GetHeaderMap(header);

	bool bHaveOperation = false;
	for (const auto NowRuleId : listIds)
	{
		auto item = m_mapRules.find(NowRuleId);
		if (item == m_mapRules.end())
			continue;

		const auto& it = item->second;
		
		if (it->ModifyType == RULE_MODIFY_TYPE::RESPONSE_HEAD)
		{
			bHaveOperation = HeaderOperation(header, *it);
			if (bHaveOperation)
			{
				response->SetHeaderMap(header);
			}
		}
		else if (it->ModifyType == RULE_MODIFY_TYPE::RESPONSE_DATA && pRespData)
		{
			std::string strNew;
			if (it->bReplaceUseRegex)
			{
				auto regex_constants = it->bReplaceCaseInsensitive ? std::regex_constants::icase : std::regex_constants::ECMAScript;
				std::regex word_regex(it->strSearch, regex_constants);
				std::smatch m;

				if (std::regex_search(*pRespData, m, word_regex))
				{
					strNew = std::regex_replace(*pRespData, word_regex, it->strReplace);
					bHaveOperation = true;
				}
			}
			else
			{
				if (ReplaceAllSubString(it->bReplaceCaseInsensitive, *pRespData, it->strSearch, it->strReplace, strNew))
				{
					bHaveOperation = true;
				}
			}

			if (bHaveOperation)
			{
				*pRespData = std::move(strNew);
			}
		}

		if (bHaveOperation && !it->bContinueSearch)
		{
			break;
		}
		
	}

	return bHaveOperation;
}

EasyReqRespHandler::EasyReqRespHandler(CefRefPtr<CefBrowser> browser, std::vector<RuleID> ResponseListIds)
	: m_ResponseListIds(std::move(ResponseListIds)),
	m_Browser(browser)
{
}

void EasyReqRespHandler::RunRequestOnUIThread(CefRefPtr<CefRequest> request)
{
	if (!CefCurrentlyOn(TID_UI))
	{
		CefPostTask(TID_UI, CEF_FUNC_BIND(&EasyReqRespHandler::RunRequestOnUIThread, this, request));
		return;
	}

	CefURLRequest::Create(request, this, CefRequestContext::CreateContext(m_Browser->GetHost()->GetRequestContext(), nullptr));
}


bool EasyReqRespHandler::Open(CefRefPtr<CefRequest> request, bool& handle_request, CefRefPtr<CefCallback> callback)
{
	handle_request = false;
	m_OpenCallback = callback;

	CefRequest::HeaderMap headers;
	request->GetHeaderMap(headers);

	auto new_request = CefRequest::Create();
	new_request->Set(request->GetURL(), request->GetMethod(), request->GetPostData(), headers);
	new_request->SetReferrer(request->GetReferrerURL(), request->GetReferrerPolicy());
	new_request->SetFirstPartyForCookies(request->GetFirstPartyForCookies());

	//得对3XX之类的跳转做个特殊处理，否则 path是否带有斜杠会影响到资源地址
	new_request->SetFlags(request->GetFlags() | UR_FLAG_STOP_ON_REDIRECT);

	RunRequestOnUIThread(new_request);

	return true;
}

void EasyReqRespHandler::GetResponseHeaders(CefRefPtr<CefResponse> response, int64& response_length, CefString& redirectUrl)
{
	auto strMimeType = m_Response->GetMimeType();

	response->SetMimeType(strMimeType);
	auto status = m_Response->GetStatus();
	auto error = m_Response->GetError();

	response->SetStatusText(m_Response->GetStatusText());
	response->SetCharset(m_Response->GetCharset());
	if (error == ERR_ABORTED && status / 100 == 3)
	{
		//跳转时不能设置，否则无法继续后续请求
	}
	else
	{
		response->SetError(error);
	}
	
	response->SetURL(m_Response->GetURL());
	response->SetStatus(status);
	CefResponse::HeaderMap headerMap;
	m_Response->GetHeaderMap(headerMap);
	response->SetHeaderMap(headerMap);

	//只修改文本类型，以免异常
	bool bIsText = strMimeType.ToString().substr(0, 5) == "text/";

	EasyReqRespModifyMgr::GetInstance().ModifyResponse(m_ResponseListIds, response, bIsText ? &m_ResponseData : nullptr);

	response_length = m_ResponseData.length();
}

bool EasyReqRespHandler::Read(void* data_out, int bytes_to_read, int& bytes_read, CefRefPtr<CefResourceReadCallback> callback)
{
	bool has_data = false;
	bytes_read = 0;

	if (offset_ < m_ResponseData.length()) {
		// Copy the next block of data into the buffer.
		int transfer_size =
			std::min(bytes_to_read, static_cast<int>(m_ResponseData.length() - offset_));
		memcpy(data_out, m_ResponseData.c_str() + offset_, transfer_size);
		offset_ += transfer_size;

		bytes_read = transfer_size;
		has_data = true;
	}

	return has_data;
}

void EasyReqRespHandler::OnRequestComplete(CefRefPtr<CefURLRequest> request)
{
	m_Response = request->GetResponse();
	auto status = request->GetRequestStatus();
	switch (status)
	{
	case UR_SUCCESS:
	case UR_CANCELED:
		m_OpenCallback->Continue();
		break;
	case UR_IO_PENDING:
		assert(0);
	case UR_FAILED:
	case UR_UNKNOWN:
		m_OpenCallback->Cancel();
		break;
	default:
		break;
	}
}

void EasyReqRespHandler::OnDownloadData(CefRefPtr<CefURLRequest> request, const void* data, size_t data_length)
{
	m_ResponseData.append(static_cast<const char*>(data), data_length);
}

bool EasyReqRespHandler::Skip(int64 bytes_to_skip, int64& bytes_skipped, CefRefPtr<CefResourceSkipCallback> callback)
{
	bytes_skipped = bytes_to_skip;
	return true;
}

