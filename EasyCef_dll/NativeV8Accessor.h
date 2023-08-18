#pragma once
#include <unordered_map>


class NativeV8Accessor :
    public CefV8Accessor
{
    using FunctionGetMap = std::unordered_map<std::string, bool(*)(CefRefPtr<CefV8Value>&)> ;
    using FunctionSetMap = std::unordered_map<std::string, bool(*)(const CefRefPtr<CefV8Value>)>;

public:
    virtual bool Get(const CefString& name,
        const CefRefPtr<CefV8Value> object,
        CefRefPtr<CefV8Value>& retval,
        CefString& exception) override;

    virtual bool Set(const CefString& name,
        const CefRefPtr<CefV8Value> object,
        const CefRefPtr<CefV8Value> value,
        CefString& exception) override;

    //仅需要在ui注册
    void RegisterKeys(CefRefPtr<CefV8Value> obj);

    IMPLEMENT_REFCOUNTING(NativeV8Accessor);
private:
    FunctionGetMap m_mapGetFuncs;
    FunctionSetMap m_mapSetFuncs;
};

