// HHMDNSTest.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include "MDNSClient_old.h"
#include "HHMDNSDriver.h"
#include "airplay.h"

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <unistd.h>
#include <netdb.h> // for getaddrinfo
#include <arpa/inet.h> // for inet_ntop
#endif

#include <android/log.h>

#define LOG_TAG "WLNativeLog"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

DNSServiceRef RegServiceRef;
DNSServiceRef BrowseServiceRef;
std::shared_ptr<MDNSClient> mdnsClient = std::make_shared<MDNSClient>();

int HHRegisterService(const char* deviceName)
{
    // JSON 字符串
    std::string jsonString1 = R"({
        "name": "Device1",
        "width": "1920",
        "height": "1080",
        "deviceType": "SmartTV"
    })";

    std::string jsonString2 = R"({
        "name": "Device2",
        "width": "1280",
        "height": "720",
        "deviceType": "SmartSpeaker"
    })";

    // for test
    //mdnsClient->RegisterService("Service1", "_hhclient._tcp", 1111, jsonString1);
    //mdnsClient->RegisterService("Service2", "_hhclient._tcp", 2222, jsonString2);
    //mdnsClient->RegisterService("Service3", "_hhclient._tcp", 3333, jsonString1);
    //mdnsClient->RegisterService("Service4", "_hhclient._tcp", 4444, jsonString2);
    //mdnsClient->RegisterService("Service5", "_hhclient._tcp", 5555, jsonString1);
    //mdnsClient->RegisterService("Service6", "_hhclient._tcp", 6666, jsonString2);

    mdnsClient->StartBrowseService("_airplay._tcp", [](const std::string& jsonTxtRecord) {
        //std::cout << "Discovered JSON TXT Record: " << jsonTxtRecord << std::endl;
        LOGI("Discovered JSON TXT Record: %s", jsonTxtRecord.c_str());
    });

    return 0;
}

int HHUnRegisterService()
{
    // for test
    mdnsClient->UnregisterService("Service1", "_hhclient._tcp");
    mdnsClient->UnregisterService("Service2", "_hhclient._tcp");
    mdnsClient->UnregisterService("Service3", "_hhclient._tcp");
    mdnsClient->UnregisterService("Service4", "_hhclient._tcp");
    mdnsClient->UnregisterService("Service5", "_hhclient._tcp");
    mdnsClient->UnregisterService("Service6", "_hhclient._tcp");

    // for remove callback
    sleep(5);

    mdnsClient->StopBrowseService("_hhclient._tcp");

    return 0;
}

void HHStartBrowseService()
{
    mdnsClient->StartBrowseService("_airplay._tcp", [](const std::string& jsonTxtRecord) {
        //LOGI("Discovered JSON TXT Record: %s", jsonTxtRecord.c_str());
    });
}

void HHStopBrowseService()
{
    mdnsClient->StopBrowseService("_airplay._tcp");
}