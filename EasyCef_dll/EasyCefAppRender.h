#pragma once


//render进程

class EasyCefAppRender :
    public CefApp,
    public CefRenderProcessHandler,
    public CefLoadHandler
{
public:


    EasyCefAppRender() {};

    CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() override { return this; }


    void OnWebKitInitialized() override;


    void OnBrowserCreated(CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefDictionaryValue> extra_info) override;

    void OnBrowserDestroyed(CefRefPtr<CefBrowser> browser)override;

    void OnContextCreated(CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        CefRefPtr<CefV8Context> context) override;

    void OnContextReleased(CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        CefRefPtr<CefV8Context> context) override;


    void OnUncaughtException(CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        CefRefPtr<CefV8Context> context,
        CefRefPtr<CefV8Exception> exception,
        CefRefPtr<CefV8StackTrace> stackTrace) override {}


    void OnFocusedNodeChanged(CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        CefRefPtr<CefDOMNode> node) override;


    bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        CefProcessId source_process,
        CefRefPtr<CefProcessMessage> message) override;


    void OnRegisterCustomSchemes(CefRawPtr<CefSchemeRegistrar> registrar) override;


  ////////////////////////////////////////////////


    IMPLEMENT_REFCOUNTING(EasyCefAppRender);


private:
    MYDISALLOW_COPY_AND_ASSIGN(EasyCefAppRender);


    CefRefPtr<CefDictionaryValue> m_dictUserSyncFunc, m_dictUserAsyncFunc;

};



class EasyCefAppOther : public CefApp
{
public:
    EasyCefAppOther() {};
    void OnBeforeCommandLineProcessing(
        const CefString& process_type,
        CefRefPtr<CefCommandLine> command_line) override;

    IMPLEMENT_REFCOUNTING(EasyCefAppOther);


private:
    MYDISALLOW_COPY_AND_ASSIGN(EasyCefAppOther);
};
