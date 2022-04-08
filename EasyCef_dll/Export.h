#pragma once


#include <windows.h>

#ifndef _EASYCEF_STATICLIB
#if defined(EASYCEFDLL_EXPORTS)
#  define EASYCEF_EXP_API extern "C" __declspec(dllexport)
#  define EASYCEF_EXP_CLASS __declspec(dllexport)
#else
#  define EASYCEF_EXP_API extern "C" __declspec(dllimport)
#  define EASYCEF_EXP_CLASS __declspec(dllimport)
#endif
#else
#  define EASYCEF_EXP_API
#  define EASYCEF_EXP_CLASS
#endif 

#if !defined(NONEED_CC_MANIFEST) && !defined(__clang__)
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif


#ifdef STU_WEBVIEWEXTATTR

struct WebViewExtraAttr;
struct EasyCefFunctionFlag;
#else

struct WebViewExtraAttr
{
	bool taskbar /*= false*/;
	bool transparent/* = false*/;
	bool resvr /*= false*/;
	unsigned char alpha /*= 255*/; //transparent 为true时有效

	//int legacydata;
	char windowinitstatus /*= 0*/;

	DWORD dwAddStyle/* = 0*/;
	DWORD dwAddExStyle /*= 0*/;

	//以上值控件窗口无效

	WebViewExtraAttr() :taskbar(true), transparent(false), resvr(false), alpha(255), windowinitstatus(0),
		dwAddStyle(0), dwAddExStyle(0)
	 {}
};


struct EasyCefFunctionFlag
{
	bool bUIImeFollow;//输入法跟随光标，目前在UI界面上使用可能有bug

	EasyCefFunctionFlag() :bUIImeFollow(false)
	{}
};

#endif


namespace EASYCEF {

typedef void(*SpeedUpWork)(float);
typedef void(*CloseHandler)();
typedef bool(*XPackExtractWork)(LPCWSTR lpszPackFile, LPCWSTR lpszFileItem, BYTE** ppOut, DWORD *pLen);
typedef void(*XPackFreeData)(BYTE* pOut);

typedef struct EasyInitConfig
{
	bool bSupportLayerWindow; //如果项目不需要分层窗口（可鼠标穿透）则不需要启用，性能更好
	int ProcessType;	//手动指定进程类型，0自动识别，1主进程，2render，3或其他
	LPCWSTR strLocal;		//L"zh-CN"
	LPCWSTR strUserAgent;
	LPCWSTR strWebViewClassName;
	LPCWSTR strUILoadingTitle;

	EasyInitConfig() {
		memset(this, 0, sizeof(EasyInitConfig));
	}
} EASYINITCONFIG, * PEASYINITCONFIG;


//注册xpack协议使用的域名对应文件，默认内置ui.pack，对应同路径下ui.pack文件，需要其他对应关系时可在此添加
//ui.pack可修改可删除，删除表示使用默认值
//域名不区分大小写，注意页面中如果域名格式是无效的或未注册域名返回400
EASYCEF_EXP_API bool RegisterPackDomain(LPCWSTR lpszDomain, LPCWSTR lpszFilePath);
EASYCEF_EXP_API void UnregisterPackDomain(LPCWSTR lpszDomain);


//EASYCEF_EXP_API bool AddCrossOriginWhitelistEntry(LPCWSTR source_origin, LPCWSTR target_protocol, LPCWSTR target_domain, bool allow_target_subdomains);
//EASYCEF_EXP_API bool RemoveCrossOriginWhitelistEntry(LPCWSTR source_origin, LPCWSTR target_protocol, LPCWSTR target_domain, bool allow_target_subdomains);

EASYCEF_EXP_API void SetSpeedUpWork(SpeedUpWork func);

//所有浏览器窗口均已关闭通知，默认已设为QuitMsgLoop
EASYCEF_EXP_API void SetCloseHandler(CloseHandler func);

EASYCEF_EXP_API void RunMsgLoop();

EASYCEF_EXP_API void QuitMsgLoop();

EASYCEF_EXP_API void ShutEasyCef();

//旧版本InitLibrary的替代接口
EASYCEF_EXP_API int InitEasyCef(HINSTANCE hInstance, LPCWSTR lpRender = NULL, PEASYINITCONFIG pConf= nullptr);

//TODO 后续参数待补充，现在先简单点
//既可以单独弹窗，也可以做子窗口
//x==y==width==height==0时视为随父窗口大小自适应
EASYCEF_EXP_API void EasyCreateWebControl(HWND hParent, int x, int y, int width, int height, LPCWSTR url, LPCWSTR cookiepath, const WebViewExtraAttr* pExt);

EASYCEF_EXP_API HWND EasyCreateWebUI(HWND hParent, int x, int y, int width, int height, LPCWSTR url, const WebViewExtraAttr* pExt);

//空为恢复默认值，非初始化前设置值部分后可能无法生效
EASYCEF_EXP_API void SetFunctionFlag(EasyCefFunctionFlag *pFlag);

//仅限chromium87或以下版本，需要加载前设置，如不设置则默认使用系统flash
EASYCEF_EXP_API void SetFlashPluginPath(LPCWSTR lpszPath);


struct EasyReqRspRule
{
	enum RULE_MATCH_TYPE
	{
		KEYWORD,
		HOST,
		REGEX,
	};

	enum RULE_MODIFY_TYPE
	{
		NONE,
		REQUEST_HEAD,
		REQUEST_POSTDATA,
		RESPONSE_HEAD,
		RESPONSE_DATA,
	};

	RULE_MATCH_TYPE MatchType;
	RULE_MODIFY_TYPE ModifyType;

	bool bContinueSearch;	//有效执行（添加/修改/删除）到本条时是否继续操作
	bool bReplaceCaseInsensitive;
	bool bAddIfNotExist; //仅header，bReplaceUseRegex true时无效
	bool bReplaceUseRegex;

	LPCSTR strUrlMathInfo;	   //KEYWORD空值表示所有
	LPCSTR strHeadField; //REQUEST_HEAD RESPONSE_HEAD
	LPCSTR strSearch;	  //使用strHeadField时可为空，为插入内容
	LPCSTR strReplace;


	EasyReqRspRule()
	{
		memset(this, 0, sizeof(EasyReqRspRule));
	}

};


EASYCEF_EXP_API unsigned AddReqRspRule(const EasyReqRspRule* pRule);
EASYCEF_EXP_API bool SetReqRspRule(unsigned id, const EasyReqRspRule* pRule);
//origin: 0 1 2 最前 当前位 最后 有结果且变化true
EASYCEF_EXP_API bool SetReqRspOrder(unsigned id, long offset, int origin);
EASYCEF_EXP_API bool DelReqRspRule(unsigned id);
EASYCEF_EXP_API bool GetReqRspRule(unsigned id, EasyReqRspRule* pRule);

//
EASYCEF_EXP_API bool SetXPackWorkCall(XPackExtractWork funWork, XPackFreeData funFree);

};
