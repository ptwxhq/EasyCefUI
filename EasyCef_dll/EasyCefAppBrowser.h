#pragma once

#include "EasyClientHandler.h"


class EasyCefAppBrowser :
    public CefApp, public CefBrowserProcessHandler
{
public:

    CefRefPtr< CefBrowserProcessHandler > GetBrowserProcessHandler() override { return this; }

    void OnContextInitialized() override;

    void OnBeforeCommandLineProcessing(
        const CefString& process_type,
        CefRefPtr<CefCommandLine> command_line) override;


    //////////////////////////////////////

    void OnBeforeChildProcessLaunch(
        CefRefPtr<CefCommandLine> command_line) override;


    void OnRegisterCustomSchemes(CefRawPtr<CefSchemeRegistrar> registrar) override;


private:


    IMPLEMENT_REFCOUNTING(EasyCefAppBrowser);

};

