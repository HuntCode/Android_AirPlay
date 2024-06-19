# 简介

基于`Android`的`Airplay`屏幕镜像服务器，并提供`WIN32`的构建，99%使用C编写，方便移植，欢迎提PR和ISSUE

# 支持的投放设备

支持iPhone和Mac设备屏幕投放和音乐播放

支持iOS系统：iOS9~iOS13

支持MacOS系统：10.14（低版本未测试）

# 涉及到的协议和算法

如果了解了相关协议和算法会更好的理解代码

1. 协议：RTSP，RTCP、RTP、DNS，DNS-SD，mDNS，NTP

2. 加解密算法：AES（cbc&ctr）

3. 签名算法：curve25519，ed25519

3. 音视频基础：H264编码，AAC编码

# 第三方开源模块

- MDNS：third_party/mDNSResponder

    Apple开源mDNS功能，用于设备发现，添加了CMake支持

- fdk-aac音频解码：third_party/fdk-aac

    用于AAC音频解码，添加了CMake支持

# 演示截图

下图是一次屏幕数据和音乐的投放演示，其中`iPhone`的系统是`iOS12`

![](https://ww1.sinaimg.cn/large/007rAy9hgy1g0l65hwvg7j30u01o0juj.jpg)