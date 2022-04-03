#pragma once

#include <list>
#include <memory>
#include <deque>
#include <unordered_map>
#include "include/cef_urlrequest.h"
#include "include/cef_resource_handler.h"

enum class RULE_MATCH_TYPE
{
	KEYWORD,
	HOST,
	REGEX,
};

enum class RULE_MODIFY_TYPE
{
	NONE,
	REQUEST_HEAD,
	REQUEST_POSTDATA,
	RESPONSE_HEAD,
	RESPONSE_DATA,
};

typedef unsigned RuleID;

class EasyReplaceRule
{
	friend class EasyReqRespModifyMgr;
public:
	const RuleID GetRuleId() {
		return Id;
	}

	const RULE_MATCH_TYPE GetMatchType() {
		return MatchType;
	}

	const std::string& GetMatchInfo() {
		return strUrlMathInfo;
	}

	bool SetUrlMatchType(LPCSTR lpszMatchInfo, RULE_MATCH_TYPE Type) {
		strUrlMathInfo = lpszMatchInfo;
		MatchType = Type;
		return CheckRegex(1);
	}

	const bool GetIsReplaceUseRegex() {
		return bReplaceUseRegex;
	}

	bool SetSearchContents(LPCSTR lpszSearch, bool bRegex) {
		bReplaceUseRegex = bRegex;
		strSearch = lpszSearch;
		return CheckRegex(2);
	}

	const std::string& GetSearchInfo() {
		return strSearch;
	}

	RULE_MODIFY_TYPE ModifyType = RULE_MODIFY_TYPE::NONE;

	bool bContinueSearch = false;	//有效执行（添加/修改/删除）到本条时是否继续操作
	bool bReplaceCaseInsensitive = false;
	bool bAddIfNotExist = false; //仅header，bReplaceUseRegex true时无效

	std::string strHeadField; //REQUEST_HEAD RESPONSE_HEAD

	std::string strReplace;
private:
	RuleID Id = 0;
	RULE_MATCH_TYPE MatchType = RULE_MATCH_TYPE::KEYWORD;
	std::string strUrlMathInfo;	   //KEYWORD空值表示所有
	bool bReplaceUseRegex = false;
	std::string strSearch;	  //使用strHeadField时可为空，为插入内容

	//正则校验，如无效本条作废:ModifyType = RULE_MODIFY_TYPE::NONE;
	bool CheckRegex(int type);
};


class EasyReqRespModifyMgr
{
	DISALLOW_COPY_AND_ASSIGN(EasyReqRespModifyMgr);
	EasyReqRespModifyMgr() = default;
public:

	static EasyReqRespModifyMgr& GetInstance();
	std::shared_ptr<EasyReplaceRule> CreateNewRule();
	RuleID AddRule(const EasyReplaceRule& Rule);
	bool DelRule(RuleID RuleId);
	std::shared_ptr<EasyReplaceRule> FindRule(const RuleID RuleId);

	
	bool SetOrderInfo(RuleID id, long offset, int origin);

	//修改的内容在控制台里看可能还是旧的，但确实已修改成功
	bool ModifyRequest(const std::vector<RuleID>& listIds, CefRefPtr<CefRequest> request);
	bool ModifyResponse(const std::vector<RuleID>& listIds, CefRefPtr<CefResponse> response, std::string* pRespData);

	bool CheckMatchUrl(const std::string& strUrl, std::vector<RuleID>* pRequestListIds, std::vector<RuleID>* pResponseListIds, int RequestType);

private:

	bool CheckMatchUrl(const std::string& strUrl, const std::string& strHost, const EasyReplaceRule& Rule, int RequestType);
	bool HeaderOperation(CefRequest::HeaderMap& header, const EasyReplaceRule& Rule);

	RuleID m_IdGen = 0;
	std::deque<RuleID> m_listRuleOrder;
	std::unordered_map<RuleID, std::shared_ptr<EasyReplaceRule>> m_mapRules;
};



class EasyReqRespHandler :
	public CefResourceHandler,
	public CefURLRequestClient
{
	IMPLEMENT_REFCOUNTING(EasyReqRespHandler);
	DISALLOW_COPY_AND_ASSIGN(EasyReqRespHandler);

	void RunRequestOnUIThread(CefRefPtr<CefRequest> request);

public:
	EasyReqRespHandler(CefRefPtr<CefBrowser> browser, std::vector<RuleID> ResponseListIds);

	bool Open(CefRefPtr<CefRequest> request,
		bool& handle_request,
		CefRefPtr<CefCallback> callback) override;

	void GetResponseHeaders(CefRefPtr<CefResponse> response,
		int64& response_length,
		CefString& redirectUrl) override;

	bool Skip(int64 bytes_to_skip,
		int64& bytes_skipped,
		CefRefPtr<CefResourceSkipCallback> callback) override;

	bool Read(void* data_out,
		int bytes_to_read,
		int& bytes_read,
		CefRefPtr<CefResourceReadCallback> callback) override;

	void Cancel() override {}

	void OnRequestComplete(CefRefPtr<CefURLRequest> request) override;

	void OnUploadProgress(CefRefPtr<CefURLRequest> request,
		int64 current,
		int64 total) override {};

	void OnDownloadProgress(CefRefPtr<CefURLRequest> request,
		int64 current,
		int64 total) override {};

	void OnDownloadData(CefRefPtr<CefURLRequest> request,
		const void* data,
		size_t data_length) override;

	bool GetAuthCredentials(bool isProxy,
		const CefString& host,
		int port,
		const CefString& realm,
		const CefString& scheme,
		CefRefPtr<CefAuthCallback> callback) override {
		return false;
	}

private:
	size_t offset_ = 0;
	std::string m_ResponseData;
	std::vector<RuleID> m_ResponseListIds;

	CefRefPtr<CefCallback> m_OpenCallback;
	CefRefPtr<CefResponse> m_Response;
	CefRefPtr<CefBrowser> m_Browser;
	
};