#pragma once

#include <functional>
#include <unordered_map>


class NativeV8Handler :
    public CefV8Handler
{

    //typedef std::function<void(const CefV8ValueList&, CefRefPtr<CefV8Value>&, CefString&)> HandleCallback;
    typedef void(*HandleCallback)(const CefV8ValueList&, CefRefPtr<CefV8Value>&, CefString&);

    //bool true:同步
    typedef std::unordered_map<std::string, std::pair<HandleCallback, bool>> FunctionMap;


public:
    bool Execute(const CefString& name,
        CefRefPtr<CefV8Value> object,
        const CefV8ValueList& arguments,
        CefRefPtr<CefV8Value>& retval,
        CefString& exception) override;


    NativeV8Handler();

    //BrowserType限1和2
    void RegisterFunctions(CefRefPtr<CefV8Value> obj, int BrowserType);


    IMPLEMENT_REFCOUNTING(NativeV8Handler);

private:

    FunctionMap m_mapFuncs;

};


class GobalNativeV8Handler :
    public CefV8Handler
{
    typedef void(*HandleCallback)(const CefV8ValueList&, CefRefPtr<CefV8Value>&, CefString&);
    typedef std::unordered_map<std::string, HandleCallback> FunctionMap;
public:
    bool Execute(const CefString& name,
        CefRefPtr<CefV8Value> object,
        const CefV8ValueList& arguments,
        CefRefPtr<CefV8Value>& retval,
        CefString& exception) override;

    GobalNativeV8Handler();

    IMPLEMENT_REFCOUNTING(GobalNativeV8Handler);

private:

    FunctionMap m_mapFuncs;
};