#ifndef HH_AIRPLAY_H
#define HH_AIRPLAY_H

#ifdef  __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C
#endif //  __cplusplus


#ifdef _WIN32
#ifdef LIBAIRPLAY_DLL
#ifdef LIBAIRPLAY_EXPORTS
#define HHAIRPLAY_API EXTERN_C __declspec(dllexport)
#else
#define HHAIRPLAY_API EXTERN_C __declspec(dllimport)
#endif
#else
#define HHAIRPLAY_API EXTERN_C
#endif
#else
#define HHAIRPLAY_API EXTERN_C
#endif

/*#if define _WIN32 && define LIBAIRPLAY_DLL
#ifdef LIBAIRPLAY_EXPORTS
	#define HHAIRPLAY_API EXTERN_C __declspec(dllexport)
#else
	#define HHAIRPLAY_API EXTERN_C __declspec(dllimport)
#endif
#else
	#define HHAIRPLAY_API EXTERN_C
#endif*/

#include "common/common_defines.h"

/** SetCallbacks
 */
HHAIRPLAY_API void HHAirPlaySetConnectedHandler(ConnectHandler handler);
HHAIRPLAY_API void HHAirPlaySetDisconnectedHandler(DisconnectHandler handler);
HHAIRPLAY_API void HHAirPlaySetVideoFrameHandler(VideoFrameHandler handler);
HHAIRPLAY_API void HHAirPlaySetAudioFrameHandler(AudioFrameHandler handler);

/** Start AirPlay Service
 *
 * @param  deviceName The device name(service name) shown on the list of Apple Screen Mirroring
 *
 * @return
 * - 0: Success.
 * - < 0: Failure.
 */
HHAIRPLAY_API int HHAirPlayStart(const char* deviceName);

/** Stop AirPlay Service
 *
 */
HHAIRPLAY_API void HHAirPlayStop();

#endif // HH_AIRPLAY_H