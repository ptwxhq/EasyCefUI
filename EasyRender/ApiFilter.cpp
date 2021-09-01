#include <windows.h>
#include "ApiFilter.h"
#include <processthreadsapi.h>
#include <detours/detours.h>
//#include "include/cef_version.h"

//BOOL(WINAPI* realCreateProcessW)(
//    _In_opt_ LPCWSTR lpApplicationName,
//    _Inout_opt_ LPWSTR lpCommandLine,
//    _In_opt_ LPSECURITY_ATTRIBUTES lpProcessAttributes,
//    _In_opt_ LPSECURITY_ATTRIBUTES lpThreadAttributes,
//    _In_ BOOL bInheritHandles,
//    _In_ DWORD dwCreationFlags,
//    _In_opt_ LPVOID lpEnvironment,
//    _In_opt_ LPCWSTR lpCurrentDirectory,
//    _In_ LPSTARTUPINFOW lpStartupInfo,
//    _Out_ LPPROCESS_INFORMATION lpProcessInformation
//    ) = CreateProcessW;


BOOL(WINAPI* realCreateProcessA)(
    _In_opt_ LPCSTR lpApplicationName,
    _Inout_opt_ LPSTR lpCommandLine,
    _In_opt_ LPSECURITY_ATTRIBUTES lpProcessAttributes,
    _In_opt_ LPSECURITY_ATTRIBUTES lpThreadAttributes,
    _In_ BOOL bInheritHandles,
    _In_ DWORD dwCreationFlags,
    _In_opt_ LPVOID lpEnvironment,
    _In_opt_ LPCSTR lpCurrentDirectory,
    _In_ LPSTARTUPINFOA lpStartupInfo,
    _Out_ LPPROCESS_INFORMATION lpProcessInformation
) = CreateProcessA;


//BOOL WINAPI _MyCreateProcessW(
//    _In_opt_ LPCWSTR lpApplicationName,
//    _Inout_opt_ LPWSTR lpCommandLine,
//    _In_opt_ LPSECURITY_ATTRIBUTES lpProcessAttributes,
//    _In_opt_ LPSECURITY_ATTRIBUTES lpThreadAttributes,
//    _In_ BOOL bInheritHandles,
//    _In_ DWORD dwCreationFlags,
//    _In_opt_ LPVOID lpEnvironment,
//    _In_opt_ LPCWSTR lpCurrentDirectory,
//    _In_ LPSTARTUPINFOW lpStartupInfo,
//    _Out_ LPPROCESS_INFORMATION lpProcessInformation
//)
//{
//    if (lpCommandLine && wcsstr(lpCommandLine, L"echo NOT SANDBOXED"))
//    {
//        return FALSE;
//    }
//
//    return realCreateProcessW(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes,
//        bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
//}

BOOL WINAPI _MyCreateProcessA(
    _In_opt_ LPCSTR lpApplicationName,
    _Inout_opt_ LPSTR lpCommandLine,
    _In_opt_ LPSECURITY_ATTRIBUTES lpProcessAttributes,
    _In_opt_ LPSECURITY_ATTRIBUTES lpThreadAttributes,
    _In_ BOOL bInheritHandles,
    _In_ DWORD dwCreationFlags,
    _In_opt_ LPVOID lpEnvironment,
    _In_opt_ LPCSTR lpCurrentDirectory,
    _In_ LPSTARTUPINFOA lpStartupInfo,
    _Out_ LPPROCESS_INFORMATION lpProcessInformation
)
{
    if (lpCommandLine && strstr(lpCommandLine, "echo NOT SANDBOXED"))
    {
        return FALSE;
    }

    return realCreateProcessA(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes,
        bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
}

void BlockFlashNotSandboxedCmd()
{
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
   // DetourAttach(&(PVOID&)realCreateProcessW, _MyCreateProcessW);
    DetourAttach(&(PVOID&)realCreateProcessA, _MyCreateProcessA);
    DetourTransactionCommit();
}
