// HHMDNSTest.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include "MDNSClient.h"
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


    //mdnsClient->RegisterService("MyDeviceService", "_wsraop._tcp", 7890, jsonString1);
    //mdnsClient->RegisterService("MyDeviceService2", "_smart._tcp", 8899, jsonString2);

    //mdnsClient->RegisterService("MyDeviceService_Android7767", "_wsraop._tcp", 7767, jsonString1);

    mdnsClient->StartBrowseService("_hhclient._tcp", [](const std::string& jsonTxtRecord) {
        std::cout << "Discovered JSON TXT Record: " << jsonTxtRecord << std::endl;
        LOGI("Discovered JSON TXT Record: %s", jsonTxtRecord.c_str());
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(5000));

    //mdnsClient->StopBrowseService("_hhclient._tcp");

    //mdnsClient->StartBrowseService("_smart._tcp", [](const std::string& jsonTxtRecord) {
    //    std::cout << "Discovered JSON TXT Record: " << jsonTxtRecord << std::endl;
    //});
    return 0;
}

#if 0
int main()
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


	std::shared_ptr<MDNSClient> mdnsClient = std::make_shared<MDNSClient>();
	mdnsClient->RegisterService("MyDeviceService1", "_wsraop._tcp", 7788, jsonString1);
	mdnsClient->RegisterService("MyDeviceService2", "_smart._tcp", 8899, jsonString2);


	mdnsClient->StartBrowseService("_wsraop._tcp", [](const std::string& jsonTxtRecord) {
		std::cout << "Discovered JSON TXT Record: " << jsonTxtRecord << std::endl;
		});
	mdnsClient->StartBrowseService("_smart._tcp", [](const std::string& jsonTxtRecord) {
		std::cout << "Discovered JSON TXT Record: " << jsonTxtRecord << std::endl;
		});

	// 阻塞主线程，直到输入 'q' 字符
	char input;
	std::cout << "Enter 'q' to quit the application." << std::endl;
	std::cout << "Enter 'u' to unregister service." << std::endl;
	while (std::cin >> input) {
		if (input == 'q') {
			break;
		}
		else if (input == 'u') {
			mdnsClient->UnregisterService("MyDeviceService1", "_wsraop._tcp");
		}
		else if (input == 's') {
			mdnsClient->StopBrowseService("_wsraop._tcp");
		}
		std::cout << "Invalid input. Enter 'q' to quit the application." << std::endl;
	}

	


	return 0;
}
#endif