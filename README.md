[中文版](https://github.com/dsafa22/AirplayServer/blob/master/README_CN.md)
# Introduction

`Airplay` screen mirror server based on `Android` and provide `WIN32` build. 99% written in C, easy to transplant, welcome to PR and ISSUE.

# Supported delivery devices

Support iPhone and Mac device screen delivery and music playback

Support iOS system: iOS9~iOS13

Support for MacOS system: 10.14 (low version not tested)

# Protocols and algorithms involved

If you understand the relevant protocols and algorithms, you will have a better understanding of the code.

1. Protocol: RTSP, RTCP, RTP, DNS, DNS-SD, mDNS, NTP

2. Encryption and decryption algorithm: AES (cbc & ctr)

3. Signature algorithm: curve25519, ed25519

3. Audio and video basics: H264 encoding, AAC encoding

# Third-party open source module

- MDNS:third_party/mDNSResponder

     Apple open source mDNS function for device discovery, added CMake support

- fdk-aac:third_party/fdk-aac

     For AAC audio decoding, added CMake support

# Screenshot

The following picture is a presentation of screen data and music, where the system of `iPhone` is `iOS12`

![](https://ww1.sinaimg.cn/large/007rAy9hgy1g0l65hwvg7j30u01o0juj.jpg)