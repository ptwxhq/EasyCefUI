#include "pch.h"

//#include <tlhelp32.h>
#include <Psapi.h>
#include <Winternl.h>
#include "Utility.h"

#include <ShlObj_core.h>

#include "include/base/cef_logging.h"
#include <random>
#include <shellscalingapi.h>
#include "EasyBrowserWorks.h"
#include "EasyIPC.h"


void GetLocalPaths();

namespace {

    LARGE_INTEGER qi_freq_ = {};

}  // namespace

uint64_t GetTimeNow() {
    if (!qi_freq_.HighPart && !qi_freq_.LowPart) {
        QueryPerformanceFrequency(&qi_freq_);
    }
    LARGE_INTEGER t = {};
    QueryPerformanceCounter(&t);
    return static_cast<uint64_t>((t.QuadPart / double(qi_freq_.QuadPart)) *
        1000000);
}

uint64_t GetTimeNowMS(int Offset)
{
    if (!qi_freq_.HighPart && !qi_freq_.LowPart) {
        QueryPerformanceFrequency(&qi_freq_);
    }
    LARGE_INTEGER t = {};
    QueryPerformanceCounter(&t);
    return static_cast<uint64_t>((t.QuadPart / double(qi_freq_.QuadPart)) * 1000 + Offset);
}

void SetUserDataPtr(HWND hWnd, void* ptr) {
    SetLastError(ERROR_SUCCESS);
    LONG_PTR result =
        ::SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(ptr));
    CHECK(result != 0 || GetLastError() == ERROR_SUCCESS);
}

WNDPROC SetWndProcPtr(HWND hWnd, WNDPROC wndProc) {
    WNDPROC old =
        reinterpret_cast<WNDPROC>(::GetWindowLongPtr(hWnd, GWLP_WNDPROC));
    CHECK(old != NULL);
    LONG_PTR result = ::SetWindowLongPtr(hWnd, GWLP_WNDPROC,
        reinterpret_cast<LONG_PTR>(wndProc));
    CHECK(result != 0 || GetLastError() == ERROR_SUCCESS);
    return old;
}

std::wstring GetResourceString(UINT id) {
#define MAX_LOADSTRING 100
    TCHAR buff[MAX_LOADSTRING] = { 0 };
    LoadString(::GetModuleHandle(NULL), id, buff, MAX_LOADSTRING);
    return buff;
}

int GetCefMouseModifiers(WPARAM wparam) {
    int modifiers = 0;
    if (wparam & MK_CONTROL)
        modifiers |= EVENTFLAG_CONTROL_DOWN;
    if (wparam & MK_SHIFT)
        modifiers |= EVENTFLAG_SHIFT_DOWN;
    if (IsKeyDown(VK_MENU))
        modifiers |= EVENTFLAG_ALT_DOWN;
    if (wparam & MK_LBUTTON)
        modifiers |= EVENTFLAG_LEFT_MOUSE_BUTTON;
    if (wparam & MK_MBUTTON)
        modifiers |= EVENTFLAG_MIDDLE_MOUSE_BUTTON;
    if (wparam & MK_RBUTTON)
        modifiers |= EVENTFLAG_RIGHT_MOUSE_BUTTON;

    // Low bit set from GetKeyState indicates "toggled".
    if (::GetKeyState(VK_NUMLOCK) & 1)
        modifiers |= EVENTFLAG_NUM_LOCK_ON;
    if (::GetKeyState(VK_CAPITAL) & 1)
        modifiers |= EVENTFLAG_CAPS_LOCK_ON;
    return modifiers;
}

int GetCefKeyboardModifiers(WPARAM wparam, LPARAM lparam) {
    int modifiers = 0;
    if (IsKeyDown(VK_SHIFT))
        modifiers |= EVENTFLAG_SHIFT_DOWN;
    if (IsKeyDown(VK_CONTROL))
        modifiers |= EVENTFLAG_CONTROL_DOWN;
    if (IsKeyDown(VK_MENU))
        modifiers |= EVENTFLAG_ALT_DOWN;

    // Low bit set from GetKeyState indicates "toggled".
    if (::GetKeyState(VK_NUMLOCK) & 1)
        modifiers |= EVENTFLAG_NUM_LOCK_ON;
    if (::GetKeyState(VK_CAPITAL) & 1)
        modifiers |= EVENTFLAG_CAPS_LOCK_ON;

    switch (wparam) {
    case VK_RETURN:
        if ((lparam >> 16) & KF_EXTENDED)
            modifiers |= EVENTFLAG_IS_KEY_PAD;
        break;
    case VK_INSERT:
    case VK_DELETE:
    case VK_HOME:
    case VK_END:
    case VK_PRIOR:
    case VK_NEXT:
    case VK_UP:
    case VK_DOWN:
    case VK_LEFT:
    case VK_RIGHT:
        if (!((lparam >> 16) & KF_EXTENDED))
            modifiers |= EVENTFLAG_IS_KEY_PAD;
        break;
    case VK_NUMLOCK:
    case VK_NUMPAD0:
    case VK_NUMPAD1:
    case VK_NUMPAD2:
    case VK_NUMPAD3:
    case VK_NUMPAD4:
    case VK_NUMPAD5:
    case VK_NUMPAD6:
    case VK_NUMPAD7:
    case VK_NUMPAD8:
    case VK_NUMPAD9:
    case VK_DIVIDE:
    case VK_MULTIPLY:
    case VK_SUBTRACT:
    case VK_ADD:
    case VK_DECIMAL:
    case VK_CLEAR:
        modifiers |= EVENTFLAG_IS_KEY_PAD;
        break;
    case VK_SHIFT:
        if (IsKeyDown(VK_LSHIFT))
            modifiers |= EVENTFLAG_IS_LEFT;
        else if (IsKeyDown(VK_RSHIFT))
            modifiers |= EVENTFLAG_IS_RIGHT;
        break;
    case VK_CONTROL:
        if (IsKeyDown(VK_LCONTROL))
            modifiers |= EVENTFLAG_IS_LEFT;
        else if (IsKeyDown(VK_RCONTROL))
            modifiers |= EVENTFLAG_IS_RIGHT;
        break;
    case VK_MENU:
        if (IsKeyDown(VK_LMENU))
            modifiers |= EVENTFLAG_IS_LEFT;
        else if (IsKeyDown(VK_RMENU))
            modifiers |= EVENTFLAG_IS_RIGHT;
        break;
    case VK_LWIN:
        modifiers |= EVENTFLAG_IS_LEFT;
        break;
    case VK_RWIN:
        modifiers |= EVENTFLAG_IS_RIGHT;
        break;
    }
    return modifiers;
}

bool IsKeyDown(WPARAM wparam) {
    return (GetKeyState(wparam) & 0x8000) != 0;
}

float GetDeviceScaleFactor() {
    static float scale_factor = 1.0;
    static bool initialized = false;

    if (!initialized) {
        // This value is safe to cache for the life time of the app since the user
        // must logout to change the DPI setting. This value also applies to all
        // screens.
        HDC screen_dc = ::GetDC(NULL);
        int dpi_x = GetDeviceCaps(screen_dc, LOGPIXELSX);
        scale_factor = static_cast<float>(dpi_x) / DPI_1X;
        ::ReleaseDC(NULL, screen_dc);
        initialized = true;
    }

    return scale_factor;
}


int LogicalToDevice(int value, float device_scale_factor) {
    float scaled_val = static_cast<float>(value) * device_scale_factor;
    return static_cast<int>(std::floor(scaled_val));
}

CefRect LogicalToDevice(const CefRect& value, float device_scale_factor) {
    return CefRect(LogicalToDevice(value.x, device_scale_factor),
        LogicalToDevice(value.y, device_scale_factor),
        LogicalToDevice(value.width, device_scale_factor),
        LogicalToDevice(value.height, device_scale_factor));
}

int DeviceToLogical(int value, float device_scale_factor) {
    float scaled_val = static_cast<float>(value) / device_scale_factor;
    return static_cast<int>(std::floor(scaled_val));
}

void DeviceToLogical(CefMouseEvent& value, float device_scale_factor) {
    value.x = DeviceToLogical(value.x, device_scale_factor);
    value.y = DeviceToLogical(value.y, device_scale_factor);
}

// Returns true if the process is per monitor DPI aware.
bool IsProcessPerMonitorDpiAware() {
    enum class PerMonitorDpiAware {
        UNKNOWN = 0,
        PER_MONITOR_DPI_UNAWARE,
        PER_MONITOR_DPI_AWARE,
    };
    static PerMonitorDpiAware per_monitor_dpi_aware = PerMonitorDpiAware::UNKNOWN;
    if (per_monitor_dpi_aware == PerMonitorDpiAware::UNKNOWN) {
        per_monitor_dpi_aware = PerMonitorDpiAware::PER_MONITOR_DPI_UNAWARE;
        HMODULE shcore_dll = ::LoadLibrary(L"shcore.dll");
        if (shcore_dll) {
            typedef HRESULT(WINAPI* GetProcessDpiAwarenessPtr)(
                HANDLE, PROCESS_DPI_AWARENESS*);
            GetProcessDpiAwarenessPtr func_ptr =
                reinterpret_cast<GetProcessDpiAwarenessPtr>(
                    ::GetProcAddress(shcore_dll, "GetProcessDpiAwareness"));
            if (func_ptr) {
                PROCESS_DPI_AWARENESS awareness;
                if (SUCCEEDED(func_ptr(nullptr, &awareness)) &&
                    awareness == PROCESS_PER_MONITOR_DPI_AWARE)
                    per_monitor_dpi_aware = PerMonitorDpiAware::PER_MONITOR_DPI_AWARE;
            }
        }
    }
    return per_monitor_dpi_aware == PerMonitorDpiAware::PER_MONITOR_DPI_AWARE;
}


float GetWindowScaleFactor(HWND hwnd) {

    if (!g_BrowserGlobalVar.FunctionFlag.bEnableHignDpi)
    {
        return 1.f;
    }

    if (hwnd && IsProcessPerMonitorDpiAware()) {
        typedef UINT(WINAPI* GetDpiForWindowPtr)(HWND);
        static GetDpiForWindowPtr func_ptr = reinterpret_cast<GetDpiForWindowPtr>(
            GetProcAddress(GetModuleHandle(L"user32.dll"), "GetDpiForWindow"));
        if (func_ptr)
            return static_cast<float>(func_ptr(hwnd)) / DPI_1X;
    }

    return GetDeviceScaleFactor();
}


std::string QuickMakeIpcParms(int BrowserId, int64_t FrameId, uint64_t timeout, const std::string& name, const CefRefPtr<CefListValue>& valueList)
{
    CefRefPtr<CefValue> forsend = CefValue::Create();
    CefRefPtr<CefDictionaryValue> dict = CefDictionaryValue::Create();

    dict->SetInt("bid", BrowserId);
    dict->SetString("fid", std::to_string(FrameId));
    dict->SetString("name", name);
    dict->SetList("arg", valueList);
    dict->SetString("to", std::to_string(timeout));

    forsend->SetDictionary(dict);

    return CefWriteJSON(forsend, JSON_WRITER_DEFAULT).ToString();
}

bool QuickGetIpcParms(const std::string& strData, int& BrowserId, int64_t& FrameId, uint64_t& timeout, std::string& name, CefRefPtr<CefListValue>& valueList)
{
    auto recVal = CefParseJSON(strData, JSON_PARSER_RFC);
    if (!recVal)
        return false;

    auto dict = recVal->GetDictionary();

    if (!dict)
        return false;

    if (dict->HasKey("bid"))
    {
        BrowserId = dict->GetInt("bid");
    }
    else
    {
        BrowserId = -1;
    }

    if (dict->HasKey("fid"))
    {
        FrameId = _atoi64(dict->GetString("fid").ToString().c_str());
    }
    else
    {
        FrameId = -1;
    }

    if (dict->HasKey("to"))
    {
        const auto val = dict->GetString("to").ToString();
        timeout = strtoull(val.c_str(), nullptr, 10);
    }
    else
    {
        timeout = 0;
    }

    if (dict->HasKey("name"))
    {
        name = dict->GetString("name");
    }
    else
    {
        name.clear();
    }

    if (dict->HasKey("arg"))
    {
        valueList = dict->GetList("arg")->Copy();
    }
    else
    {
        valueList->Clear();
    }

    return true;
}

bool GetParentProcessInfo(DWORD *pdwId, std::wstring* pstrPathOut)
{
    static DWORD dwProcessId = 0;
    static std::wstring strPath;
    static bool bFirstRun = true;
    if (bFirstRun)
    {
        bFirstRun = false;
        typedef NTSTATUS(WINAPI* pfnNtQueryInformationProcess)(HANDLE, UINT, PVOID, ULONG, PULONG);
        pfnNtQueryInformationProcess _NtQueryInformationProcess = nullptr;
        PROCESS_BASIC_INFORMATION pbi = {};
        auto ntdll = GetModuleHandleA("ntdll");
        if (ntdll)
            _NtQueryInformationProcess = (pfnNtQueryInformationProcess)GetProcAddress(ntdll, "NtQueryInformationProcess");
        if (_NtQueryInformationProcess)
        {
            HANDLE hProcessSelf = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, GetCurrentProcessId());

            NTSTATUS status = _NtQueryInformationProcess(hProcessSelf,
                ProcessBasicInformation,
                (PVOID)&pbi,
                sizeof(PROCESS_BASIC_INFORMATION),
                NULL
            );
            CloseHandle(hProcessSelf);
            if (0 == status)
            {
                dwProcessId = (DWORD)pbi.Reserved3;

                HANDLE hProcessPar = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, dwProcessId);
                if (hProcessPar != INVALID_HANDLE_VALUE)
                {
                    DWORD dwLen = MAXSHORT;
                    auto strPathBuf = new WCHAR[dwLen];
                    if (QueryFullProcessImageNameW(hProcessPar, 0, strPathBuf, &dwLen))
                    {
                        strPath = strPathBuf;
                    }
                    delete[] strPathBuf;

                    CloseHandle(hProcessPar);
                }
            }
            
        }
        
    }

    if (pstrPathOut)
    {
        *pstrPathOut = strPath;
    }

    if (pdwId)
    {
        *pdwId = dwProcessId;
    }


    return !!dwProcessId;
}

std::string GetRandomString(size_t length)
{
    static const char CHARACTERS[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

    std::random_device random_device;
    std::mt19937 generator(random_device());
    std::uniform_int_distribution<> distribution(0, _countof(CHARACTERS) - 2);

    std::string random_string(length, 0);

    for (auto& ch : random_string)
    {
        ch = CHARACTERS[distribution(generator)];
    }

    return random_string;
}

std::string GetDataURI(const std::string& data, const std::string& mime_type) {
    return "data:" + mime_type + ";base64," +
        CefURIEncode(CefBase64Encode(data.data(), data.size()), false)
        .ToString();
}

std::string GetInternalPage(const std::string& data)
{
    return EASYCEFPROTOCOL "info/" +
        CefURIEncode(CefBase64Encode(data.data(), data.size()), false)
        .ToString();
}

std::wstring GetDefAppDataFolder()
{
    static std::wstring strPath;
    if (strPath.empty())
    {
        WCHAR szAppData[MAX_PATH];
        SHGetSpecialFolderPathW(NULL, szAppData, CSIDL_APPDATA, TRUE);
        strPath = szAppData;

        if (g_BrowserGlobalVar.ExecName.empty())
        {
            GetLocalPaths();
        }

        auto pPos = g_BrowserGlobalVar.ExecName.rfind('.');
        auto strFileName = g_BrowserGlobalVar.ExecName.substr(0, pPos);

        strPath += L'\\' + strFileName;

    }

    return strPath;
}

CefRefPtr<CefV8Value> CefValueToCefV8Value(CefRefPtr<CefValue> value) 
{
    CefRefPtr<CefV8Value> result;
    switch (value->GetType()) {
    case VTYPE_INVALID:
        {
            //std::cout << "Type: VTYPE_INVALID" << std::endl;
            result = CefV8Value::CreateNull();
        }
        break;
    case VTYPE_NULL:
        {
            //std::cout << "Type: VTYPE_NULL" << std::endl;
            result = CefV8Value::CreateNull();
        }
        break;
    case VTYPE_BOOL:
        {
            //std::cout << "Type: VTYPE_BOOL" << std::endl;
            result = CefV8Value::CreateBool(value->GetBool());
        }
        break;
    case VTYPE_INT:
        {
            //std::cout << "Type: VTYPE_INT" << std::endl;
            result = CefV8Value::CreateInt(value->GetInt());
        }
        break;
    case VTYPE_DOUBLE:
        {
            //std::cout << "Type: VTYPE_DOUBLE" << std::endl;
            result = CefV8Value::CreateDouble(value->GetDouble());
        }
        break;
    case VTYPE_STRING:
        {
            //std::cout << "Type: VTYPE_STRING" << std::endl;
            result = CefV8Value::CreateString(value->GetString());
        }
        break;
    case VTYPE_BINARY:
        {
            //std::cout << "Type: VTYPE_BINARY" << std::endl;
            result = CefV8Value::CreateNull();
        }
        break;
    case VTYPE_DICTIONARY:
        {
            //std::cout << "Type: VTYPE_DICTIONARY" << std::endl;
            result = CefV8Value::CreateObject(nullptr, nullptr);
            CefRefPtr<CefDictionaryValue> dict = value->GetDictionary();
            CefDictionaryValue::KeyList keys;
            dict->GetKeys(keys);
            for (size_t i = 0; i < keys.size(); ++i) {
                CefString key = keys[i];
                result->SetValue(key, CefValueToCefV8Value(dict->GetValue(key)), V8_PROPERTY_ATTRIBUTE_NONE);
            }
        }
        break;
    case VTYPE_LIST:
        {
            //std::cout << "Type: VTYPE_LIST" << std::endl;
            CefRefPtr<CefListValue> list = value->GetList();
            int size = list->GetSize();
            result = CefV8Value::CreateArray(size);
            for (int i = 0; i < size; ++i) {
                result->SetValue(i, CefValueToCefV8Value(list->GetValue(i)));
            }
        }
        break;
    }
    return result;
}


CefRefPtr<CefValue> CefV8ValueToCefValue(CefRefPtr<CefV8Value> value)
{
    CefRefPtr<CefValue> result = CefValue::Create();

    if (value->IsBool())
    {
        result->SetBool(value->GetBoolValue());
    }
    else if (value->IsInt())
    {
        result->SetInt(value->GetIntValue());
    }
    else if (value->IsUInt())
    {
        result->SetDouble(value->GetUIntValue());
    }
    else if (value->IsDouble())
    {
        result->SetDouble(value->GetDoubleValue());
    }
    else if (value->IsString())
    {
        result->SetString(value->GetStringValue());
    }
    else if (value->IsArray())
    {
        auto list = CefListValue::Create();
        for (int i = 0; i < value->GetArrayLength(); i++)
        {
            list->SetValue(i, CefV8ValueToCefValue(value->GetValue(i)));
        }
        result->SetList(list);
    }
    else if (value->IsObject())
    {
        auto dict = CefDictionaryValue::Create();
        std::vector<CefString> keys;
        value->GetKeys(keys);
        for (auto it: keys)
        {
            dict->SetValue(it, CefV8ValueToCefValue(value->GetValue(it)));
        }
        result->SetDictionary(dict);
    }
    else
    {
        result = CefValue::Create();
        result->SetNull();
    }

    return result;
}

CefString CefV8ValueToString(CefRefPtr<CefV8Value> value)
{
    auto toVal = CefV8ValueToCefValue(value);

    return CefWriteJSON(toVal, JSON_WRITER_DEFAULT);
}

CefRefPtr<CefDictionaryValue> CreateExtraInfo(bool bIsManagedPop, HWND hUIHwnd)
{
    auto extra_info = CefDictionaryValue::Create();

    {
        const auto tmpVal = EasyIPCServer::GetInstance().GetHandle();
        auto valKey = CefBinaryValue::Create(&tmpVal, sizeof(tmpVal));
        extra_info->SetBinary(ExtraKeyNames[IpcBrowserServer], valKey);
    }

    if (!bIsManagedPop)
    {
        auto valKey = CefBinaryValue::Create(&hUIHwnd, sizeof(hUIHwnd));
        extra_info->SetBinary(ExtraKeyNames[UIWndHwnd], valKey);
    }
 
    extra_info->SetBool(ExtraKeyNames[IsManagedPop], bIsManagedPop);
    extra_info->SetBool(ExtraKeyNames[EnableHighDpi], g_BrowserGlobalVar.FunctionFlag.bEnableHignDpi);

    extra_info->SetDictionary(ExtraKeyNames[RegSyncJSFunctions], EasyBrowserWorks::GetInstance().GetUserJSFunction(true));
    extra_info->SetDictionary(ExtraKeyNames[RegAsyncJSFunctions], EasyBrowserWorks::GetInstance().GetUserJSFunction(false));

    return extra_info;
}

void SetRequestDefaultSettings(CefRefPtr<CefRequestContext> request_context)
{
    if (!request_context)
        return;

    CefString errstr;

#if CEF_VERSION_MAJOR <= 87


    auto val1 = CefValue::Create();
    val1->SetInt(1);
    auto valtrue = CefValue::Create();
    valtrue->SetBool(true);
    request_context->SetPreference("plugins.run_all_flash_in_allow_mode", valtrue, errstr);
    request_context->SetPreference("profile.default_content_setting_values.plugins", val1, errstr);

    request_context->SetPreference("plugins.allow_outdated", valtrue, errstr);
    request_context->SetPreference("webkit.webprefs.plugins_enabled", valtrue, errstr);


#ifndef _DEBUG

    auto val = CefValue::Create();
    auto inside = CefListValue::Create();
    inside->SetString(0, "ui.pack");
    val->SetList(inside);

    /*
    debug的libcef.dll由于有一个Check使得profile.managed_plugins_allowed_for_urls不可作为用于定义配置，release没问题
    flash相关的不想再多花时间了，能实现就凑合就这样了
    */
    request_context->SetPreference("profile.managed_plugins_allowed_for_urls", val, errstr);


#endif // !_DEBUG



#endif
}

void SetAllowDarkMode(int nValue)
{
    if (nValue <= 0 || nValue >= 3)
        nValue = 0;

    if (nValue == 1)
        g_BrowserGlobalVar.DarkModeType = PreferredAppMode::AllowDark;
    else if (nValue == 2)
        g_BrowserGlobalVar.DarkModeType = PreferredAppMode::ForceDark;
    else if (nValue == 3)
        g_BrowserGlobalVar.DarkModeType = PreferredAppMode::ForceLight;

    const auto DarkModeForApp = [] (bool bNewVer, PreferredAppMode eMode)
    {
        HMODULE hUxtheme = LoadLibraryExW(L"uxtheme.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);

        if (hUxtheme)
        {
            auto ord135 = GetProcAddress(hUxtheme, MAKEINTRESOURCEA(135));

            if (bNewVer)
            {
                // 1903 18362

                using fnSetPreferredAppMode = PreferredAppMode(WINAPI*)(PreferredAppMode appMode); // ordinal 135, in 1903
                auto _SetPreferredAppMode = reinterpret_cast<fnSetPreferredAppMode>(ord135);
                if (_SetPreferredAppMode)
                    _SetPreferredAppMode(eMode);
            }
            else
            {
                using fnAllowDarkModeForApp = bool (WINAPI*)(bool allow); // ordinal 135, in 1809

                auto _AllowDarkModeForApp = reinterpret_cast<fnAllowDarkModeForApp>(ord135);

                if (_AllowDarkModeForApp)
                    _AllowDarkModeForApp(true);
            }
        }
    };
    

    if (g_BrowserGlobalVar.WindowsVerMajor == 10 && g_BrowserGlobalVar.WindowsVerMinor == 0)
    {
        if (g_BrowserGlobalVar.WindowsVerBuild >= 18362)
        {
            DarkModeForApp(true, g_BrowserGlobalVar.DarkModeType);
        }
        else if (g_BrowserGlobalVar.WindowsVerBuild >= 17763)
        {
            DarkModeForApp(false, g_BrowserGlobalVar.DarkModeType);
        }
        else
        {
            g_BrowserGlobalVar.DarkModeType = PreferredAppMode::ForceLight;
        }
    }
    else
    {
        g_BrowserGlobalVar.DarkModeType = PreferredAppMode::ForceLight;
    }
    
}

bool IsSystemWindows7OrOlder()
{
    if (g_BrowserGlobalVar.WindowsVerMajor < 6 || (g_BrowserGlobalVar.WindowsVerMajor == 6 && g_BrowserGlobalVar.WindowsVerMinor < 2))
    {
        return true;
    }
    return false;
}

bool IsSystemWindows11OrGreater()
{
    if (g_BrowserGlobalVar.WindowsVerMajor >= 10 && g_BrowserGlobalVar.WindowsVerBuild >= 22000)
    {
        return true;
    }
    return false;
}

bool ReplaceSubstr(const std::string& input, const std::string& search, const std::string& replace, std::string& output)
{
    output.clear();
    auto itPos = input.begin();
    auto itInBegin = itPos;

    auto inEnd = input.end();

    bool bFound = false;

    while (true)
    {
        itPos = std::search(itPos, inEnd, search.begin(), search.end());

        output.append(itInBegin, itPos);

        if (itPos == inEnd)
        {
            break;
        }

        bFound = true;

        output += replace;

        itPos += search.length();

        itInBegin = itPos;
    }

    return bFound;
}

bool ReplaceSubstrCaseinsensitive(const std::string& input, std::string search, const std::string& replace, std::string& output)
{
    output.clear();
    std::transform(search.begin(), search.end(), search.begin(), tolower);

    auto itPos = input.begin();
    auto itInBegin = itPos;

    auto inEnd = input.end();

    bool bFound = false;

    while (true)
    {
        itPos = std::search(itPos, inEnd, search.begin(), search.end(),
            [](const char c1, const char c2) {
                return tolower(c1) == c2;
            }
        );

        output.append(itInBegin, itPos);

        if (itPos == inEnd)
        {
            break;
        }

        bFound = true;

        output += replace;

        itPos += search.length();

        itInBegin = itPos;
    }

    return bFound;
}

bool ReplaceAllSubString(bool bCaseinsensitive, const std::string& input, const std::string search, const std::string& replace, std::string& output)
{
    if (bCaseinsensitive)
    {
        return ReplaceSubstrCaseinsensitive(input, search, replace, output);
    }
    else
    {
        return ReplaceSubstr(input, search, replace, output);
    }
}

std::string ArrangeJsonString(std::string strParam)
{
    const auto trim = [](const std::string& s)
    {
        const auto check = [](int c) {
            return !(c < -1 || c > 255 || std::isprint(c));
        };

        auto wsfront = std::find_if_not(s.begin(), s.end(), check);
        auto wsback = std::find_if_not(s.rbegin(), s.rend(), check).base();
        return (wsback <= wsfront ? std::string() : std::string(wsfront, wsback));
    };

    strParam = trim(strParam);

    auto JsonParm = CefValue::Create();
    JsonParm->SetString(strParam);
    strParam = CefWriteJSON(JsonParm, JSON_WRITER_DEFAULT);

    return strParam;
}



// Returns |url| without the query or fragment components, if any.
std::wstring GetUrlWithoutQueryOrFragment(const std::wstring& url) {
    // Find the first instance of '?' or '#'.
    const size_t pos = std::min(url.find('?'), url.find('#'));
    if (pos != std::string::npos)
        return url.substr(0, pos);

    return url;
}

namespace webinfo {

std::string GetTimeString(const CefTime& value) {
    if (value.GetTimeT() == 0)
        return "Unspecified";

    static const char* kMonths[] = {
        "January", "February", "March",     "April",   "May",      "June",
        "July",    "August",   "September", "October", "November", "December" };
    std::string month;
    if (value.month >= 1 && value.month <= 12)
        month = kMonths[value.month - 1];
    else
        month = "Invalid";


    return std::format("{} {}, {} {:02}:{:02}:{:02}", month, value.day_of_month, value.year, value.hour, value.minute, value.second);

}

#if CEF_VERSION_MAJOR > 104

std::string GetTimeString(const CefBaseTime& value) {
    CefTime time;
    if (cef_time_from_basetime(value, &time)) {
        return GetTimeString(time);
    }
    else {
        return "Invalid";
    }
}

#endif

std::string GetBinaryString(CefRefPtr<CefBinaryValue> value) {
    if (!value.get())
        return "&nbsp;";

    // Retrieve the value.
    const size_t size = value->GetSize();
    std::string src;
    src.resize(size);
    value->GetData(const_cast<char*>(src.data()), size, 0);

    // Encode the value.
    return CefBase64Encode(src.data(), src.size());
}

#define FLAG(flag)                          \
  if (status & flag) {                      \
    result += std::string(#flag) + "<br/>"; \
  }

#define VALUE(val, def)       \
  if (val == def) {           \
    return std::string(#def); \
  }

std::string GetCertStatusString(cef_cert_status_t status) {
    std::string result;

    FLAG(CERT_STATUS_COMMON_NAME_INVALID);
    FLAG(CERT_STATUS_DATE_INVALID);
    FLAG(CERT_STATUS_AUTHORITY_INVALID);
    FLAG(CERT_STATUS_NO_REVOCATION_MECHANISM);
    FLAG(CERT_STATUS_UNABLE_TO_CHECK_REVOCATION);
    FLAG(CERT_STATUS_REVOKED);
    FLAG(CERT_STATUS_INVALID);
    FLAG(CERT_STATUS_WEAK_SIGNATURE_ALGORITHM);
    FLAG(CERT_STATUS_NON_UNIQUE_NAME);
    FLAG(CERT_STATUS_WEAK_KEY);
    FLAG(CERT_STATUS_PINNED_KEY_MISSING);
    FLAG(CERT_STATUS_NAME_CONSTRAINT_VIOLATION);
    FLAG(CERT_STATUS_VALIDITY_TOO_LONG);
    FLAG(CERT_STATUS_IS_EV);
    FLAG(CERT_STATUS_REV_CHECKING_ENABLED);
    FLAG(CERT_STATUS_SHA1_SIGNATURE_PRESENT);
    FLAG(CERT_STATUS_CT_COMPLIANCE_FAILED);

    if (result.empty())
        return "&nbsp;";
    return result;
}

std::string GetSSLVersionString(cef_ssl_version_t version) {
    VALUE(version, SSL_CONNECTION_VERSION_UNKNOWN);
    VALUE(version, SSL_CONNECTION_VERSION_SSL2);
    VALUE(version, SSL_CONNECTION_VERSION_SSL3);
    VALUE(version, SSL_CONNECTION_VERSION_TLS1);
    VALUE(version, SSL_CONNECTION_VERSION_TLS1_1);
    VALUE(version, SSL_CONNECTION_VERSION_TLS1_2);
    VALUE(version, SSL_CONNECTION_VERSION_TLS1_3);
    VALUE(version, SSL_CONNECTION_VERSION_QUIC);
    return std::string();
}

std::string GetContentStatusString(cef_ssl_content_status_t status) {
    std::string result;

    VALUE(status, SSL_CONTENT_NORMAL_CONTENT);
    FLAG(SSL_CONTENT_DISPLAYED_INSECURE_CONTENT);
    FLAG(SSL_CONTENT_RAN_INSECURE_CONTENT);

    if (result.empty())
        return "&nbsp;";
    return result;
}

std::string GetCertificateInformation(const std::string& url,
    CefRefPtr<CefX509Certificate> cert,
    cef_cert_status_t certstatus) {
    CefRefPtr<CefX509CertPrincipal> subject = cert->GetSubject();
    CefRefPtr<CefX509CertPrincipal> issuer = cert->GetIssuer();

    // Build a table showing certificate information. Various types of invalid
    // certificates can be tested using https://badssl.com/.
    std::ostringstream ss;
    ss << "<h3>X.509 Certificate Information:</h3>"
        "<table border=1><tr><th>Field</th><th>Value</th></tr>";

    if (certstatus != CERT_STATUS_NONE) {
        ss << "<tr><td>Status</td><td>" << GetCertStatusString(certstatus)
            << "</td></tr>";
    }

    ss << "<tr><td>Subject</td><td>"
        << (subject.get() ? subject->GetDisplayName().ToString() : "&nbsp;")
        << "</td></tr>"
        "<tr><td>Issuer</td><td>"
        << (issuer.get() ? issuer->GetDisplayName().ToString() : "&nbsp;")
        << "</td></tr>"
        //"<tr><td>Serial #*</td><td>"
        //<< GetBinaryString(cert->GetSerialNumber()) << "</td></tr>"
        << "<tr><td>Valid Start</td><td>" << GetTimeString(cert->GetValidStart())
        << "</td></tr>"
        "<tr><td>Valid Expiry</td><td>"
        << GetTimeString(cert->GetValidExpiry()) << "</td></tr>";

    /*CefX509Certificate::IssuerChainBinaryList der_chain_list;
    CefX509Certificate::IssuerChainBinaryList pem_chain_list;
    cert->GetDEREncodedIssuerChain(der_chain_list);
    cert->GetPEMEncodedIssuerChain(pem_chain_list);
    DCHECK_EQ(der_chain_list.size(), pem_chain_list.size());

    der_chain_list.insert(der_chain_list.begin(), cert->GetDEREncoded());
    pem_chain_list.insert(pem_chain_list.begin(), cert->GetPEMEncoded());

    for (size_t i = 0U; i < der_chain_list.size(); ++i) {
        ss << "<tr><td>DER Encoded*</td>"
            "<td style=\"max-width:800px;overflow:scroll;\">"
            << GetBinaryString(der_chain_list[i])
            << "</td></tr>"
            "<tr><td>PEM Encoded*</td>"
            "<td style=\"max-width:800px;overflow:scroll;\">"
            << GetBinaryString(pem_chain_list[i]) << "</td></tr>";
    }

    ss << "</table> * Displayed value is base64 encoded."; */
    ss << "</table>";

    if (!url.empty())
    {
        ss << R"_raw(<p><input type="button" value="Continue(unsecure)" onclick="nativeapp.__ContinueUnsecure__(')_raw" 
            << url << R"_raw(')"/> <input type="button" value="Go Back" onclick="javascript:history.back();"</p>)_raw";
    }

    

    return ss.str();
}

std::string GetErrorPage(const std::string& failed_url, const std::string& other_info, cef_errorcode_t error_code)
{

    const auto GetErrorString = [](cef_errorcode_t code) ->std::string {
        // Case condition that returns |code| as a string.
#define CASE(code) \
  case code:       \
    return #code

        switch (code) {
            CASE(ERR_NONE);
            CASE(ERR_FAILED);
            CASE(ERR_ABORTED);
            CASE(ERR_INVALID_ARGUMENT);
            CASE(ERR_INVALID_HANDLE);
            CASE(ERR_FILE_NOT_FOUND);
            CASE(ERR_TIMED_OUT);
            CASE(ERR_FILE_TOO_BIG);
            CASE(ERR_UNEXPECTED);
            CASE(ERR_ACCESS_DENIED);
            CASE(ERR_NOT_IMPLEMENTED);
            CASE(ERR_CONNECTION_CLOSED);
            CASE(ERR_CONNECTION_RESET);
            CASE(ERR_CONNECTION_REFUSED);
            CASE(ERR_CONNECTION_ABORTED);
            CASE(ERR_CONNECTION_FAILED);
            CASE(ERR_NAME_NOT_RESOLVED);
            CASE(ERR_INTERNET_DISCONNECTED);
            CASE(ERR_SSL_PROTOCOL_ERROR);
            CASE(ERR_ADDRESS_INVALID);
            CASE(ERR_ADDRESS_UNREACHABLE);
            CASE(ERR_SSL_CLIENT_AUTH_CERT_NEEDED);
            CASE(ERR_TUNNEL_CONNECTION_FAILED);
            CASE(ERR_NO_SSL_VERSIONS_ENABLED);
            CASE(ERR_SSL_VERSION_OR_CIPHER_MISMATCH);
            CASE(ERR_SSL_RENEGOTIATION_REQUESTED);
            CASE(ERR_CERT_COMMON_NAME_INVALID);
            CASE(ERR_CERT_DATE_INVALID);
            CASE(ERR_CERT_AUTHORITY_INVALID);
            CASE(ERR_CERT_CONTAINS_ERRORS);
            CASE(ERR_CERT_NO_REVOCATION_MECHANISM);
            CASE(ERR_CERT_UNABLE_TO_CHECK_REVOCATION);
            CASE(ERR_CERT_REVOKED);
            CASE(ERR_CERT_INVALID);
            CASE(ERR_CERT_END);
            CASE(ERR_INVALID_URL);
            CASE(ERR_DISALLOWED_URL_SCHEME);
            CASE(ERR_UNKNOWN_URL_SCHEME);
            CASE(ERR_TOO_MANY_REDIRECTS);
            CASE(ERR_UNSAFE_REDIRECT);
            CASE(ERR_UNSAFE_PORT);
            CASE(ERR_INVALID_RESPONSE);
            CASE(ERR_INVALID_CHUNKED_ENCODING);
            CASE(ERR_METHOD_NOT_SUPPORTED);
            CASE(ERR_UNEXPECTED_PROXY_AUTH);
            CASE(ERR_EMPTY_RESPONSE);
            CASE(ERR_RESPONSE_HEADERS_TOO_BIG);
            CASE(ERR_CACHE_MISS);
            CASE(ERR_INSECURE_RESPONSE);
        default:
            return "UNKNOWN";
        }
    };


    std::ostringstream ss;
    ss << R"(<html><head><title>Page failed to load</title></head><style>
@media (prefers-color-scheme: light) {
body {
background-color: #f0f0f0;
}}
@media (prefers-color-scheme: dark) {
body {
background-color: #3e3e3e;
color: #fff;
}}
body {
margin:10px 20px;
}
.caption {-webkit-app-region: drag;}
</style><body><h1 class="caption">Page failed to load.</h1>)";
    if (!failed_url.empty())
    {
        ss << R"(URL: <a href=")"
            << failed_url << "\">" << failed_url
            << "</a><br/>";
    }

    if (error_code != ERR_NONE)
    {
        ss << "Error: " << GetErrorString(error_code) << " ("
            << error_code << ")";
    }

    if (!other_info.empty())
        ss << "<br/>" << other_info;

    ss << "</body></html>";

    return ss.str();
}


// Load a data: URI containing the error message.
void LoadErrorPage(CefRefPtr<CefFrame> frame, const std::string& failed_url, cef_errorcode_t error_code, const std::string& other_info)
{
    if (failed_url.substr(0, strlen(EASYCEFSCHEME)) == EASYCEFSCHEME && error_code == ERR_UNKNOWN_URL_SCHEME)
    {
        std::ostringstream ss;
        ss << "data:text/html, Error " << error_code << "\n"
            << "Failed to load " << failed_url << "\n"
            << other_info;
        frame->LoadURL(ss.str());
    }
    else
    {

        frame->LoadURL(GetInternalPage(GetErrorPage(failed_url, other_info, error_code)));
    }


}

}
