#ifndef WAPI_H
#define WAPI_H

#if defined(_MSC_VER)
    //  Microsoft
    #define WEXPORT __declspec(dllexport)
    #define WIMPORT __declspec(dllimport)
#elif defined(__GNUC__)
    //  GCC
    #define WEXPORT __attribute__((visibility("default")))
    #define WIMPORT
#else
    //  do nothing and hope for the best?
    #define WEXPORT
    #define WIMPORT
    #pragma warning Unknown dynamic link import/export semantics.
#endif


#endif // WAPI_H
