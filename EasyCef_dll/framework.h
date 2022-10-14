#pragma once

#define WIN32_LEAN_AND_MEAN             // 从 Windows 头文件中排除极少使用的内容
#define NOMINMAX

//兼容旧项目，目前未全部设计一套，直接按旧的走，故暂不可删
#define EASY_LEGACY_API_COMPATIBLE 1

// Windows 头文件
#include <windows.h>

#include <atlbase.h>
#include <atlwin.h>

#include "include/cef_version.h"


#include "include/cef_base.h"
#include "include/cef_app.h"
#include "include/cef_browser.h"
#include "include/cef_client.h"
#include "include/cef_v8.h"
#include "include/cef_parser.h"

#include "include/wrapper/cef_helpers.h"

#include "EasyStructs.h"
#include "Utility.h"


typedef unsigned long wvhandle;

#ifdef _DEBUG
#define ASSERT(x) assert(x)
#define VERIFY(x) assert(x)
#else
#define ASSERT(x)
#define VERIFY(x) x
#endif

#if CEF_VERSION_MAJOR > 92
#define CEF_FUNC_BIND base::BindOnce
#else
#define CEF_FUNC_BIND base::Bind
#endif

#if CEF_VERSION_MAJOR > 95
#define CEF_REQUEST_CALLBACK CefCallback
#else
#define CEF_REQUEST_CALLBACK CefRequestCallback
#endif


#include <format>


#define MYDISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&) = delete;               \
  void operator=(const TypeName&) = delete


#define DPI_1X 96.0f