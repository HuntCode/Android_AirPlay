#ifndef HH_MDNS_DRIVER_H
#define HH_MDNS_DRIVER_H

#ifdef  __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C
#endif //  __cplusplus


#ifdef _WIN32
#ifdef LIBAIRPLAY_DLL
#ifdef LIBAIRPLAY_EXPORTS
#define HHMDNS_API EXTERN_C __declspec(dllexport)
#else
#define HHMDNS_API EXTERN_C __declspec(dllimport)
#endif
#else
#define HHMDNS_API EXTERN_C
#endif
#else
#define HHMDNS_API EXTERN_C
#endif


HHMDNS_API int HHRegisterService(const char* deviceName);

HHMDNS_API int HHUnRegisterService();

HHMDNS_API void HHStartBrowseService();

HHMDNS_API void HHStopBrowseService();

#endif // HH_AIRPLAY_H