#include "MDNSClient.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

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

struct ResolveInfo{
    MDNSClient* client;
    const unsigned char* txtRecord;
};

// 解析TXTRecord
std::string parseTXTRecordToJson(const unsigned char* txtRecord, uint16_t txtLen) {
    rapidjson::Document doc;
    doc.SetObject();
    rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

    uint16_t index = 0;
    while (index < txtLen) {
        uint8_t keyLen = txtRecord[index];
        std::string keyValue((const char*)&txtRecord[index + 1], keyLen);

        size_t pos = keyValue.find('=');
        if (pos != std::string::npos) {
            std::string key = keyValue.substr(0, pos);
            std::string value = keyValue.substr(pos + 1);
            doc.AddMember(rapidjson::Value(key.c_str(), allocator), rapidjson::Value(value.c_str(), allocator), allocator);
        }

        index += keyLen + 1;
    }

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    return buffer.GetString();
}

void DNSSD_API getaddrinfoCallback(
        DNSServiceRef sdRef,
        DNSServiceFlags flags,
        uint32_t interfaceIndex,
        DNSServiceErrorType errorCode,
        const char* hostname,
        const struct sockaddr* address,
        uint32_t ttl,
        void* context)
{
    if (errorCode == kDNSServiceErr_NoError) {
        if (address->sa_family == AF_INET) {
            struct sockaddr_in* addr_in = (struct sockaddr_in*)address;
            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(addr_in->sin_addr), ip, INET_ADDRSTRLEN);
            std::string remoteIP = ip;
            LOGI("remoteIP: %s", remoteIP.c_str());
        }
    }
    else {
        printf("Error: %d\n", errorCode);
    }

    int a = 1024;
}

void DNSSD_API ResolveCallback(
        DNSServiceRef sdRef,
        DNSServiceFlags flags,
        uint32_t interfaceIndex,
        DNSServiceErrorType errorCode,
        const char* fullname,
        const char* hosttarget,
        uint16_t port,
        uint16_t txtLen,
        const unsigned char* txtRecord,
        void* context)
{
    MDNSClient* client = (MDNSClient*)context;

    DNSServiceErrorType error = DNSServiceGetAddrInfo(&sdRef, 0, 0, kDNSServiceProtocol_IPv4 | kDNSServiceProtocol_IPv6, hosttarget, getaddrinfoCallback, context);

    if (error == kDNSServiceErr_NoError) {
        DNSServiceProcessResult(sdRef);
    }
    else {
        //printf("error: %d\n", error);
    }

    //if (errorCode == kDNSServiceErr_NoError) {
    //    std::string jsonTxtRecord = parseTXTRecordToJson(txtRecord, txtLen);
//
    //    std::string fullServiceName = fullname;
    //    size_t start = fullServiceName.find(".") + 1;
    //    size_t end = fullServiceName.find(".", start);
    //    end = fullServiceName.find(".", end + 1);
//
    //    std::string serviceType = fullServiceName.substr(start, end - start);
//
    //    // 解析 IP 地址
    //    struct addrinfo hints, * res;
    //    memset(&hints, 0, sizeof(hints));
    //    hints.ai_family = AF_INET; // 仅使用 IPv4
//
    //    int result = getaddrinfo(hosttarget, nullptr, &hints, &res);
    //    if (result != 0) {
    //        LOGI("Error getting address info: %d", result);
    //    }
    //    else {
    //        char ipStr[INET_ADDRSTRLEN];
    //        inet_ntop(AF_INET, &(((struct sockaddr_in*)res->ai_addr)->sin_addr), ipStr, INET_ADDRSTRLEN);
    //        freeaddrinfo(res);
//
    //        std::cout << "Resolved IP address: " << ipStr << std::endl;
//
    //        // 解析后的 JSON 字符串
    //        rapidjson::Document document;
    //        document.Parse(jsonTxtRecord.c_str());
    //        rapidjson::Document::AllocatorType& allocator = document.GetAllocator();
//
    //        // 添加 IP 地址
    //        rapidjson::Value ipKey("ip", allocator);
    //        rapidjson::Value ipValue(ipStr, allocator);
    //        document.AddMember(ipKey, ipValue, allocator);
//
    //        // 添加 Port
    //        rapidjson::Value portKey("port", allocator);
    //        rapidjson::Value portValue(std::to_string(ntohs(port)).c_str(), allocator);
    //        document.AddMember(portKey, portValue, allocator);
//
    //        // 将 JSON 对象转换为字符串
    //        rapidjson::StringBuffer buffer;
    //        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    //        document.Accept(writer);
    //        jsonTxtRecord = buffer.GetString();
    //    }
//
    //    //std::cout << "Resolved service: " << fullname << std::endl;
    //    //std::cout << "Host: " << hosttarget << std::endl;
    //    //std::cout << "Port: " << ntohs(port) << std::endl;
    //    //std::cout << "TXT Record (JSON): " << jsonTxtRecord << std::endl;
//
    //    LOGI("jsonTxtRecord : %s", jsonTxtRecord.c_str());
//
    //    client->OnDeviceInfoCallback(serviceType, jsonTxtRecord);
    //}
    //else {
    //    std::cerr << "Error resolving service: " << errorCode << std::endl;
    //}
}

void DNSSD_API BrowseCallback(DNSServiceRef serviceRef,
                              DNSServiceFlags flags,
                              uint32_t interfaceIndex,
                              DNSServiceErrorType errorCode,
                              const char* serviceName,
                              const char* regtype,
                              const char* replyDomain,
                              void* context)
{
    if (errorCode == kDNSServiceErr_NoError) {
        if (flags & kDNSServiceFlagsAdd) {
            std::cout << "Discovered service: " << "interface-" << interfaceIndex << " " << serviceName << "." << regtype << replyDomain << std::endl;
            DNSServiceRef resolveRef;
            DNSServiceResolve(&resolveRef, 0, interfaceIndex, serviceName, regtype, replyDomain, ResolveCallback, context);
            DNSServiceProcessResult(resolveRef);
        }
        else {
            std::cout << "Service removed: " << serviceName << "." << regtype << "." << replyDomain << std::endl;
        }
    }
    else {
        std::cerr << "Error discovering service: " << errorCode << std::endl;
    }
}

MDNSClient::MDNSClient()
{
}

MDNSClient::~MDNSClient()
{
    for (auto& pair : m_browseRunning) {
        pair.second = false;
    }
    for (auto& pair : m_browseThreads) {
        if (pair.second.joinable()) {
            pair.second.join();
        }
    }

    for (auto& ref : m_registerRefs) {
        DNSServiceRefDeallocate(ref.second);
    }

    for (auto& ref : m_browseRefs) {
        DNSServiceRefDeallocate(ref.second);
    }
}

int MDNSClient::RegisterService(const std::string& name, const std::string& regtype, uint16_t port, const std::string& jsonString)
{
    if (m_registerRefs.find(name + regtype) != m_registerRefs.end()) {
        std::cerr << "Service already registered: " << name << " with type: " << regtype << std::endl;
        return kDNSServiceErr_AlreadyRegistered;
    }

    DNSServiceErrorType errorCode;
    DNSServiceRef serviceRef;
    TXTRecordRef txtRecord;
    TXTRecordCreate(&txtRecord, 0, nullptr);

    if (!jsonString.empty()) {
        // 解析JSON字符串
        rapidjson::Document doc;
        doc.Parse(jsonString.c_str());

        if (!doc.HasParseError()) {
            for (rapidjson::Value::ConstMemberIterator itr = doc.MemberBegin(); itr != doc.MemberEnd(); ++itr) {
                std::string key = itr->name.GetString();
                std::string value = itr->value.GetString();
                TXTRecordSetValue(&txtRecord, key.c_str(), value.size(), value.c_str());
            }
        }
        else {
            std::cerr << "Error parsing JSON: " << doc.GetParseError() << std::endl;
            return kDNSServiceErr_Invalid;
        }
    }

    errorCode = DNSServiceRegister(&serviceRef, 0, 0, name.c_str(), regtype.c_str(), "", nullptr,
                                   htons(port), TXTRecordGetLength(&txtRecord),
                                   TXTRecordGetBytesPtr(&txtRecord), nullptr, nullptr);
    if (errorCode != kDNSServiceErr_NoError) {
        std::cerr << "Error registering service: " << errorCode << std::endl;
    }
    else {
        m_registerRefs[name + regtype] = serviceRef;
        std::cout << "Service registered: " << name << " with type: " << regtype << std::endl;
    }

    TXTRecordDeallocate(&txtRecord);
    return errorCode;
}

void MDNSClient::UnregisterService(const std::string& name, const std::string& regtype)
{
    auto key = name + regtype;
    auto it = m_registerRefs.find(key);
    if (it != m_registerRefs.end()) {
        DNSServiceRefDeallocate(it->second);
        m_registerRefs.erase(it);
        std::cout << "Service unregistered: " << name << " with type: " << regtype << std::endl;
    }
    else {
        std::cerr << "Service not found: " << name << " with type: " << regtype << std::endl;
    }
}

void MDNSClient::StartBrowseService(const std::string& regtype, DeviceInfoCallback callback)
{
    if (m_browseThreads.find(regtype) != m_browseThreads.end()) {
        std::cerr << "Service discovery already running: " << regtype << std::endl;
        m_deviceInfoCallbacks[regtype].push_back(callback);
        return;
    }

    m_deviceInfoCallbacks[regtype].push_back(callback);

    m_browseRunning[regtype] = true;
    m_browseThreads[regtype] = std::thread([this, regtype]() {

        DNSServiceErrorType errorCode;
        DNSServiceRef serviceRef;

        errorCode = DNSServiceBrowse(&serviceRef, 0, 0, regtype.c_str(), "local.", BrowseCallback, this);
        if (errorCode != kDNSServiceErr_NoError) {
            std::cerr << "Error discovering service: " << errorCode << std::endl;
            return;
        }

        m_browseRefs[regtype] = serviceRef;
        std::cout << "Started browsing for service: " << regtype << std::endl;

        int dns_sd_fd = DNSServiceRefSockFD(serviceRef);
        if (dns_sd_fd == -1) {
            std::cerr << "Error getting socket file descriptor: " << strerror(errno) << std::endl;
            DNSServiceRefDeallocate(serviceRef);
            return;
        }

        while (m_browseRunning[regtype]) {
            fd_set read_fds;
            FD_ZERO(&read_fds);
            FD_SET(dns_sd_fd, &read_fds);
            struct timeval tv = { 1, 0 }; // 1 second timeout

            int result = select(dns_sd_fd + 1, &read_fds, nullptr, nullptr, &tv);
            if (result > 0) {
                if (FD_ISSET(dns_sd_fd, &read_fds)) {
                    errorCode = DNSServiceProcessResult(serviceRef);
                    if (errorCode != kDNSServiceErr_NoError
                        && errorCode != kDNSServiceErr_ServiceNotRunning) {
                        std::cerr << "Error processing result: " << errorCode << std::endl;
                        break;
                    }
                }
            }
            else if (result == 0) {
                // Timeout, continue checking browseRunning
                continue;
            }
            else {
                std::cerr << "Error in select: " << strerror(errno) << std::endl;
                break;
            }
        }

        DNSServiceRefDeallocate(serviceRef);
        m_browseRefs.erase(regtype);
        std::cout << "Service discovery thread stopped: " << regtype << std::endl;
    });
}

void MDNSClient::StopBrowseService(const std::string& regtype)
{
    if (m_browseRunning.find(regtype) != m_browseRunning.end()) {
        m_browseRunning[regtype] = false;

        // 关闭套接字以唤醒阻塞的 select
        if (m_browseRefs.find(regtype) != m_browseRefs.end()) {
            int dns_sd_fd = DNSServiceRefSockFD(m_browseRefs[regtype]);
            if (dns_sd_fd != -1) {
#if defined(WIN32)
                closesocket(dns_sd_fd);
#else
                close(dns_sd_fd);
#endif
            }
        }

        if (m_browseThreads[regtype].joinable()) {
            m_browseThreads[regtype].join();
        }
        m_browseThreads.erase(regtype);
        m_browseRunning.erase(regtype);
        m_deviceInfoCallbacks.erase(regtype);
        std::cout << "Service discovery stopped: " << regtype << std::endl;
    }
    else {
        std::cerr << "Service discovery not found: " << regtype << std::endl;
    }
}

void MDNSClient::OnDeviceInfoCallback(const std::string& regtype, const std::string& deviceInfo)
{
    auto it = m_deviceInfoCallbacks.find(regtype);
    if (it != m_deviceInfoCallbacks.end()) {
        for (const auto& callback : it->second) {
            callback(deviceInfo); // Call each callback function with deviceInfo
        }
    }
    else {
        std::cerr << "No callbacks found for regtype: " << regtype << std::endl;
    }
}