// EasyCef.cpp 
//

#include "pch.h"
#include "EasyCef.h"


#if (CEF_VERSION_MAJOR < 87) || (CEF_VERSION_MAJOR > 98) || !(\
(CEF_VERSION_MAJOR == 87) || \
(CEF_VERSION_MAJOR == 90) || \
(CEF_VERSION_MAJOR == 94) || \
(CEF_VERSION_MAJOR == 96) || \
(CEF_VERSION_MAJOR == 98) || \
0)

#define EASY_CEF_BUILD_WARN_MSG "!!!!WARNNING:This project is not tested on this Cef version (" MAKE_STRING(CEF_VERSION_MAJOR) "), maybe fail to build.!!!!"
#pragma message(EASY_CEF_BUILD_WARN_MSG)

#else

#if (CEF_VERSION_MAJOR != 87)
#pragma message("!!Project Is Not Full Tested On This Cef Version " MAKE_STRING(CEF_VERSION_MAJOR) "!!")
#endif

#endif


#if CEF_VERSION_MAJOR >= 94
#if !(_HAS_DEPRECATED_RESULT_OF && _SILENCE_CXX17_RESULT_OF_DEPRECATION_WARNING)
#pragma message("!! you may need set _HAS_DEPRECATED_RESULT_OF and _SILENCE_CXX17_RESULT_OF_DEPRECATION_WARNING !!")
#endif
#endif
