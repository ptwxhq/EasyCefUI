// EasyRender.cpp : 定义应用程序的入口点。
//

#include "framework.h"
#include "EasyRender.h"

#include <cmath>

#include "SpeedBox.h"

#include "../EasyCef_dll/EasyExport.h"

//本进程需要sysver.manifest
#ifdef _WIN64

#ifdef _DEBUG
#pragma comment(lib, "x64/Debug/easycef_d.lib")
#else
#pragma comment(lib, "x64/Release/easycef.lib")
#endif // DEBUG

#else

#ifdef _DEBUG
#pragma comment(lib, "Debug/easycef_d.lib")
#else
#pragma comment(lib, "Release/easycef.lib")
#endif // DEBUG

#endif // _WIN64



#include "ApiFilter.h"
#include <format>


void SpeedWork(float val)
{
    static bool bSet = false;
    if (fabs(val - 1.0) < 0.01)
    {
        if (bSet)
        {
            //设置启动之后就不要去关了，原先是这样的，应该原先有什么问题吧，就按这个来吧
            EnableSpeedControl(TRUE);
            SetSpeed(1.f);
        }
        else
        {
            EnableSpeedControl(FALSE);
        }
       
    }
    else
    {
        bSet = true;
        EnableSpeedControl(TRUE);
        SetSpeed(val);
    }
}


void SetHostResolverInfo(LPCSTR lpszHost, LPCSTR lpszIp)
{
    if (lpszHost && lpszHost[0])
    {
        if (lpszIp && lpszIp[0])
        {
            AddLocalHost(lpszHost, lpszIp);
        
        }
        else
        {
            RemoveLocalHost(lpszHost);
        }
    }
}




int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    //>CEF118
#if defined(ARCH_CPU_32_BITS)
    // Run the main thread on 32-bit Windows using a fiber with the preferred 4MiB
    // stack size. This function must be called at the top of the executable entry
    // point function (`main()` or `wWinMain()`). It is used in combination with
    // the initial stack size of 0.5MiB configured via the `/STACK:0x80000` linker
    // flag on executable targets. This saves significant memory on threads (like
    // those in the Windows thread pool, and others) whose stack size can only be
    // controlled via the linker flag.
    int exit_code = CefRunWinMainWithPreferredStackSize(wWinMain, hInstance,
        lpCmdLine, nCmdShow);
    if (exit_code >= 0) {
        // The fiber has completed so return here.
        return exit_code;
    }
#endif

    if (__argc < 2)
    {
        return 0;
    }

    using namespace EASYCEF;

    SetSpeedUpWork(SpeedWork);

    auto pt = GetProcessType();

    switch (pt)
    {
    case 2:
        break;
    case 12:
        HostFilterOn();
        SetHostResolverWork(SetHostResolverInfo);
        break;
    case 4:
        BlockFlashNotSandboxedCmd();
        break;
    case 3:
    case 5:
    case 10:
    case 11:
    case -1:
        break;
    default:
        return 0;
    }


    EASYCEF::EasyInitConfig config;

    return InitEasyCef(hInstance, nullptr, &config);
}
