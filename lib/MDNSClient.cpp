#include "MDNSClient.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
//#include "logger.h"
#define ANDROID

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

std::mutex g_browseRefMutex;

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

int MDNSClient::RegisterService(const std::string& name,
                                const std::string& regtype,
                                uint16_t port,
                                const std::string& jsonString)
{
    if (m_registerRefs.find(name + regtype) != m_registerRefs.end()) {
        //ErrorL << "Service already registered: " << name << " with type: " << regtype << std::endl;
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
            //ErrorL << "Error parsing JSON: " << doc.GetParseError() << std::endl;
            return kDNSServiceErr_Invalid;
        }
    }

    errorCode = DNSServiceRegister(&serviceRef, 0, 0, name.c_str(), regtype.c_str(), "", nullptr,
                                   htons(port), TXTRecordGetLength(&txtRecord),
                                   TXTRecordGetBytesPtr(&txtRecord), nullptr, nullptr);
    if (errorCode != kDNSServiceErr_NoError) {
        //ErrorL << "Error registering service: " << errorCode << std::endl;
        LOGE("Error registering service: %d", errorCode);
    }
    else {
        m_registerRefs[name + regtype] = serviceRef;
        //DebugL << "Service registered: " << name << " with type: " << regtype << std::endl;
        LOGI("Service registered: %s with type: %s", name.c_str(), regtype.c_str());
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
        //DebugL << "Service unregistered: " << name << " with type: " << regtype << std::endl;
        LOGI("Service unregistered: %s with type: %s", name.c_str(), regtype.c_str());
    }
    else {
        //DebugL << "Service not found: " << name << " with type: " << regtype << std::endl;
        LOGI("Service not found: %s with type: %s", name.c_str(), regtype.c_str());
    }
}

void MDNSClient::StartBrowseService(const std::string& regtype, DeviceInfoCallback callback)
{
    if (m_browseThreads.find(regtype) != m_browseThreads.end()) {
        LOGE("Service discovery already running: %s", regtype.c_str());
        //m_deviceInfoCallbacks[regtype].push_back(callback);
        return;
    }

    m_deviceInfoCallbacks[regtype].push_back(callback);

    m_browseRunning[regtype] = true;
    m_browseThreads[regtype] = std::thread([this, regtype]() {

        DNSServiceErrorType errorCode;
        DNSServiceRef serviceRef;

        //auto svcContext = std::make_shared<ServiceContext>(ServiceContext{ this, regtype, "" }); // 使用 shared_ptr 管理 ServiceContext
        ServiceContext* svcContext = new ServiceContext{ this, regtype, "" };
        errorCode = DNSServiceBrowse(&serviceRef, 0, 0, regtype.c_str(), "local.", BrowseCallback, svcContext);
        if (errorCode != kDNSServiceErr_NoError) {
            //ErrorL << "Error discovering service: " << errorCode << std::endl;
            LOGE("Error discovering service: %d", errorCode);
            delete svcContext; // 发生错误时释放 ServiceContext
            return;
        }

        m_browseRefs[regtype] = serviceRef;
        //DebugL << "Started browsing for service: " << regtype << std::endl;
        LOGI("Started browsing for service: %s", regtype.c_str());

        int dns_sd_fd = DNSServiceRefSockFD(serviceRef);
        //DebugL << "dns_sd_fd: "<< dns_sd_fd << std::endl;
        LOGI("dns_sd_fd: %d", dns_sd_fd);

        if (dns_sd_fd == -1) {
            LOGE("Error getting socket file descriptor: %s", strerror(errno));
            DNSServiceRefDeallocate(serviceRef);
            return;
        }

        while (m_browseRunning[regtype]) {
#if defined(ANDROID)
            //Android上DNSServiceProcessResult无效，跳过select流程
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
#else
            fd_set read_fds;
            FD_ZERO(&read_fds);
            FD_SET(dns_sd_fd, &read_fds);
            struct timeval tv = { 1, 0 }; // 1 second timeout

            int result = select(dns_sd_fd + 1, &read_fds, nullptr, nullptr, &tv);
            if (result > 0) {
                if (FD_ISSET(dns_sd_fd, &read_fds)) {
                    //LOGI("Before DNSServiceProcessResult... ");
                    errorCode = DNSServiceProcessResult(serviceRef);
                    //LOGI("DNSServiceProcessResult  errorCode: %d", errorCode);
                    if (errorCode != kDNSServiceErr_NoError
                        && errorCode != kDNSServiceErr_ServiceNotRunning) {
                        LOGE("Error processing result: %d", errorCode);
                        break;
                    }
                }
            }
            else if (result == 0) {
                // Timeout, continue checking browseRunning
                continue;
            }
            else {
                LOGE("Error in select: %s", strerror(errno));
                break;
            }
#endif
        }

        if (svcContext) {
            delete svcContext;
            svcContext = nullptr;
        }

        std::lock_guard<std::mutex> lock(g_browseRefMutex);
        DNSServiceRefDeallocate(m_browseRefs[regtype]);
        m_browseRefs.erase(regtype);
        LOGI("[003]Service discovery thread stopped: %s", regtype.c_str());
    });
}

void MDNSClient::StopBrowseService(const std::string& regtype)
{
    if (m_browseRunning.find(regtype) != m_browseRunning.end()) {
        m_browseRunning[regtype] = false;
        {
            std::lock_guard<std::mutex> lock(g_browseRefMutex);
            // 关闭套接字以唤醒阻塞的 select
            if (m_browseRefs.find(regtype) != m_browseRefs.end()) {
                int dns_sd_fd = DNSServiceRefSockFD(m_browseRefs[regtype]);
                if (dns_sd_fd != -1) {
                    LOGI("Before closesocket... dns_sd_fd: %d", dns_sd_fd);
#if defined(WIN32)
                    closesocket(dns_sd_fd);
#else
                    close(dns_sd_fd);
#endif
                }
            }
        }

        if (m_browseThreads[regtype].joinable()) {
            m_browseThreads[regtype].join();
        }
        m_browseThreads.erase(regtype);
        m_browseRunning.erase(regtype);
        m_deviceInfoCallbacks.erase(regtype);
        LOGI("Service discovery stopped: %s", regtype.c_str());
    }
    else {
        //DebugL << "Service discovery not found: " << regtype << std::endl;
        LOGI("Service discovery not found: %s", regtype.c_str());
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
        LOGI("No callbacks found for regtype: %s",regtype.c_str());
    }
}

std::mutex g_browsecbMtx;

void DNSSD_API MDNSClient::BrowseCallback(
        DNSServiceRef serviceRef,
        DNSServiceFlags flags,
        uint32_t interfaceIndex,
        DNSServiceErrorType errorCode,
        const char* serviceName,
        const char* regtype,
        const char* replyDomain,
        void* context)
{
#if defined(ANDROID)
    //std::lock_guard<std::mutex> lock(g_browsecbMtx);
#endif
    LOGI("Enter MDNSClient::BrowseCallback");
    if (errorCode == kDNSServiceErr_NoError) {
        ServiceContext* svcContext = static_cast<ServiceContext*>(context);
        if (flags & kDNSServiceFlagsAdd) {
            LOGI("Discovered service: interface-%d %s.%s%s", interfaceIndex, serviceName, regtype, replyDomain);
            svcContext->action = "add";
        }
        else {
            LOGI("Service removed: %s.%s.%s", serviceName, regtype, replyDomain);
            svcContext->action = "remove";
        }

#if defined(ANDROID)
        // Embedded版本DNSServiceRef每次都会malloc内存，不同于Windows等平台DNSServiceRef是共用的
        // 在回调中最后进行释放比较合适
        DNSServiceRef resolveRef;
        DNSServiceResolve(&serviceRef, 0, interfaceIndex, serviceName, regtype, replyDomain, ResolveCallback, context);
        LOGI("Create resolveRef address: %p", static_cast<void*>(resolveRef));
#else
        DNSServiceResolve(&serviceRef, 0, interfaceIndex, serviceName, regtype, replyDomain, ResolveCallback, context);
            DNSServiceProcessResult(serviceRef);
#endif
    }
    else {
        LOGE("Error discovering service: %d", errorCode);
    }
}

void DNSSD_API MDNSClient::ResolveCallback(
        DNSServiceRef serviceRef,
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
#if defined(ANDROID)
    //std::lock_guard<std::mutex> lock(g_browsecbMtx);
#endif
    LOGI("Enter MDNSClient::ResolveCallback");
    //auto svcContext = std::shared_ptr<MDNSClient::ServiceContext>(static_cast<MDNSClient::ServiceContext*>(context));
    ServiceContext* svcContext = static_cast<ServiceContext*>(context);
    //LOGI("svcContext address: %p", static_cast<void*>(svcContext));
    if (errorCode == kDNSServiceErr_NoError) {
        //LOGI("before  MDNSClient* client = svcContext->client;");
        MDNSClient* client = svcContext->client;
        //LOGI("after  MDNSClient* client = svcContext->client;");
        if (client == nullptr) {
            LOGI("client == nullptr");
            return;
        }

        if (fullname == nullptr) {
            LOGI("fullname == nullptr");
            //return;
        }

        LOGI("svcContext->fullname: %s", svcContext->fullname.c_str());
        if(svcContext->fullname.empty())
        {
            //LOGI("svcContext->fullname is empty()");
        }

        std::string jsonTxtRecord = client->parseTXTRecordToJson(txtRecord, txtLen);

        std::string fullServiceName = fullname;
        size_t start = fullServiceName.find(".") + 1;
        size_t end = fullServiceName.find(".", start);
        end = fullServiceName.find(".", end + 1);

        std::string serviceType = fullServiceName.substr(start, end - start);

        LOGI("jsonTxtRecord: %s", jsonTxtRecord.c_str());
        // 保存设备信息
        client->m_deviceInfos[fullname] = { jsonTxtRecord, ntohs(port) };

        // 更新 ServiceContext 的 fullname
        svcContext->fullname = fullname;

#if defined(ANDROID)
        // Embedded版本DNSServiceRef每次都会malloc内存，不同于Windows等平台DNSServiceRef是共用的
        // 在回调中最后进行释放比较合适
        DNSServiceRef addressRef;
        //LOGI("Create addressRef...");

        //因为不是Windows平台这种顺序执行，重新创建一个新的上下文
        ServiceContext* newSvcContext = new ServiceContext{ client, svcContext->regtype, fullname, svcContext->action};
#endif
        // 解析 IP 地址
        DNSServiceErrorType addrInfoError = DNSServiceGetAddrInfo(
#if defined(ANDROID)
                &addressRef,
#else
                &serviceRef,
#endif
                0, // flags
                interfaceIndex,
                kDNSServiceProtocol_IPv4, // 仅解析 IPv4 地址
                hosttarget,
                AddrInfoCallback,
#if defined(ANDROID)
                newSvcContext // 使用独立的上下文
#else
                svcContext // 使用相同的上下文
#endif
        );

        if (addrInfoError != kDNSServiceErr_NoError) {
            LOGE("Error getting address info: %d", addrInfoError);
        }
        else {
            DNSServiceProcessResult(serviceRef);
        }
    }
    else {
        LOGE("Error resolving service: %d", errorCode);
    }

#if defined(ANDROID)
    //释放DNSServiceResolve申请的DNSServiceRef内存
    //LOGI("Release resolveRef...");
    DNSServiceRefDeallocate(serviceRef);
#endif
}

void DNSSD_API MDNSClient::AddrInfoCallback(
        DNSServiceRef serviceRef,
        DNSServiceFlags flags,
        uint32_t interfaceIndex,
        DNSServiceErrorType errorCode,
        const char* hostname,
        const struct sockaddr* address,
        uint32_t ttl,
        void* context)
{
    LOGI("Enter MDNSClient::AddrInfoCallback");
#if defined(ANDROID)
    //std::lock_guard<std::mutex> lock(g_browsecbMtx);
#endif
    //auto svcContext = std::shared_ptr<MDNSClient::ServiceContext>(static_cast<MDNSClient::ServiceContext*>(context));
    ServiceContext* svcContext = static_cast<ServiceContext*>(context);
    if (errorCode == kDNSServiceErr_NoError) {
        char ipStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(((struct sockaddr_in*)address)->sin_addr), ipStr, INET_ADDRSTRLEN);

        LOGI("Resolved IP address: %s", ipStr);

        //LOGI("before  MDNSClient* client = svcContext->client;");
        MDNSClient* client = svcContext->client;
        //LOGI("after  MDNSClient* client = svcContext->client;");
        if (client == nullptr) {
            //LOGI("client == nullptr ");
            return;
        }

        LOGI("svcContext->fullname: %s", svcContext->fullname.c_str());
        if(svcContext->fullname.empty()){
            //LOGI("svcContext->fullname is empty()");
        }

        // 找到当前解析的设备信息
        auto it = client->m_deviceInfos.find(svcContext->fullname);
        if (it != client->m_deviceInfos.end()) {
            auto& deviceInfo = it->second;

            std::string deviceName = svcContext->fullname;
            size_t pos = deviceName.find(svcContext->regtype);
            if (pos > 1) {
                deviceName = deviceName.substr(0, pos-1);
            }

            // 解析后的 JSON 字符串
            rapidjson::Document document;
            document.Parse(deviceInfo.jsonTxtRecord.c_str());
            rapidjson::Document::AllocatorType& allocator = document.GetAllocator();

            // 添加 IP 地址和端口号
            rapidjson::Value ipKey("ip", allocator);
            rapidjson::Value ipValue(ipStr, allocator);
            document.AddMember(ipKey, ipValue, allocator);

            rapidjson::Value portKey("port", allocator);
            rapidjson::Value portValue(deviceInfo.port);
            document.AddMember(portKey, portValue, allocator);

            document.AddMember("name", rapidjson::StringRef(deviceName.c_str()),
                               document.GetAllocator());
            document.AddMember("protol", rapidjson::StringRef(svcContext->regtype.c_str()),
                               document.GetAllocator());
            document.AddMember("action", rapidjson::StringRef(svcContext->action.c_str()),
                               document.GetAllocator());
            // 将 JSON 对象转换为字符串
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            document.Accept(writer);
            std::string updatedJsonTxtRecord = buffer.GetString();

            LOGI("OnDeviceInfoCallback updatedJsonTxtRecord : %s", updatedJsonTxtRecord.c_str());

            client->OnDeviceInfoCallback(svcContext->regtype, updatedJsonTxtRecord);
        }
    }
    else {
        LOGE("Error in address info callback: %d", errorCode);
    }
#if defined(ANDROID)
    //释放DNSServiceGetAddrInfo申请的DNSServiceRef内存
    //LOGI("Release addressRef...");
    DNSServiceRefDeallocate(serviceRef);

    if(svcContext){
        delete svcContext;
        svcContext = nullptr;
    }

#endif
}

// 解析TXTRecord
std::string MDNSClient::parseTXTRecordToJson(const unsigned char* txtRecord, uint16_t txtLen) {
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