// EasyCef.cpp : 定义应用程序的入口点。
//

#include "pch.h"
#include "EasyCef.h"

//目前基于87开发，后续看兼容90以上版本便于升级

//
//#include "Export.h"
//
//
//void GetLocalPaths();
//void     RegEasyCefSchemes();
//
//int testwinMain(HINSTANCE hInstance,
//                     HINSTANCE hPrevInstance,
//                     LPWSTR    lpCmdLine,
//                     int       nCmdShow)
//{
//    GetLocalPaths();
//    // Enable High-DPI support on Windows 7 or newer.
//    CefEnableHighDPISupport();
//
//    void* sandbox_info = nullptr;
//
//#if defined(CEF_USE_SANDBOX)
//    // Manage the life span of the sandbox information object. This is necessary
//    // for sandbox support on Windows. See cef_sandbox_win.h for complete details.
//    CefScopedSandboxInfo scoped_sandbox;
//    sandbox_info = scoped_sandbox.sandbox_info();
//#endif
//
//    // Provide CEF with command-line arguments.
//    CefMainArgs main_args(hInstance);
//
//    CefRefPtr<CefCommandLine> command_line = CefCommandLine::CreateCommandLine();
//    command_line->InitFromString(::GetCommandLineW());
//
//    // Create a ClientApp of the correct type.
//    CefRefPtr<CefApp> app;
//    if (!command_line->HasSwitch("type"))
//    {
//        app = new EasyCefAppBrowser();
// //       _beginthread(,)
//
//    }
//    else
//    {
//        const std::string& processType = command_line->GetSwitchValue("type");
//        if (processType == "renderer")
//        {
//            app = new EasyCefAppRender();
//        }
//    }
//
//    // CEF applications have multiple sub-processes (render, plugin, GPU, etc)
//    // that share the same executable. This function checks the command-line and,
//    // if this is a sub-process, executes the appropriate logic.
//    int exit_code = CefExecuteProcess(main_args, app, sandbox_info);
//    if (exit_code >= 0) {
//        // The sub-process has completed so return here.
//        return exit_code;
//    }
//
//    // Specify CEF global settings here.
//    CefSettings settings;
//
//#if !defined(CEF_USE_SANDBOX)
//    settings.no_sandbox = true;
//#endif
//
////    settings.windowless_rendering_enabled = 1;
////    settings.background_color = 0;
//
//    //auto p1 = std::hash<std::wstring>{}(g_BrowserGlobalVar.FileDir+L"123");
//    //auto p2 = std::hash<std::wstring>{}(g_BrowserGlobalVar.FileDir + L"124");
//
//
//
//#if CEF_VERSION_MAJOR >= 90
//    cef_string_set(L"xpack", wcslen(L"xpack"), &settings.cookieable_schemes_list, true);
//#endif
//   // cef_string_set(L"zh-CN", wcslen(L"zh-CN"), &settings.locale, true);
//    CefString(&settings.locale).FromASCII("zh-CN");
//
//    std::wstring strpath = g_BrowserGlobalVar.FileDir + LR"(EasyRender.exe)";
//    CefString(&settings.browser_subprocess_path).FromWString(strpath);
//
//    // Initialize CEF.
//    CefInitialize(main_args, settings, app.get(), sandbox_info);
//
//    RegEasyCefSchemes();
//
//    EASYCEF::SetCloseHandler(CefQuitMessageLoop);
//
//
//    // Run the CEF message loop. This will block until CefQuitMessageLoop() is
//    // called.
//    CefRunMessageLoop();
//
//    // Shut down CEF.
//    CefShutdown();
//
//    return 0;
//}
