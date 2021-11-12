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
#ifndef _WIN64
#define ASSERT(x) {if(!(x)) _asm{int 0x03}}while(0)
#define VERIFY(x) ASSERT(x)
#else
#define ASSERT(x) assert(x)
#define VERIFY(x) ASSERT(x)
#endif

#else
#define ASSERT(x)
#define VERIFY(x) x
#endif

#if CEF_VERSION_MAJOR > 92
#define CEF_FUNC_BIND base::BindOnce
#else
#define CEF_FUNC_BIND base::Bind
#endif
