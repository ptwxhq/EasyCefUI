// EasyCef.cpp 
//

#include "pch.h"
#include "EasyCef.h"

#define MIN_BUILDED_CEF 87
#define MAX_BUILDED_CEF 120

#if (CEF_VERSION_MAJOR < MIN_BUILDED_CEF) || (CEF_VERSION_MAJOR > MAX_BUILDED_CEF) || !(\
(CEF_VERSION_MAJOR == MIN_BUILDED_CEF) || \
(CEF_VERSION_MAJOR == 90) || \
(CEF_VERSION_MAJOR == 94) || \
(CEF_VERSION_MAJOR == 96) || \
(CEF_VERSION_MAJOR == 98) || \
(CEF_VERSION_MAJOR == 100) || \
(CEF_VERSION_MAJOR == 109) || \
(CEF_VERSION_MAJOR == 120) || \
(CEF_VERSION_MAJOR == MAX_BUILDED_CEF))

#define EASY_CEF_BUILD_WARN_MSG "!!!!WARNNING:This project is not tested on this Cef version (" MAKE_STRING(CEF_VERSION_MAJOR) "), maybe fail to build.!!!!"
#pragma message(EASY_CEF_BUILD_WARN_MSG)

#else

#if (CEF_VERSION_MAJOR != 87)
#pragma message("!!Project Is Not Full Tested On This Cef Version " MAKE_STRING(CEF_VERSION_MAJOR) "!!")
#endif

#endif


#if CEF_VERSION_MAJOR >= 94	&& CEF_VERSION_MAJOR < 99
#if !defined(__clang__) && _MSVC_LANG > 201703L
#if !(_HAS_DEPRECATED_RESULT_OF && _SILENCE_CXX17_RESULT_OF_DEPRECATION_WARNING)
#pragma message("!! you may need set _HAS_DEPRECATED_RESULT_OF and _SILENCE_CXX17_RESULT_OF_DEPRECATION_WARNING !!")
#endif
#endif
#endif
