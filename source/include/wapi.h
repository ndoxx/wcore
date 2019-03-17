#ifndef WAPI_H
#define WAPI_H

#if defined(_MSC_VER)
    //  Microsoft
    #if defined(WCORE_BUILD_LIB)
        #define WAPI __declspec(dllexport)
    #else
        #define WAPI __declspec(dllimport)
    #endif
#elif defined(__GNUC__)
    //  GCC
    #if defined(WCORE_BUILD_LIB)
        #define WAPI __attribute__((visibility("default")))
    #else
        #define WAPI
    #endif
#else
    //  do nothing and hope for the best?
    #define WAPI
    #pragma warning Unknown dynamic link import/export semantics.
#endif


#endif // WAPI_H
