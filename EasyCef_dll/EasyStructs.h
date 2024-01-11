#pragma once

#include <functional>
#include <unordered_set>

struct EasyCefFunctionFlag
{
	bool bUIImeFollow = false;//输入法跟随光标，目前在UI界面上使用可能有bug
	bool bEnableHignDpi = true;
	char cLog = 0;

};

enum class PreferredAppMode
{
	Default,
	AllowDark,
	ForceDark,
	ForceLight,
	Max
};


//全局变量
struct BrowserGlobalVar
{
	bool Debug = false;
	bool BrowserExist = false;
	bool SupportLayerWindow = false;
	bool IsBrowserProcess = false;

	DWORD WindowsVerMajor = 0;
	DWORD WindowsVerMinor = 0;
	DWORD WindowsVerBuild = 0;


	HINSTANCE hInstance = nullptr;
	HINSTANCE hDllInstance = nullptr;

	void* sandbox_info = nullptr;

	std::function<void()> funcCloseCallback;

	std::function<void(float)> funcSpeedupCallback;

	std::function<void(LPCSTR, LPCSTR)> funcSetHostResolverWork;

	std::function<bool(LPCWSTR, LPCWSTR, BYTE**, DWORD*)> funcXpackExtract;

	std::function<void(BYTE*)> funcXpackFreeData;

	using FuncAddContextMenu = void(*)(void* ,int, LPCWSTR, bool, bool);
	std::function<void(HWND hWnd, int x, int y, bool bIsEdit, FuncAddContextMenu call, void* ext)> funcBeforeContextMenu;

	std::function<bool(HWND, int)> funcDoMenuCommand;

	std::function<bool(HWND, int, LPCWSTR, LPCWSTR, bool)> funcLoadErrorCallback;

	std::function<void(HWND, LPCWSTR, LPCWSTR, bool)> funcCallNativeCompleteStatus;

	std::function<void(HWND, LPCWSTR, LPCWSTR, LPCWSTR)> funcCallDOMCompleteStatus;

	std::function<void(HWND, LPCWSTR, LPCWSTR)> funcPopNewUrlCallback;

	std::function<bool(HWND, unsigned int, LPCWSTR, LPCWSTR)> funcBeforeDownloadCallback;

	std::function<bool(unsigned int, long long, long long)> funcDownloadStatusCallback;


	//控件常用回调
	std::function<void(int, HWND)> funcWebControlCreated;
	std::function<void(int, bool, bool, bool)> funcWebControLoadingState;
	std::function<void(int, LPCWSTR)> funcWebControlUrlChange;
	std::function<void(int, LPCWSTR)> funcWebControlTitleChange;
	std::function<void(int, LPCWSTR, bool*)> funcWebControlLoadBegin;
	std::function<void(int)> funcWebControlLoadEnd;
	std::function<void(int, LPCWSTR)> funcWebControlFavIconChange;
	std::function<void(int)> funcWebControlBeforeClose;
	std::function<bool(int, LPCWSTR, HWND*)> funcWebControlBeforePopup;


	PreferredAppMode DarkModeType = PreferredAppMode::Default;

	//仅限UI窗口时使用
	std::wstring WebViewClassName = L"EasyCefUIClass";
	std::wstring UILoadingWindowTitle = L"EasyUI Loading...";
	std::wstring UserAgent;

	std::wstring FilePath; //exe文件完整路径
	std::wstring DllPath;  //dll文件完整路径，如果不是dll则同上

	std::wstring FileDir;  //尾部有\ 的

	std::wstring ExecName;//执行进程名

	std::wstring CachePath;

	std::wstring FlashPluginPath;

	std::wstring BrowserSettingsPath;


	EasyCefFunctionFlag FunctionFlag;


	std::unordered_set<std::wstring> listAllowUnsecureDomains;

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


class EasyCefRect : public CefRect
{
public:

	EasyCefRect() = default;

	EasyCefRect(int x, int y, int width, int height) {
		Set(x, y, width, height);
	}

	EasyCefRect(const CefRect& rc) :CefRect(rc) {
	}

	operator RECT()	const
	{
		return { x, y, x + width, y + height };
	}

	void CopyRect(const RECT* lprcSrc) {
		Set(lprcSrc->left, lprcSrc->top, lprcSrc->right - lprcSrc->left, lprcSrc->bottom - lprcSrc->top);
	}

};




extern BrowserGlobalVar g_BrowserGlobalVar;


enum EXTRAKEYNAMES
{
	IpcBrowserServer,
	IsManagedPop,
	UIWndHwnd,
	EnableHighDpi,
	RegSyncJSFunctions,
	RegAsyncJSFunctions,

	IPC_RenderUtilityNetwork,
};

extern const char* const ExtraKeyNames[];



#define WIDE_STR(x) WIDE_STR2(x)
#define WIDE_STR2(x) L##x
#define EASYCEFSCHEME "easycef"
#define EASYCEFPROTOCOL EASYCEFSCHEME"://"

#define EASYCEFSCHEMEW WIDE_STR(EASYCEFSCHEME)
#define EASYCEFPROTOCOLW WIDE_STR(EASYCEFPROTOCOL)

#define PACKSCHEME "xpack"
#define PACKPROTOCOL PACKSCHEME"://"

#define MIME_HTML "text/html"

