#pragma once

#include <functional>
#include <unordered_map>


class NativeV8Handler :
    public CefV8Handler
{
    //bool true:同步
    using FunctionMap = std::unordered_map<std::string, std::pair<void(*)(const CefV8ValueList&, CefRefPtr<CefV8Value>&, CefString&), bool>> ;


public:
    bool Execute(const CefString& name,
        CefRefPtr<CefV8Value> object,
        const CefV8ValueList& arguments,
        CefRefPtr<CefV8Value>& retval,
        CefString& exception) override;


    NativeV8Handler();

    //BrowserType限1和2
    void RegisterFunctions(CefRefPtr<CefV8Value> obj, int BrowserType);

    void RegisterUserFunctions(CefRefPtr<CefV8Value> obj, CefRefPtr<CefDictionaryValue> list, bool bSync, int BrowserType);


    IMPLEMENT_REFCOUNTING(NativeV8Handler);

private:

    FunctionMap m_mapFuncs;

};


class GobalNativeV8Handler :
    public CefV8Handler
{
    using FunctionMap = std::unordered_map<std::string, void(*)(const CefV8ValueList&, CefRefPtr<CefV8Value>&, CefString&)> ;
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