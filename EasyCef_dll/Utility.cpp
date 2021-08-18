#include "pch.h"

//#include <tlhelp32.h>
#include <Psapi.h>
#include <Winternl.h>
#include "Utility.h"


#include <ShlObj_core.h>

#include "include/base/cef_logging.h"
#include <random>

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
        scale_factor = static_cast<float>(dpi_x) / 96.0f;
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

std::string QuickMakeIpcParms(int BrowserId, int64 FrameId, const std::string& name, const CefRefPtr<CefListValue>& valueList)
{
    CefRefPtr<CefValue> forsend = CefValue::Create();
    CefRefPtr<CefDictionaryValue> dict = CefDictionaryValue::Create();

    dict->SetInt("bid", BrowserId);
    dict->SetString("fid", std::to_string(FrameId));
    dict->SetString("name", name);
    dict->SetList("arg", valueList);

    forsend->SetDictionary(dict);

    return CefWriteJSON(forsend, JSON_WRITER_DEFAULT).ToString();
}

bool QuickGetIpcParms(const std::string& strData, int& BrowserId, int64& FrameId, std::string& name, CefRefPtr<CefListValue>& valueList)
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
        typedef LONG(WINAPI* pfnNtQueryInformationProcess)(HANDLE, UINT, PVOID, ULONG, PULONG);
        pfnNtQueryInformationProcess _NtQueryInformationProcess = nullptr;
        PROCESS_BASIC_INFORMATION pbi;
        auto ntdll = GetModuleHandleA("ntdll");
        if (ntdll)
            _NtQueryInformationProcess = (pfnNtQueryInformationProcess)GetProcAddress(ntdll, "NtQueryInformationProcess");
        if (_NtQueryInformationProcess)
        {
            HANDLE hProcessSelf = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, GetCurrentProcessId());

            long status = _NtQueryInformationProcess(hProcessSelf,
                ProcessBasicInformation,
                (PVOID)&pbi,
                sizeof(PROCESS_BASIC_INFORMATION),
                NULL
            );
            CloseHandle(hProcessSelf);
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
    const std::string CHARACTERS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

    std::random_device random_device;
    std::mt19937 generator(random_device());
    const std::uniform_int_distribution<> distribution(0, CHARACTERS.size() - 1);

    std::string random_string;

    for (std::size_t i = 0; i < length; ++i)
    {
        random_string += CHARACTERS[distribution(generator)];
    }

    return random_string;
}

std::string GetDataURI(const std::string& data, const std::string& mime_type) {
    return "data:" + mime_type + ";base64," +
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
            result = CefV8Value::CreateObject(NULL, NULL);
            CefRefPtr<CefDictionaryValue> dict = value->GetDictionary();
            CefDictionaryValue::KeyList keys;
            dict->GetKeys(keys);
            for (unsigned int i = 0; i < keys.size(); i++) {
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
            for (int i = 0; i < size; i++) {
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
        for (size_t i = 0; i < value->GetArrayLength(); i++)
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
