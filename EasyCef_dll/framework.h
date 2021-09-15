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

#if CEF_VERSION_MAJOR == 94
#pragma message("!! you may need set _HAS_DEPRECATED_RESULT_OF and _SILENCE_CXX17_RESULT_OF_DEPRECATION_WARNING !!")
#endif



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

#if CEF_VERSION_MAJOR != 87 && CEF_VERSION_MAJOR != 90 && CEF_VERSION_MAJOR != 94
#pragma message("!!!!WARNNING:This project is base tested on Cef ver 87/90/94, maybe fail to build on other versions.!!!!")
#endif

#ifdef _DEBUG
#define ASSERT(x) {if(!(x)) _asm{int 0x03}}while(0)
#define VERIFY(x) {if(!(x)) _asm{int 0x03}}while(0)
#else
#define ASSERT(x)
#define VERIFY(x) x
#endif

#if CEF_VERSION_MAJOR > 92
#define CEF_FUNC_BIND base::BindOnce
#else
#define CEF_FUNC_BIND base::Bind
#endif
