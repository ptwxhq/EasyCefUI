#pragma once


struct EasyCefFunctionFlag
{
	bool bUIImeFollow = false;//输入法跟随光标，目前在UI界面上使用可能有bug
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
	typedef void(*_fun_allvoid)();

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

	using CloseCallback = void(*)();
	CloseCallback funcCloseCallback;

	using SpeedupCallback = void(*)(float);
	SpeedupCallback funcSpeedupCallback;

	using XpackExtract = bool(*)(LPCWSTR lpszPackFile, LPCWSTR lpszFileItem, BYTE** ppOut, DWORD* pLen);
	XpackExtract funcXpackExtract;

	using XpackFreeData = void(*)(BYTE* pOut);
	XpackFreeData funcXpackFreeData;

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

extern const char* ExtraKeyNameIsManagedPopup;
extern const char* ExtraKeyNameIsUIBrowser;
extern const char* ExtraKeyNameUIWndHwnd;


#define WIDE_STR(x) WIDE_STR2(x)
#define WIDE_STR2(x) L##x
#define EASYCEFSCHEME "easycef"
#define EASYCEFPROTOCOL EASYCEFSCHEME"://"

#define EASYCEFSCHEMEW WIDE_STR(EASYCEFSCHEME)
#define EASYCEFPROTOCOLW WIDE_STR(EASYCEFPROTOCOL)

#define PACKSCHEME "xpack"
#define PACKPROTOCOL PACKSCHEME"://"

#define MIME_HTML "text/html"

