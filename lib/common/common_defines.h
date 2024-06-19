#ifndef HH_COMMON_DEFINES_H
#define HH_COMMON_DEFINES_H

//#ifdef __cplusplus
//extern "C" {
//#endif
#include <functional>
typedef  std::function<void(uint32_t streamId)> ConnectHandler;
typedef  std::function<void(uint32_t streamId)> DisconnectHandler;
typedef  std::function<void(uint32_t streamId, unsigned char* data, int data_len)> VideoFrameHandler;
typedef  std::function<void(uint32_t streamId, unsigned char* data, int data_len)> AudioFrameHandler;

//typedef void (*ConnectHandler)(int streamId);
//typedef void (*DisconnectHandler)(int streamId);
//typedef void (*VideoFrameHandler)(int streamId, unsigned char* data, int data_len);
//typedef void (*AudioFrameHandler)(int streamId, unsigned char* data, int data_len);

extern ConnectHandler g_connectHandler;
extern DisconnectHandler g_disconnectHandler;
extern VideoFrameHandler g_videoFrameHandler;
extern AudioFrameHandler g_audioFrameHandler;

//#ifdef __cplusplus
//}
//#endif


#endif // HH_COMMON_DEFINES_H