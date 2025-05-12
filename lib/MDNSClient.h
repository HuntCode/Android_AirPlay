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
#include <mutex>

class MDNSClient:public std::enable_shared_from_this<MDNSClient>
{
public:
    MDNSClient();
    ~MDNSClient();

    using DeviceInfoCallback = std::function<void(const std::string&)>;

    int RegisterService(const std::string& name, const std::string& regtype, uint16_t port, const std::string& jsonString);
    void UnregisterService(const std::string& name, const std::string& regtype);
    void StartBrowseService(const std::string& regtype, DeviceInfoCallback callback);
    void StopBrowseService(const std::string& regtype);

    void OnDeviceInfoCallback(const std::string& regtype, const std::string& deviceInfo);
    void Release();

    // 添加新的 resolveRef（已创建成功）
    void AddResolveRef(const std::string& regtype, const std::string& instanceName, DNSServiceRef resolveRef);

    // 移除并释放对应 resolveRef（主动或被动移除）
    void RemoveResolveRef(const std::string& regtype, const std::string& instanceName);

private:
    struct DeviceInfo {
        std::string jsonTxtRecord;
        uint16_t port;
    };

    struct ServiceContext {
        MDNSClient* client;
        std::string regtype;
        std::string fullname;
        std::string action;
    };

    struct BrowseContext {
        DNSServiceRef serviceRef = nullptr;
        std::thread browseThread;
        bool running = false;
        std::vector<DeviceInfoCallback> callbacks;

        std::map<std::string, DNSServiceRef> resolveRefs;

        bool cleanBrowseRef = false;
    };

    std::map<std::string, DeviceInfo> m_deviceInfos;
    std::map<std::string, DNSServiceRef> m_registerRefs;
    std::map<std::string, BrowseContext> m_browseContexts;

    std::mutex m_contextMutex;

    static void DNSSD_API BrowseCallback(
        DNSServiceRef serviceRef,
        DNSServiceFlags flags,
        uint32_t interfaceIndex,
        DNSServiceErrorType errorCode,
        const char* serviceName,
        const char* regtype,
        const char* replyDomain,
        void* context);

    static void DNSSD_API ResolveCallback(
        DNSServiceRef serviceRef,
        DNSServiceFlags flags,
        uint32_t interfaceIndex,
        DNSServiceErrorType errorCode,
        const char* fullname,
        const char* hosttarget,
        uint16_t port,
        uint16_t txtLen,
        const unsigned char* txtRecord,
        void* context);

    static void DNSSD_API AddrInfoCallback(
        DNSServiceRef serviceRef,
        DNSServiceFlags flags, 
        uint32_t interfaceIndex, 
        DNSServiceErrorType errorCode, 
        const char* hostname, 
        const struct sockaddr* address, 
        uint32_t ttl, 
        void* context);

    std::string parseTXTRecordToJson(const unsigned char* txtRecord, uint16_t txtLen);

    std::string ExtractInstanceName(const std::string& fullname);

    // 避免本地中文乱码，仅用于显示可读
    // 尽量使用UTF8
    std::string Utf8ToAnsi(const std::string& utf8Str);

    void CleanupBrowseContext(const std::string& regtype);

};

#endif // MDNS_CLIENT_H


