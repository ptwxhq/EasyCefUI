#pragma once

//you may need to build release once if there is no version_git.h file
#include "version_git.h"

#define MAJOR_VER_NUM 1
#define MINOR_VER_NUM GIT_VER_YEAR
#define PATCH_VER_NUM GIT_VER_MONANDDAY
#define BUILD_VER_NUM GIT_VER_NUM

#define STRING_NUMVER_(x) #x
#define STRING_NUMVER(x) STRING_NUMVER_(x)

#define RC_VERSION_STRING STRING_NUMVER(MAJOR_VER_NUM) "." STRING_NUMVER(MINOR_VER_NUM) "." STRING_NUMVER(PATCH_VER_NUM) "." STRING_NUMVER(BUILD_VER_NUM)
#define RC_VERSION_INFO MAJOR_VER_NUM,MINOR_VER_NUM,PATCH_VER_NUM,BUILD_VER_NUM
