﻿#pragma once


struct EasyCefFunctionFlag
{
	bool bUIImeFollow = false;//输入法跟随光标，目前在UI界面上使用可能有bug
	char cLog = 0;

};


//全局变量
struct BrowserGlobalVar
{
	typedef void(*_fun_allvoid)();

	bool Debug = false;
	bool BrowserExist = false;
	bool SupportLayerWindow = false;
	bool IsBrowserProcess = false;


	HINSTANCE hInstance = nullptr;
	HINSTANCE hDllInstance = nullptr;

	void* sandbox_info = nullptr;

	//std::string IpcServerName;	//主浏览器进程的

	void* funcCloseCallback = nullptr;

	void* funcSpeedupCallback = nullptr;

	HWND hWndHidden = nullptr;	//隐藏窗口，用于部分处理

	//仅限分层窗口时使用
	std::wstring WebViewClassName = L"EasyCefUIClass";
	std::wstring UserAgent;

	std::wstring FilePath; //exe文件完整路径
	std::wstring DllPath;  //dll文件完整路径，如果不是dll则同上

	std::wstring FileDir;  //尾部有\ 的

	std::wstring ExecName;//执行进程名

	std::wstring CachePath;

	std::wstring FlashPluginPath;

	std::wstring BrowserSettingsPath;


	EasyCefFunctionFlag FunctionFlag;

} ;

#define STU_WEBVIEWEXTATTR
struct WebViewExtraAttr
{
	bool taskbar = false;
	bool transparent = false;
	bool resvr = false;
	unsigned char alpha = 255; //transparent 为true时有效

	//int legacydata;
	char windowinitstatus = 0;
	DWORD dwAddStyle = 0;
	DWORD dwAddExStyle = 0;
};





extern BrowserGlobalVar g_BrowserGlobalVar;

extern const char* ExtraKeyNameIsManagedPopup;
extern const char* ExtraKeyNameIsUIBrowser;
