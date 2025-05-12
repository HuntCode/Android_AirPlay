#include "MDNSClient.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <iostream>
#include <cstring>
#include <mutex>

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <unistd.h>
#include <netdb.h> // for getaddrinfo
#include <arpa/inet.h> // for inet_ntop
#endif

#ifdef __ANDROID__
#include <android/log.h>

#define LOG_TAG "WLNativeLog"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#else
#include "log/logger.h"
#endif

std::mutex g_shareRefMutex;

DNSServiceErrorType g_error;
std::map<std::string, DNSServiceRef> g_shareRefs;


MDNSClient::MDNSClient() 
{
}

MDNSClient::~MDNSClient() 
{
    Release();
}

void MDNSClient::Release() {
    // 停止所有浏览线程并释放 DNSServiceRef
    for (auto& [regtype, context] : m_browseContexts) {
        context.running = false;

        for (auto& [instanceName, resolveRef] : context.resolveRefs) {
            if (resolveRef) {
                DNSServiceRefDeallocate(resolveRef);
                //DebugL << "Deallocated resolveRef for: " << instanceName;
                LOGI("Deallocated resolveRef for: %s", instanceName.c_str());
            }
        }

        if (context.serviceRef) {
            DNSServiceRefDeallocate(context.serviceRef);
            context.serviceRef = nullptr;
        }

        if (context.browseThread.joinable()) {
            context.browseThread.join();
        }
    }
    m_browseContexts.clear();

    // 释放注册服务资源
    for (auto& ref : m_registerRefs) {
        DNSServiceRefDeallocate(ref.second);
    }
    m_registerRefs.clear();

    // 释放共享连接
    //for (auto& ref : g_shareRefs) {
    //    DNSServiceRefDeallocate(ref.second);
    //}
    //g_shareRefs.clear();
}

void MDNSClient::AddResolveRef(const std::string& regtype, const std::string& instanceName, DNSServiceRef resolveRef) {
    std::lock_guard<std::mutex> lock(m_contextMutex);
    auto it = m_browseContexts.find(regtype);
    if (it == m_browseContexts.end()) {
        //DebugL << "Browse context not found for regtype: " << regtype;
        LOGI("Browse context not found for regtype: %s", regtype.c_str());
        return;
    }

    BrowseContext& ctx = it->second;

    // 先清理可能存在的旧的
    auto oldRefIt = ctx.resolveRefs.find(instanceName);
    if (oldRefIt != ctx.resolveRefs.end()) {
        //DebugL << "Deallocated old resolveRef for: " << instanceName;
        LOGI("Deallocated old resolveRef for: %s", instanceName.c_str());
        DNSServiceRefDeallocate(oldRefIt->second);
        ctx.resolveRefs.erase(oldRefIt);
    }

    ctx.resolveRefs[instanceName] = resolveRef;
}

void MDNSClient::RemoveResolveRef(const std::string& regtype, const std::string& instanceName) {

    std::string extractInstanceName = ExtractInstanceName(instanceName);

    std::lock_guard<std::mutex> lock(m_contextMutex);
    auto it = m_browseContexts.find(regtype);
    if (it == m_browseContexts.end()) {
        //DebugL << "Browse context not found for regtype: " << regtype;
        return;
    }

    BrowseContext& ctx = it->second;

    auto refIt = ctx.resolveRefs.find(extractInstanceName);
    if (refIt != ctx.resolveRefs.end()) {
        if (refIt->second) {
            DNSServiceRefDeallocate(refIt->second);
            //DebugL << "Deallocated resolveRef for " << extractInstanceName;
        }
        ctx.resolveRefs.erase(refIt);
    }
}

int MDNSClient::RegisterService(const std::string& name, 
    const std::string& regtype,
    uint16_t port,
    const std::string& jsonString)
{
    if (m_registerRefs.find(name + regtype) != m_registerRefs.end()) {
        //DebugL << "Service already registered: " << name << " with type: " << regtype << std::endl;
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
    }
    else {
        m_registerRefs[name + regtype] = serviceRef;
	    //DebugL << "Service registered: " << name << " with type: " << regtype << std::endl;
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
    }
    else {
        //DebugL << "Service not found: " << name << " with type: " << regtype << std::endl;
    }
}

void MDNSClient::StartBrowseService(const std::string& regtype, DeviceInfoCallback callback)
{
    // 前一次异常数据，需要清理
    if(m_browseContexts.find(regtype) != m_browseContexts.end() && m_browseContexts[regtype].cleanBrowseRef) {
        CleanupBrowseContext(regtype);
    }

    auto& ctx = m_browseContexts[regtype];

    if (ctx.running) {
        //DebugL << "Service discovery already running: " << regtype << std::endl;
        LOGI("Service discovery already running: %s", regtype.c_str());
        return;
    }

    // 添加回调并设置运行标志
    ctx.callbacks.push_back(callback);
    ctx.running = true;

    ctx.browseThread = std::thread([this, regtype]() {
        DNSServiceErrorType errorCode;
        DNSServiceRef browseRef;

        ServiceContext* svcContext = new ServiceContext{ this, regtype, "", ""};
        errorCode = DNSServiceBrowse(&browseRef, 0, 0, regtype.c_str(), "", BrowseCallback, svcContext);
        if (errorCode != kDNSServiceErr_NoError) {
	        LOGI("Error discovering service: %d", errorCode);
            delete svcContext; // 发生错误时释放 ServiceContext
            svcContext = nullptr;
            m_browseContexts[regtype].cleanBrowseRef = true;
            return;
        }

        m_browseContexts[regtype].serviceRef = browseRef;
        //DebugL << "Started browsing for service: " << regtype << std::endl;
        LOGI("Started browsing for service: %s", regtype.c_str());

        while (m_browseContexts[regtype].running) {
            //Android上DNSServiceProcessResult无效，跳过select流程
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        if (svcContext) {
            delete svcContext;
            svcContext = nullptr;
        }

        //DebugL << "Service discovery Thread stopped: " << regtype << std::endl;
        LOGI("Service discovery Thread stopped: %s", regtype.c_str());
    });
}

void MDNSClient::StopBrowseService(const std::string& regtype)
{
    auto it = m_browseContexts.find(regtype);
    if (it != m_browseContexts.end()) {
        BrowseContext& ctx = it->second;
        ctx.running = false;

        for (auto& [instanceName, resolveRef] : ctx.resolveRefs) {
            if (resolveRef) {
                DNSServiceRefDeallocate(resolveRef);
                //DebugL << "Deallocated resolveRef for: " << instanceName;
            }
        }

        if (ctx.serviceRef) {
            DNSServiceRefDeallocate(ctx.serviceRef);
            ctx.serviceRef = nullptr;
        }

        if (ctx.browseThread.joinable()) {
            ctx.browseThread.join();
        }

        m_browseContexts.erase(it);
        //DebugL << "Service discovery stopped: " << regtype << std::endl;
        LOGI("Service discovery stopped: %s", regtype.c_str());
    }
	else {
		//DebugL << "Service discovery not found: " << regtype << std::endl;
	}
}

void MDNSClient::OnDeviceInfoCallback(const std::string& regtype, const std::string& deviceInfo)
{
    auto it = m_browseContexts.find(regtype);
    if (it != m_browseContexts.end()) {
        for (const auto& callback : it->second.callbacks) {
            callback(deviceInfo); // Call each callback function with deviceInfo
        }
    }
    else {
        //DebugL << "No callbacks found for regtype: " << regtype << std::endl;
        LOGI("No callbacks found for regtype: %s", regtype.c_str());
    }
}

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
    //DebugL << "Enter MDNSClient::BrowseCallback";
    if (errorCode == kDNSServiceErr_NoError) {
        ServiceContext* svcContext = static_cast<ServiceContext*>(context);
        if (flags & kDNSServiceFlagsAdd) {
	        //DebugL << "Service add: " << "interface-" << interfaceIndex << " " << serviceName << "." << regtype << replyDomain;
	        LOGI("Service add: interface-%d %s %s.%s", interfaceIndex, serviceName, regtype, replyDomain);
            svcContext->action = "add";
        }
        else {
            //DebugL << "Service remove: " << "interface-" << interfaceIndex << " " << serviceName << "." << regtype << replyDomain;
            LOGI("Service remove: interface-%d %s %s.%s", interfaceIndex, serviceName, regtype, replyDomain);
            svcContext->action = "remove";
        }

        DNSServiceRef resolveRef;
        DNSServiceErrorType error = DNSServiceResolve(&resolveRef, 0, interfaceIndex, serviceName, regtype, replyDomain, ResolveCallback, svcContext);
		if (error != kDNSServiceErr_NoError) {
			//ErrorL << "Error resolving service: " << error << std::endl;
			LOGI("Error resolving service: %d", error);
            //出错时resolveRef为空，不需要释放
            //DNSServiceRefDeallocate(resolveRef);
		}
        else {
            svcContext->client->AddResolveRef(svcContext->regtype, serviceName, resolveRef);
        }
    }
    else {
        //DebugL << "Error discovering service: " << errorCode << std::endl;
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
    ServiceContext* svcContext = static_cast<ServiceContext*>(context);
    MDNSClient* client = svcContext->client;
    if (errorCode == kDNSServiceErr_NoError) {
        if (client == nullptr) {
            //DebugL << "client == nullptr ";
            return;
        }

        std::string jsonTxtRecord = client->parseTXTRecordToJson(txtRecord, txtLen);

        std::string fullServiceName = fullname;
        size_t start = fullServiceName.find(".") + 1;
        size_t end = fullServiceName.find(".", start);
        end = fullServiceName.find(".", end + 1);

        std::string serviceType = fullServiceName.substr(start, end - start);

        client->m_deviceInfos[fullname] = { jsonTxtRecord, ntohs(port) };

        // 更新 ServiceContext 的 fullname
        svcContext->fullname = fullname;

		DNSServiceRef addressRef;
        ServiceContext* newSvcContext = new ServiceContext{ client, svcContext->regtype, fullname, svcContext->action };

        // 解析 IP 地址
        DNSServiceErrorType addrInfoError = DNSServiceGetAddrInfo(
            &addressRef,
			0, // flags
            interfaceIndex,
            kDNSServiceProtocol_IPv4, // 仅解析 IPv4 地址
            hosttarget,
            AddrInfoCallback,
            newSvcContext // 使用独立的上下文
        );

        if (addrInfoError != kDNSServiceErr_NoError) {
            //DebugL << "Error getting address info: " << addrInfoError;
            LOGI("Error getting address info: %d", addrInfoError);
            delete newSvcContext;
            newSvcContext = nullptr;
        }
    }
    else {
        //DebugL << "Error resolving service: " << errorCode;
    }

    std::string instanceName = fullname;
    size_t pos = instanceName.find(svcContext->regtype);
    if (pos > 1) {
        instanceName = instanceName.substr(0, pos - 1);
    }
    client->RemoveResolveRef(svcContext->regtype, instanceName);
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
    // 使用独立的上下文
    ServiceContext* svcContext = static_cast<ServiceContext*>(context);
    if (errorCode == kDNSServiceErr_NoError) {
        char ipStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(((struct sockaddr_in*)address)->sin_addr), ipStr, INET_ADDRSTRLEN);

        MDNSClient* client = svcContext->client;
        if (client == nullptr) {
            //DebugL << "client == nullptr ";
            return;
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

	        //DebugL << "OnDeviceInfoCallback updatedJsonTxtRecord : " << updatedJsonTxtRecord;
	        //DebugL << "OnDeviceInfoCallback updatedJsonTxtRecord name: " << deviceName << " action: " << svcContext->action;
	        LOGI("OnDeviceInfoCallback updatedJsonTxtRecord name: %s action: %s", deviceName.c_str(),  svcContext->action.c_str());

            client->OnDeviceInfoCallback(svcContext->regtype, updatedJsonTxtRecord);
        }
    }
    else {
	    //DebugL << "Error in address info callback: " << errorCode << std::endl;
    }

    // 释放回调前DNSServiceRef addressRef;
    DNSServiceRefDeallocate(serviceRef);

    //用完后释放独立上下文
    if (svcContext) {
        delete svcContext;
        svcContext = nullptr;
    }
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

std::string MDNSClient::ExtractInstanceName(const std::string& fullname) {
    std::string result;
    size_t i = 0;
    while (i < fullname.length()) {
        if (fullname[i] == '\\' && i + 3 < fullname.length() &&
            isdigit(fullname[i + 1]) && isdigit(fullname[i + 2]) && isdigit(fullname[i + 3])) {
            // 提取三位数字并转换为字符
            int val = std::stoi(fullname.substr(i + 1, 3));
            result += static_cast<char>(val);
            i += 4;
        }
        else {
            result += fullname[i];
            ++i;
        }
    }

    return result;
}

std::string MDNSClient::Utf8ToAnsi(const std::string& utf8Str) {
#if defined(WIN32)
    int wideLen = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, nullptr, 0);
    std::wstring wideStr(wideLen, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, &wideStr[0], wideLen);

    int ansiLen = WideCharToMultiByte(CP_ACP, 0, wideStr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string ansiStr(ansiLen, 0);
    WideCharToMultiByte(CP_ACP, 0, wideStr.c_str(), -1, &ansiStr[0], ansiLen, nullptr, nullptr);

    ansiStr.pop_back(); // 去掉末尾的 \0
    return ansiStr;
#else
    return utf8Str; //TODO: 其他平台未实现
#endif
}

void MDNSClient::CleanupBrowseContext(const std::string& regtype)
{
    auto it = m_browseContexts.find(regtype);
    if (it != m_browseContexts.end()) {
        BrowseContext& ctx = it->second;
        ctx.running = false;

        if (ctx.serviceRef) {
            DNSServiceRefDeallocate(ctx.serviceRef);
            ctx.serviceRef = nullptr;
        }

        for (auto& [_, resolveRef] : ctx.resolveRefs) {
            if (resolveRef) {
                DNSServiceRefDeallocate(resolveRef);
            }
        }

        if (ctx.browseThread.joinable()) {
            ctx.browseThread.join();
        }

        m_browseContexts.erase(it);
    }
}