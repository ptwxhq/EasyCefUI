#pragma once


#include <windows.h>

#ifndef _EASYCEF_STATICLIB
#if defined(EASYCEFDLL_EXPORTS)
#  define EASYCEF_EXP_API extern "C" __declspec(dllexport)
#  define EASYCEF_EXP_CLASS __declspec(dllexport)
#else
#  define EASYCEF_EXP_API extern "C" __declspec(dllimport)
#  define EASYCEF_EXP_CLASS __declspec(dllimport)

#pragma message("!!!!!--注意：生成exe文件请务必添加包含compatibility的manifest文件，否则无法正常运行--!!!!!")
#pragma message("生成后事件例: mt.exe -manifest \"compatibility.manifest\" -outputresource:\"xxx.exe\";#1")

#endif
#else
#  define EASYCEF_EXP_API
#  define EASYCEF_EXP_CLASS
#endif 

#ifndef NONEED_CC_MANIFEST
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


typedef struct EasyInitConfig
{
	bool bSupportLayerWindow; //如果项目不需要分层窗口（可鼠标穿透）则不需要启用，性能更好
	bool bShareNPPlugin;	//旧接口里面的，还不知道干嘛用的
	int ProcessType;	//手动指定进程类型，0自动识别，1主进程，2render，3或其他
	LPCWSTR strLocal;		//L"zh-CN"
	LPCWSTR strUserAgent;
	LPCWSTR strWebViewClassName;

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

};
