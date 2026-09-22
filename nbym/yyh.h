#pragma once

#if !defined(EQLIB_API)
    #if defined(_WIN32)
        #if defined(EQLIB_BUILD_SHARED)
            #define EQLIB_API __declspec(dllexport)
        #else
            #define EQLIB_API
        #endif
    #else
        #if defined(EQLIB_BUILD_SHARED)
            #define EQLIB_API __attribute__((visibility("default")))
        #else
            #define EQLIB_API
        #endif
    #endif
#endif

#if !defined(EQLIB_INTERNAL)
    #if defined(_WIN32)
        #define EQLIB_INTERNAL
    #else
        #define EQLIB_INTERNAL __attribute__((visibility("hidden")))
    #endif
#endif
