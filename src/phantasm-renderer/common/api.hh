#pragma once

#include <clean-core/macros.hh>

#ifdef CC_OS_WINDOWS

#ifdef PR_BUILD_DLL

#ifdef PR_DLL
#define PR_API __declspec(dllexport)
#else
#define PR_API __declspec(dllimport)
#endif

#else
#define PR_API
#endif

#else
#define PR_API
#endif
