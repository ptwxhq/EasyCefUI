#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "ApiFilter.h"
#include <processthreadsapi.h>
#include <detours/detours.h>
#include <ws2tcpip.h>
#include <unordered_map>
#include <string>
#include <format>

#pragma comment(lib, "ws2_32")

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


static BOOL WINAPI _MyCreateProcessA(
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




INT(WINAPI* real_getaddrinfo)(
    _In_opt_        PCSTR               pNodeName,
    _In_opt_        PCSTR               pServiceName,
    _In_opt_        const ADDRINFOA* pHints,
    _Outptr_        PADDRINFOA* ppResult
    ) = getaddrinfo;


std::unordered_map<std::string, std::string> g_mapLocalHost;  //domain,ip

void AddLocalHost(const char* domain, const char* ip)
{
    g_mapLocalHost.insert(std::make_pair(domain, ip));
}

void RemoveLocalHost(const char* domain)
{
    g_mapLocalHost.erase(domain);
}


static INT WINAPI _Mygetaddrinfo(
    _In_opt_        PCSTR               pNodeName,
    _In_opt_        PCSTR               pServiceName,
    _In_opt_        const ADDRINFOA* pHints,
    _Outptr_        PADDRINFOA* ppResult
)
{
#ifdef _DEBUG
   OutputDebugStringA(std::format("Node:{}, Service:{}\n", pNodeName ? pNodeName : "(null)", pServiceName ? pServiceName : "(null)").c_str());
#endif // _DEBUG
    std::string strIp;
    auto pWorkNodeName = pNodeName;
    if (pNodeName && !g_mapLocalHost.empty())
    {
       const auto it = g_mapLocalHost.find(pNodeName);
       if (it != g_mapLocalHost.end())
       {
           strIp = it->second;
           //pWorkNodeName = "127.0.0.1";
       }
    }

    auto oldres = real_getaddrinfo(pWorkNodeName, pServiceName, pHints, ppResult);

    if (oldres == 0 && !strIp.empty())
    {   
        auto addrInfo = *ppResult;
        while (addrInfo)
        {
            if (addrInfo->ai_family == AF_INET)
            {
                auto pSAddrIn = (sockaddr_in*)addrInfo->ai_addr;
#ifdef _DEBUG
                OutputDebugStringA(std::format("Edit:{}.{}.{}.{} To:{}\n", pSAddrIn->sin_addr.S_un.S_un_b.s_b1,
                    pSAddrIn->sin_addr.S_un.S_un_b.s_b2,
                    pSAddrIn->sin_addr.S_un.S_un_b.s_b3,
                    pSAddrIn->sin_addr.S_un.S_un_b.s_b4,
                    strIp.c_str()).c_str());
#endif // _DEBUG

                inet_pton(AF_INET, strIp.c_str(), &pSAddrIn->sin_addr);
            }

            addrInfo = addrInfo->ai_next;
        }

    }

    return oldres;
}


void BlockFlashNotSandboxedCmd()
{
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)realCreateProcessA, _MyCreateProcessA);
    DetourTransactionCommit();
}



void HostFilterOn()
{
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)real_getaddrinfo, _Mygetaddrinfo);
    DetourTransactionCommit();
}
