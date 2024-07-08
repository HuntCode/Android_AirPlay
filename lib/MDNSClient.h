/*
*   author: wanglv
*   create_time: 2024/06/26
*   description:
*        a mdns client
*/

#ifndef MDNS_CLIENT_H
#define MDNS_CLIENT_H


#include <iostream>
#include <dns_sd.h>
#include <string>
#include <map>
#include <vector>
#include <thread>
#include <functional>

using DeviceInfoCallback = std::function<void(const std::string&)>;

class MDNSClient
{
public:
    MDNSClient();
    ~MDNSClient();

    int RegisterService(const std::string& name, const std::string& regtype, uint16_t port, const std::string& jsonString);
    void UnregisterService(const std::string& name, const std::string& regtype);
    void StartBrowseService(const std::string& regtype, DeviceInfoCallback callback);
    void StopBrowseService(const std::string& regtype);

    void OnDeviceInfoCallback(const std::string& regtype, const std::string& deviceInfo);

private:
    std::map<std::string, DNSServiceRef> m_registerRefs;
    std::map<std::string, DNSServiceRef> m_browseRefs;
    std::map<std::string, std::thread> m_browseThreads;
    std::map<std::string, std::atomic<bool>> m_browseRunning;
    std::map<std::string, std::vector<DeviceInfoCallback>> m_deviceInfoCallbacks;

};

#endif // MDNS_CLIENT_H


