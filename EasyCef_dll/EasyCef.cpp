// EasyCef.cpp 
//

#include "pch.h"
#include "EasyCef.h"


#if (CEF_VERSION_MAJOR < 87) || (CEF_VERSION_MAJOR > 96) || !(\
(CEF_VERSION_MAJOR == 87) || \
(CEF_VERSION_MAJOR == 90) || \
(CEF_VERSION_MAJOR == 94) || \
(CEF_VERSION_MAJOR == 96) || \
0)
#pragma message("!!!!WARNNING:This project is not tested on this Cef version, maybe fail to build.!!!!")
#endif


#if CEF_VERSION_MAJOR >= 94
#if !(_HAS_DEPRECATED_RESULT_OF && _SILENCE_CXX17_RESULT_OF_DEPRECATION_WARNING)
#pragma message("!! you may need set _HAS_DEPRECATED_RESULT_OF and _SILENCE_CXX17_RESULT_OF_DEPRECATION_WARNING !!")
#endif
#endif
