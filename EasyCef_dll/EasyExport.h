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

enum WEBWININITSTATUS
{
	ATTR_ENABLE_DRAG_DROP = 1,	//允许拖拽文件
	INIT_MIN_SIZE = 1 << 1,		//创建窗口时最小化
	INIT_MAX_SIZE = 1 << 2,		//创建窗口时最大化
};

struct WebViewExtraAttr
{
	bool taskbar /*= false*/;
	bool transparent/* = false*/;
	bool resvr /*= false*/;
	unsigned char alpha /*= 255*/; //transparent 为true时有效

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
	bool bUIImeFollow;
	bool bEnableHignDpi; //禁用后GetWindowScaleFactor总是1.0f

	EasyCefFunctionFlag() :bUIImeFollow(false), bEnableHignDpi(true)
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
	int ProcessType;	//手动指定进程类型，0自动识别，1主进程，2render
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
//x==y==width==height==0时视为随父窗口大小自适应。控件的句柄通过CallWebControlCreated回调得到
EASYCEF_EXP_API void EasyCreateWebControl(HWND hParent, int x, int y, int width, int height, LPCWSTR url, LPCWSTR cookiepath, const WebViewExtraAttr* pExt);

EASYCEF_EXP_API HWND EasyCreateWebUI(HWND hParent, int x, int y, int width, int height, LPCWSTR url, const WebViewExtraAttr* pExt);

//空为恢复默认值，非初始化前设置值部分后可能无法生效
EASYCEF_EXP_API void SetFunctionFlag(EasyCefFunctionFlag *pFlag);

//仅限chromium87或以下版本，需要加载前设置，如不设置则默认使用系统flash
EASYCEF_EXP_API void SetFlashPluginPath(LPCWSTR lpszPath);
EASYCEF_EXP_API void SetCachePath(LPCWSTR lpszPath);


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

EASYCEF_EXP_API float GetWindowScaleFactor(HWND hwnd);

//默认lpszExtName为bin
EASYCEF_EXP_API bool AddMemoryFile(const void* pData, unsigned int nDataLen, size_t* id, LPCWSTR lpszDomain = nullptr, LPCWSTR lpszExtName = nullptr);
EASYCEF_EXP_API void DelMemoryFile(size_t id);
EASYCEF_EXP_API bool GetMemoryFileUrl(size_t id, LPWSTR lpszUrl, unsigned int nInLen, unsigned int* nOutLen);
EASYCEF_EXP_API bool GetMemoryFile(size_t id, void* pData, unsigned int* nLen);
EASYCEF_EXP_API bool GetMemoryByUrl(LPCWSTR lpszUrl, void* pData, unsigned int* nLen);

//强制页面重绘
EASYCEF_EXP_API void ForceWebUIPaint(HWND hWnd);



EASYCEF_EXP_API char* CreateEasyString(size_t nLen);
EASYCEF_EXP_API void FreeEasyString(const char* str);
//自定义注册函数
typedef int(*jscall_UserFunction)(HWND hWnd, LPCSTR jsonParams, char** strRet, void* context);	 //使用CreateEasyString返回strRet

//RegType: 
#define REGJS_ONUI			1
#define REGJS_ONWEBCONTROL	2
#define REGJS_ON_ALL		(REGJS_ONUI|REGJS_ONWEBCONTROL)
//sync only
#define REGJS_RUN_UITHREAD	4
EASYCEF_EXP_API bool RegisterJSFunction(bool bSync, LPCSTR lpszFunctionName, jscall_UserFunction func, void *context, int RegType);


typedef void(*FuncAddContextMenu)(void* callneed, int command, LPCWSTR lpszLabel, bool bTop, bool bEnable);
typedef void(*CallBeforeContextMenu)(HWND hWnd, int x, int y, bool bIsEdit, FuncAddContextMenu call, void* callneed);
typedef bool(*CallDoMenuCommand)(HWND hWnd, int command);
EASYCEF_EXP_API bool SetAddContextMenuCall(CallBeforeContextMenu func, CallDoMenuCommand fundo);


typedef void(*CallDOMCompleteStatus)(HWND hWnd, LPCWSTR lpszFrameName, LPCWSTR lpszUrl, LPCWSTR lpszMainUrl);
EASYCEF_EXP_API void SetDOMCompleteStatusCallback(CallDOMCompleteStatus func);

typedef void(*CallNativeCompleteStatus)(HWND hWnd, LPCWSTR lpszUrl, LPCWSTR lpszFrameName, bool bIsMain);
EASYCEF_EXP_API void SetNativeCompleteStatusCallback(CallNativeCompleteStatus func);

typedef void(*CallPopNewUrl)(HWND hWnd, LPCWSTR lpszUrl, LPCWSTR lpszFrameName);
EASYCEF_EXP_API void SetPopNewUrlCallback(CallPopNewUrl func);


//返回true不加载错误页面
typedef bool(*LoadErrorHandler)(HWND hWnd, int iErrorCode, LPCWSTR lpszUrl, LPCWSTR lpszFrameName, bool bIsMain);
EASYCEF_EXP_API void SetLoadErrorHandler(LoadErrorHandler func);

//返回true自主处理下载，返回false使用默认处理（打开保存对话框）
typedef bool(*BeforeDownloadHandler)(HWND hWnd, unsigned int nId, LPCWSTR lpszUrl, LPCWSTR lpszSuggestFileName);
//返回false取消下载  BeforeDownloadHandler之前可能会先收到DownloadStatusHandler，并且未返回期间会持续进行下载
typedef bool(*DownloadStatusHandler)(unsigned int nId, long long nDownloadedSize, long long nTotalSize);
EASYCEF_EXP_API void SetDownloadHandler(BeforeDownloadHandler func, DownloadStatusHandler funcStatus);

//lpszFrameName NULL -> 全部框架页
EASYCEF_EXP_API void ExecuteJavaScript(HWND hWnd, LPCWSTR lpszFrameName, LPCWSTR lpszJSCode);



//s:String(UTF8) w:String(WCHAR) i:Number(int/char/short/long), f:Number(float/double),  b:Boolean u:Null 暂不支持其他数据
//exp: "sinbffw", "str", 10, NULL, true, 9.1, 7.f, L"wide"
//返回信息无需释放，需要在下一次调用前使用
EASYCEF_EXP_API	bool InvokeJSFunction(HWND hWnd, LPCSTR lpszJSFunctionName, LPCSTR lpszFrameName, const char** ppStrRet, DWORD dwTimeout, LPCSTR parmfmt, ...);
EASYCEF_EXP_API	bool InvokeJSFunctionAsync(HWND hWnd, LPCSTR lpszJSFunctionName, LPCSTR lpszFrameName, LPCSTR parmfmt, ...);

//尽量不用这个，容易引起不稳定问题（请只在控件模式下使用，如不使用EasyRender进程则此调用无效
EASYCEF_EXP_API void AdjustRenderProcessSpeed(HWND hWnd, double dbSpeed);


EASYCEF_EXP_API void CleanCookies();

EASYCEF_EXP_API bool EasyLoadUrl(HWND hWnd, LPCWSTR lpszUrl);
EASYCEF_EXP_API void EasyCloseAllWeb();

EASYCEF_EXP_API void SetWindowAlpha(HWND hWnd, unsigned char val);

//浏览控件操作
enum WEBCONTROLWORK
{
	WC_GOBACK,				//后退
	WC_GOFOWARD,			//前进
	WC_RELOAD,				//刷新
	WC_RELOADIGNORECACHE,	//不使用缓存刷新
	WC_STOP,				//停止加载
	WC_MUTEAUDIO,			//静音
	WC_UNMUTEAUDIO,			//取消静音

};

EASYCEF_EXP_API bool WebControlDoWork(HWND hWnd, WEBCONTROLWORK dowork);
//主线程调用
EASYCEF_EXP_API bool GetViewZoomLevel(HWND hWnd, double& level);

EASYCEF_EXP_API bool SetViewZoomLevel(HWND hWnd, double level);


//下载回调，通用
//typedef void(__stdcall* call_WebkitDownFileUrl)(HWND id, LPCWSTR url, LPCWSTR suggestFileName);


//控件回调
typedef void(*CallWebControlCreated)(int id, HWND hControl);
typedef void(*CalllWebControLoadingState)(int id, bool loading, bool canBack, bool canForward);
typedef void(*CallWebControUrlChange)(int id, LPCWSTR url);
typedef void(*CallWebControTitleChange)(int id, LPCWSTR title);
typedef void(*CallWebControFavIconChange)(int id, LPCWSTR icon_url);
typedef void(*CallWebControLoadBegin)(int id, LPCWSTR url, bool* cancel);
typedef void(*CallWebControLoadEnd)(int id);
typedef void(*CallWebControBeforeClose)(int id);
//返回false取消加载，未设置或者不设置hParent使用默认弹窗
typedef bool(*CallWebControBeforePopup)(int id, LPCWSTR url, HWND *hParent);

struct WebControlCallbacks
{
	CallWebControlCreated			WebCreated;
	CalllWebControLoadingState		WebLoadingState;
	CallWebControUrlChange			WebUrlChange;
	CallWebControTitleChange		WebTitleChange;
	CallWebControFavIconChange		WebFavIconChange;
	CallWebControLoadBegin			WebLoadBegin;
	CallWebControLoadEnd			WebLoadEnd;
	CallWebControBeforeClose		WebBeforeClose;
	CallWebControBeforePopup		WebBeforePopup;
};

EASYCEF_EXP_API void SetWebControlCallbacks(WebControlCallbacks* callbacks);



};
