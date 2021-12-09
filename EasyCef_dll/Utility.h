#pragma once

#include <string>

#include "include/internal/cef_types_wrappers.h"

uint64_t GetTimeNow();

// Set the window's user data pointer.
void SetUserDataPtr(HWND hWnd, void* ptr);

// Return the window's user data pointer.
template <typename T>
T GetUserDataPtr(HWND hWnd) {
	return reinterpret_cast<T>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
}

// Set the window's window procedure pointer and return the old value.
WNDPROC SetWndProcPtr(HWND hWnd, WNDPROC wndProc);

// Return the resource string with the specified id.
std::wstring GetResourceString(UINT id);

int GetCefMouseModifiers(WPARAM wparam);
int GetCefKeyboardModifiers(WPARAM wparam, LPARAM lparam);
bool IsKeyDown(WPARAM wparam);

// Returns the device scale factor. For example, 200% display scaling will
// return 2.0.
float GetDeviceScaleFactor();


// Convert |value| from logical coordinates to device coordinates.
int LogicalToDevice(int value, float device_scale_factor);
CefRect LogicalToDevice(const CefRect& value, float device_scale_factor);

// Convert |value| from device coordinates to logical coordinates.
int DeviceToLogical(int value, float device_scale_factor);
void DeviceToLogical(CefMouseEvent& value, float device_scale_factor);

std::string QuickMakeIpcParms(int BrowserId, int64 FrameId, const std::string& name, const CefRefPtr<CefListValue>& valueList);

bool QuickGetIpcParms(const std::string& strData, int& BrowserId, int64& FrameId, std::string& name, CefRefPtr<CefListValue>& valueList);

bool GetParentProcessInfo(DWORD* pdwId, std::wstring* pstrPathOut);

std::string GetRandomString(size_t length);

std::string GetDataURI(const std::string& data, const std::string& mime_type);

std::string GetInternalPage(const std::string& data);

//不要在render进程内调用
std::wstring GetDefAppDataFolder();

CefRefPtr<CefV8Value> CefValueToCefV8Value(CefRefPtr<CefValue> value);

CefRefPtr<CefValue> CefV8ValueToCefValue(CefRefPtr<CefV8Value> value);

CefString CefV8ValueToString(CefRefPtr<CefV8Value> value);

void SetRequestDefaultSettings(CefRefPtr<CefRequestContext> request_context);

void SetAllowDarkMode(int nValue);

namespace webinfo {

	std::string GetCertificateInformation(const std::string& url, CefRefPtr<CefX509Certificate> cert, cef_cert_status_t certstatus);
	std::string GetErrorPage(const std::string& failed_url, const std::string& other_info, cef_errorcode_t error_code = (cef_errorcode_t)10000);
	void LoadErrorPage(CefRefPtr<CefFrame> frame, const std::string& failed_url, cef_errorcode_t error_code, const std::string& other_info);
}

