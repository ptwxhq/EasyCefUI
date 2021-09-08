#pragma once

#include "Export.h"
//////////////////////////////////////////////////////////////////////////////////

//旧接口兼容导出

/*
影响已经很小，仍有变更内容：
·const WCHAR* 替换为 LCPWSTR
·部分std::wstring& 替换为 LCPWSTR
·CStringW* 换为WCHAR** 需要使用FreeJSReturnStr释放
·追加FreeJSReturnStr
·const X& 去掉const和&属性

·XXXX QueryPluginsProcessID参数变更，由(const HWND&, std::vector<DWORD>& plugins_process_ids)修改为(HWND, DWORD*Count, DWORD**plugins_process_ids)
·XXXX追加FreeQueryPluginReturnData

·TryQuitLoop添加返回值

·WIDGET_NORMAL_SIZE和ENABLE_DRAG_DROP定义值变更

callJSMethod,SendMouseClickEvent没用到，直接废弃
QueryRenderProcessID
QueryPluginsProcessID也废弃

因实现原有废弃：
clearResolveHost

*/


namespace wrapQweb {

#define REMOVE_DATA_MASK_APPCACHE  1 << 0
	//清理实际应该就下面这个cookie的有效，其他需要关闭进程删
#define REMOVE_DATA_MASK_COOKIES 1 << 1
#define REMOVE_DATA_MASK_FILE_SYSTEMS 1 << 2
#define REMOVE_DATA_MASK_INDEXEDDB  1 << 3
#define REMOVE_DATA_MASK_LOCAL_STORAGE 1 << 4
#define REMOVE_DATA_MASK_SHADER_CACHE 1 << 5
#define REMOVE_DATA_MASK_WEBSQL 1 << 6
#define REMOVE_DATA_MASK_WEBRTC_IDENTITY 1 << 7
#define REMOVE_DATA_MASK_SERVICE_WORKERS 1 << 8
#define REMOVE_DATA_MASK_ALL  0xFFFFFFFF


	//创建窗口时默认大小
#define WIDGET_NORMAL_SIZE 0 
	//1 << 0

	//创建窗口时最小化
#define WIDGET_MIN_SIZE 1 << 1

	//创建窗口时最大化
#define WIDGET_MAX_SIZE 1 << 2

	//允许拖拽
#define ENABLE_DRAG_DROP 1
	//1 << 4

	struct WRAP_CEF_MENU_COMMAND
	{
		bool bEnable;
		bool top;
		WCHAR szTxt[256];
		int command;
	};

	typedef WCHAR* JS_RETURN_WSTR;

	typedef long(__stdcall* call_closeWindow)(HWND hWnd);

	typedef long(__stdcall* call_setWindowPos)(HWND hWnd, long order, long x, long y, long cx, long cy, long flag);

	typedef long(__stdcall* call_createWindow)(HWND hWnd, long x, long y, long width, long height, long min_cx, long min_cy, long max_cx, long max_cy,
		LPCWSTR skin, long alpha, unsigned long ulStyle, bool bTrans, unsigned long extra);

	typedef long(__stdcall* call_createModalWindow)(HWND hWnd, long x, long y, long width, long height, long min_cx, long min_cy, long max_cx, long max_cy,
		LPCWSTR skin, long alpha, unsigned long ulStyle, bool bTrans, unsigned long extra);

	typedef long(__stdcall* call_createModalWindow2)(HWND hWnd, long x, long y, long width, long height, long min_cx, long min_cy, long max_cx, long max_cy,
		LPCWSTR skin, long alpha, unsigned long ulStyle, bool bTrans, unsigned long extra, unsigned long parentSign);

	typedef LPCWSTR(__stdcall* call_invokeMethod)(HWND hWnd, LPCWSTR modulename, LPCWSTR methodname, LPCWSTR parm, unsigned long extra);

	typedef LPCWSTR(__stdcall* call_crossInvokeWebMethod)(HWND hWnd, long winSign, LPCWSTR modulename,
		LPCWSTR methodname, LPCWSTR parm, bool bNoticeJSTrans2JSON);

	typedef LPCWSTR(__stdcall* call_crossInvokeWebMethod2)(HWND hWnd, long winSign, LPCWSTR framename, LPCWSTR modulename,
		LPCWSTR methodname, LPCWSTR parm, bool bNoticeJSTrans2JSON);

	typedef void(__stdcall* call_asyncCrossInvokeWebMethod)(HWND hWnd, long winSign, LPCWSTR modulename,
		LPCWSTR methodname, LPCWSTR parm, bool bNoticeJSTrans2JSON);

	typedef void(__stdcall* call_asyncCrossInvokeWebMethod2)(HWND hWnd, long winSign, LPCWSTR framename, LPCWSTR modulename,
		LPCWSTR methodname, LPCWSTR parm, bool bNoticeJSTrans2JSON);

	typedef LPCWSTR(__stdcall* call_winProty)(HWND hWnd);

	typedef LPCWSTR(__stdcall* call_softwareAttribute)(unsigned long);

	typedef void(__stdcall* call_NativeComplate)(HWND);

	typedef void(__stdcall* call_NativeFrameComplate)(HWND, LPCWSTR url, LPCWSTR frameName);

	typedef void(__stdcall* call_NativeFrameBegin)(HWND, LPCWSTR url, LPCWSTR frameName);

	typedef void(__stdcall* call_newNativeUrl)(HWND, LPCWSTR url, LPCWSTR frameName);

	typedef bool(__stdcall* call_doMenuCommand)(HWND, int id);

	typedef LPCWSTR(__stdcall* call_InjectJS)(HWND, LPCWSTR url, LPCWSTR mainurl, LPCWSTR frameName);

	typedef void(__stdcall* call_InertMenu)(HWND, LPCWSTR attribName, WRAP_CEF_MENU_COMMAND[]);

	typedef void(__stdcall* call_LoadError)(HWND, int errcode, LPCWSTR frameName, LPCWSTR url);

	typedef struct _FunMap {
		call_closeWindow closeWindow;
		call_setWindowPos setWindowPos;
		call_createWindow createWindow;
		call_createModalWindow createModalWindow;
		call_createModalWindow2 createModalWindow2;
		call_invokeMethod invokeMethod;
		call_crossInvokeWebMethod crossInvokeWebMethod;
		call_crossInvokeWebMethod2 crossInvokeWebMethod2;
		call_asyncCrossInvokeWebMethod asyncCrossInvokeWebMethod;
		call_asyncCrossInvokeWebMethod2 asyncCrossInvokeWebMethod2;
		call_winProty winProty;
		call_softwareAttribute softAttr;
		call_NativeComplate nativeComplate;
		call_NativeFrameComplate nativeFrameComplate;
		call_newNativeUrl newNativeUrl;
		call_doMenuCommand doMenuCommand;
		call_InjectJS injectJS;
		call_InertMenu insertMenu;
		call_LoadError loadError;
	}FunMap;

	//在InitLibrary之前调用，之后调用无效
	//EASYCEF_EXP_API void SetUserAgent(LPCWSTR ua);

	//__declspec(deprecated("建议使用新的api[InitEasyCef]代替"))
	//	EASYCEF_EXP_API int InitLibrary(HINSTANCE hInstance, LPCWSTR lpRender = NULL, LPCWSTR szLocal = L"zh-CN", bool bShareNPPlugin = false);

	//__declspec(deprecated("建议使用新的api[ShutEasyCef]代替"))
	//EASYCEF_EXP_API void FreeLibary();

	EASYCEF_EXP_API void InitQWeb(FunMap* map);

	//__declspec(deprecated("建议使用新的api[RunMsgLoop]代替"))
	//EASYCEF_EXP_API void RunLoop();

	//__declspec(deprecated("建议使用新的api[QuitMsgLoop]代替"))
	//EASYCEF_EXP_API void QuitLoop();

	//__declspec(deprecated("建议使用新的api[SetCloseHandler/QuitMsgLoop]代替"))
	//EASYCEF_EXP_API bool TryQuitLoop();

	EASYCEF_EXP_API void CloseWebview(HWND);

	EASYCEF_EXP_API void CloseAllWebView();

	EASYCEF_EXP_API bool UILoadUrl(HWND, LPCWSTR url);

	EASYCEF_EXP_API HWND CreateWebView(int x, int y, int width, int height, LPCWSTR lpResource, int alpha, bool taskbar, bool trans, int winCombination);

	//现在没有实现，等同于CreateWebView
	EASYCEF_EXP_API HWND CreateInheritWebView(HWND, int x, int y, int width, int height, LPCWSTR lpResource, int alpha, bool taskbar, bool trans, int winCombination);

	EASYCEF_EXP_API bool QueryNodeAttrib(HWND, int x, int y, const char* name, WCHAR* outVal, int len);

	EASYCEF_EXP_API void SetFouceWebView(HWND hWnd, bool fouce);

	//操作脚本
	//

	//软件同步通信方法(模块，方法 ，参数，返回值， 框架名，前端使用的参数bNoticeJSTrans2JSON）
	//不需要返回值时自动转异步
		EASYCEF_EXP_API bool invokedJSMethod(HWND, const char* utf8_module, const char* utf8_method,
			const char* utf8_parm, /*CStringW**/WCHAR** outstr,
			const char* utf8_frame_name = 0, bool bNoticeJSTrans2JSON = true);

	//软件异步通信方法(模块，方法 ，参数，返回值， 框架名，前端使用的参数bNoticeJSTrans2JSON）
		EASYCEF_EXP_API bool asyncInvokedJSMethod(HWND, const char* utf8_module, const char* utf8_method,
			const char* utf8_parm,
			const char* utf8_frame_name = 0, bool bNoticeJSTrans2JSON = true);

	EASYCEF_EXP_API void FreeJSReturnStr(JS_RETURN_WSTR str);

	//清理解析
	//实现起来有点麻烦，删掉，具体看cpp内注释
	//EASYCEF_EXP_API void clearResolveHost();

	//EASYCEF_EXP_API bool QueryRenderProcessID(HWND, int& pid);

	//EASYCEF_EXP_API bool QueryPluginsProcessID(HWND, DWORD* Count, DWORD** plugins_process_ids);
	//EASYCEF_EXP_API void FreeQueryPluginReturnData(DWORD* plugins_process_ids);

	//主线程调用
	EASYCEF_EXP_API bool GetViewZoomLevel(HWND, double& level);

	EASYCEF_EXP_API bool SetViewZoomLevel(HWND, double level);

	//npapi已经不能用了，已被ppapi替代，ppapi也要废弃了
	//chromium 87是最后一个支持flash的版本，这里让87默认开启
	//因此这个没有什么意义了，删去
	//EASYCEF_EXP_API bool PrepareSetEnableNpapi(bool bEnable);

	//在InitLibrary之前调用，之后调用无效
	EASYCEF_EXP_API void PrepareSetCachePath(const LPCWSTR path);

	//---------------------------------------------------------------------------------------------------------------------
	//类分割线

	typedef void(__stdcall* call_WebkitAfterCreate)(HWND, HWND, HWND, int id);

	typedef void(__stdcall* call_WebkitOpenNewUrl)(int id, LPCWSTR url, LPCWSTR cookie_ctx);

	typedef void(__stdcall* call_WebkitLoadingStateChange)(int id, bool loading, bool canBack, bool canForward);

	typedef void(__stdcall* call_WebkitChangeUrl)(int id, LPCWSTR url);

	typedef void(__stdcall* call_WebkitChangeTitle)(int id, LPCWSTR title);

	typedef void(__stdcall* call_WebkitBeginLoad)(int id, LPCWSTR url, bool* cancel);

	typedef void(__stdcall* call_WebkitEndLoad)(int id);

	typedef LPCWSTR(__stdcall* call_WebkitInvokeMethod)(int id, LPCWSTR modulename, LPCWSTR methodname, LPCWSTR parm, unsigned long extra);

	typedef LPCWSTR(__stdcall* call_WebkitInjectJS)(int id, LPCWSTR url, LPCWSTR mainurl, LPCWSTR title);

	typedef void(__stdcall* call_WebkitDownFileUrl)(int id, LPCWSTR url, LPCWSTR suggestFileName);

	typedef void(__stdcall* call_WebkitAsyncCallMethod)(int id, LPCWSTR modulename, LPCWSTR methodname, LPCWSTR parm, unsigned long extra);

	typedef void(__stdcall* call_WebkitPluginCrash)(int id, LPCWSTR path);

	typedef void(__stdcall* call_WebkitBeforeClose)(int id);

	typedef void(__stdcall* call_WebkitDocLoaded)(int id, LPCWSTR url, LPCWSTR frameName, bool bMainFrame);

	typedef void(__stdcall* call_WebkitSiteIcon)(int id, LPCWSTR main_url, LPCWSTR icon_url);

	typedef bool(__stdcall* call_WebkitNewTab)(int id, LPCWSTR main_url, HWND* parent);

	typedef void(__stdcall* call_WebkitLoadError)(int id, int errcode, bool isMain, LPCWSTR frameName, LPCWSTR url);

	typedef struct _EchoMap {
		call_WebkitAfterCreate webkitAfterCreate;
		call_WebkitOpenNewUrl webkitOpenNewUrl;
		call_WebkitLoadingStateChange webkitLoadingStateChange;
		call_WebkitChangeUrl webkitChangeUrl;
		call_WebkitChangeTitle webkitChangeTitle;
		call_WebkitBeginLoad webkitBeginLoad;
		call_WebkitEndLoad webkitEndLoad;
		call_WebkitInvokeMethod webkitInvokeMethod;
		call_WebkitInjectJS webkitInjectJS;
		call_WebkitDownFileUrl webkitDownFileUrl;
		call_WebkitAsyncCallMethod webkitAsyncCallMethod;
		call_WebkitPluginCrash webkitPluginCrash;
		call_WebkitBeforeClose webkitBeforeClose;
		call_WebkitDocLoaded webkitDocLoaded;
		call_WebkitSiteIcon webkitSiteIcon;
		call_WebkitNewTab webkitNewTab;
		call_WebkitLoadError webkitLoadError;
	}EchoMap;

	//初始化浏览器控件响应函数
	EASYCEF_EXP_API void InitEchoFn(EchoMap* map);

	//chromium81开始cookie路径有严格要求，必须位于主目录的下面，不能随意设置，否则使用内存模式
	EASYCEF_EXP_API void CreateWebControl(HWND hwnd, LPCWSTR url, LPCWSTR cookie = NULL);

	EASYCEF_EXP_API bool CloseWebControl(HWND hwnd);

	EASYCEF_EXP_API bool LoadUrl(HWND hwnd, LPCWSTR url);

	EASYCEF_EXP_API bool GoBack(HWND hwnd);

	EASYCEF_EXP_API bool GoForward(HWND hwnd);

	EASYCEF_EXP_API bool Reload(HWND hwnd);

	EASYCEF_EXP_API bool ReloadIgnoreCache(HWND hwnd);

	//主线程调用
	EASYCEF_EXP_API bool IsAudioMuted(HWND hwnd);

	EASYCEF_EXP_API void SetAudioMuted(HWND hwnd, bool bMute);

	EASYCEF_EXP_API bool Stop(HWND hwnd);

	//尽量不用这个，容易出问题（请只在控件模式下使用
	EASYCEF_EXP_API void AdjustRenderSpeed(HWND hWnd, double dbSpeed);

	EASYCEF_EXP_API void ClearBrowserData(int combType);

	//注入脚本，注入页面所有框架
	EASYCEF_EXP_API void InjectJS(HWND hwnd, LPCWSTR js);

};




