/**
 * RPiPlay - An open-source AirPlay mirroring server for Raspberry Pi
 * Copyright (C) 2019 Florian Draschbacher
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 */

#include <stddef.h>
#include <cstring>
#include <signal.h>
#ifdef WIN32
#include <inttypes.h>
#include <io.h>
#else
#include <unistd.h>
#endif
#include <string>
#include <vector>
#include <fstream>

#ifdef WIN32

#include <winsock2.h>
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")

#else

#include <sys/socket.h>
#include <ifaddrs.h>
#ifdef __linux__
#include <netpacket/packet.h>
#else
#include <net/if_dl.h>   /* macOS and *BSD */
#endif

#endif

#include "log.h"
#include "lib/raop.h"
#include "lib/stream.h"
#include "lib/logger.h"
#include "lib/dnssd.h"
#include "renderers/video_renderer.h"
#include "renderers/audio_renderer.h"

#include "airplay.h"

#define VERSION "1.2"

#define DEFAULT_NAME "HHAirplay"
#define DEFAULT_BACKGROUND_MODE BACKGROUND_MODE_ON
#define DEFAULT_AUDIO_DEVICE AUDIO_DEVICE_HDMI
#define DEFAULT_LOW_LATENCY false
#define DEFAULT_DEBUG_LOG false
#define DEFAULT_ROTATE 0
#define DEFAULT_FLIP FLIP_NONE
#define DEFAULT_HW_ADDRESS { (char) 0x48, (char) 0x5d, (char) 0x60, (char) 0x7c, (char) 0xee, (char) 0x22 }

int start_server(std::vector<char> hw_addr, std::string name, bool debug_log,
    video_renderer_config_t const* video_config, audio_renderer_config_t const* audio_config);

int stop_server();

typedef video_renderer_t* (*video_init_func_t)(logger_t* logger, video_renderer_config_t const* config);
typedef audio_renderer_t* (*audio_init_func_t)(logger_t* logger, video_renderer_t* video_renderer, audio_renderer_config_t const* config);

typedef struct video_renderer_list_entry_s {
    const char* name;
    const char* description;
    video_init_func_t init_func;
} video_renderer_list_entry_t;

typedef struct audio_renderer_list_entry_s {
    const char* name;
    const char* description;
    audio_init_func_t init_func;
} audio_renderer_list_entry_t;

static bool running = false;
static dnssd_t* dnssd = NULL;
static raop_t* raop = NULL;
static video_init_func_t video_init_func = NULL;
static audio_init_func_t audio_init_func = NULL;
static video_renderer_t* video_renderer = NULL;
static audio_renderer_t* audio_renderer = NULL;
static logger_t* render_logger = NULL;

static const video_renderer_list_entry_t video_renderers[] = {
#if defined(HAS_RPI_RENDERER)
    {"rpi", "Raspberry Pi OpenMAX accelerated H.264 renderer", video_renderer_rpi_init},
#endif
#if defined(HAS_GSTREAMER_RENDERER)
    {"gstreamer", "GStreamer H.264 renderer", video_renderer_gstreamer_init},
#endif
#if defined(HAS_DUMMY_RENDERER)
    {"dummy", "Dummy renderer; does not actually display video", video_renderer_dummy_init},
#endif
#if defined(HAS_HH_RENDERER)
    {"hh", "HH video renderer", video_renderer_hh_init},
#endif
};

static const audio_renderer_list_entry_t audio_renderers[] = {
#if defined(HAS_RPI_RENDERER)
    {"rpi", "AAC renderer using fdk-aac for decoding and OpenMAX for rendering", audio_renderer_rpi_init},
#endif
#if defined(HAS_GSTREAMER_RENDERER)
    {"gstreamer", "GStreamer audio renderer", audio_renderer_gstreamer_init},
#endif
#if defined(HAS_DUMMY_RENDERER)
    {"dummy", "Dummy renderer; does not actually play audio", audio_renderer_dummy_init},
#endif
#if defined(HAS_HH_RENDERER)
    {"hh", "HH audio renderer", audio_renderer_hh_init},
#endif
};

static int parse_hw_addr(std::string str, std::vector<char>& hw_addr) {
    for (int i = 0; i < str.length(); i += 3) {
        hw_addr.push_back((char)stol(str.substr(i), NULL, 16));
    }
    return 0;
}

static std::string find_mac() {
    /*  finds the MAC address of a network interface *
    *  in a Windows, Linux, *BSD or macOS system.   */
    std::string mac = "";
    char str[3];
#ifdef _WIN32
    ULONG buflen = sizeof(IP_ADAPTER_ADDRESSES);
    PIP_ADAPTER_ADDRESSES addresses = (IP_ADAPTER_ADDRESSES*)malloc(buflen);
    if (addresses == NULL) {
        return mac;
    }
    if (GetAdaptersAddresses(AF_UNSPEC, 0, NULL, addresses, &buflen) == ERROR_BUFFER_OVERFLOW) {
        free(addresses);
        addresses = (IP_ADAPTER_ADDRESSES*)malloc(buflen);
        if (addresses == NULL) {
            return mac;
    }
}
    if (GetAdaptersAddresses(AF_UNSPEC, 0, NULL, addresses, &buflen) == NO_ERROR) {
        for (PIP_ADAPTER_ADDRESSES address = addresses; address != NULL; address = address->Next) {
            if (address->PhysicalAddressLength != 6                 /* MAC has 6 octets */
                || (address->IfType != 6 && address->IfType != 71)  /* Ethernet or Wireless interface */
                || address->OperStatus != 1) {                      /* interface is up */
                continue;
            }
            mac.erase();
            for (int i = 0; i < 6; i++) {
                snprintf(str, sizeof(str), "%02x", int(address->PhysicalAddress[i]));
                mac = mac + str;
                if (i < 5) mac = mac + ":";
            }
            break;
        }
    }
    free(addresses);
    return mac;
#else
    struct ifaddrs* ifap, * ifaptr;
    int non_null_octets = 0;
    unsigned char octet[6];
    if (getifaddrs(&ifap) == 0) {
        for (ifaptr = ifap; ifaptr != NULL; ifaptr = ifaptr->ifa_next) {
            if (ifaptr->ifa_addr == NULL) continue;
#ifdef __linux__
            if (ifaptr->ifa_addr->sa_family != AF_PACKET) continue;
            struct sockaddr_ll* s = (struct sockaddr_ll*)ifaptr->ifa_addr;
            for (int i = 0; i < 6; i++) {
                if ((octet[i] = s->sll_addr[i]) != 0) non_null_octets++;
            }
#else    /* macOS and *BSD */
            if (ifaptr->ifa_addr->sa_family != AF_LINK) continue;
            unsigned char* ptr = (unsigned char*)LLADDR((struct sockaddr_dl*)ifaptr->ifa_addr);
            for (int i = 0; i < 6; i++) {
                if ((octet[i] = *ptr) != 0) non_null_octets++;
                ptr++;
            }
#endif
            if (non_null_octets) {
                mac.erase();
                for (int i = 0; i < 6; i++) {
                    snprintf(str, sizeof(str), "%02x", octet[i]);
                    mac = mac + str;
                    if (i < 5) mac = mac + ":";
                }
                break;
            }
        }
    }
    freeifaddrs(ifap);
#endif
    return mac;
}

static video_init_func_t find_video_init_func(const char* name) {
    for (int i = 0; i < sizeof(video_renderers) / sizeof(video_renderers[0]); i++) {
        if (!strcmp(name, video_renderers[i].name)) {
            return video_renderers[i].init_func;
        }
    }
    return NULL;
}

static audio_init_func_t find_audio_init_func(const char* name) {
    for (int i = 0; i < sizeof(audio_renderers) / sizeof(audio_renderers[0]); i++) {
        if (!strcmp(name, audio_renderers[i].name)) {
            return audio_renderers[i].init_func;
        }
    }
    return NULL;
}

// Server callbacks
extern "C" void conn_init(void* cls, unsigned int streamId) {
    if (video_renderer) video_renderer->funcs->update_background(video_renderer, 1, streamId);
}

extern "C" void conn_destroy(void* cls, unsigned int streamId) {
    if (video_renderer) video_renderer->funcs->update_background(video_renderer, -1, streamId);
}

extern "C" void audio_process(void* cls, raop_ntp_t * ntp, aac_decode_struct * data, unsigned int streamId) {
    if (audio_renderer != NULL) {
        audio_renderer->funcs->render_buffer(audio_renderer, ntp, data->data, data->data_len, data->pts, streamId);
    }
}

extern "C" void video_process(void* cls, raop_ntp_t * ntp, h264_decode_struct * data, unsigned int streamId) {
    if (video_renderer != NULL) {
        video_renderer->funcs->render_buffer(video_renderer, ntp, data->data, data->data_len, data->pts, data->frame_type, streamId);
    }
}

extern "C" void audio_flush(void* cls) {
    if (audio_renderer) audio_renderer->funcs->flush(audio_renderer);
}

extern "C" void video_flush(void* cls) {
    if (video_renderer) video_renderer->funcs->flush(video_renderer);
}

extern "C" void audio_set_volume(void* cls, float volume) {
    if (audio_renderer != NULL) {
        audio_renderer->funcs->set_volume(audio_renderer, volume);
    }
}

extern "C" void log_callback(void* cls, int level, const char* msg) {
    switch (level) {
    case LOGGER_DEBUG: {
        LOGD("%s", msg);
        break;
    }
    case LOGGER_WARNING: {
        LOGW("%s", msg);
        break;
    }
    case LOGGER_INFO: {
        LOGI("%s", msg);
        break;
    }
    case LOGGER_ERR: {
        LOGE("%s", msg);
        break;
    }
    default:
        break;
    }

}

int start_server(std::vector<char> hw_addr, std::string name, bool debug_log,
    video_renderer_config_t const* video_config, audio_renderer_config_t const* audio_config) {
    raop_callbacks_t raop_cbs;
    memset(&raop_cbs, 0, sizeof(raop_cbs));
    raop_cbs.conn_init = conn_init;
    raop_cbs.conn_destroy = conn_destroy;
    raop_cbs.audio_process = audio_process;
    raop_cbs.video_process = video_process;
    raop_cbs.audio_flush = audio_flush;
    raop_cbs.video_flush = video_flush;
    raop_cbs.audio_set_volume = audio_set_volume;

    raop = raop_init(10, &raop_cbs);
    if (raop == NULL) {
        LOGE("Error initializing raop!");
        return -1;
    }

    raop_set_log_callback(raop, log_callback, NULL);
    raop_set_log_level(raop, debug_log ? RAOP_LOG_DEBUG : LOGGER_INFO);

    render_logger = logger_init();
    logger_set_callback(render_logger, log_callback, NULL);
    logger_set_level(render_logger, debug_log ? LOGGER_DEBUG : LOGGER_INFO);

    if (video_config->low_latency) logger_log(render_logger, LOGGER_INFO, "Using low-latency mode");

    if ((video_renderer = video_init_func(render_logger, video_config)) == NULL) {
        LOGE("Could not init video renderer");
        return -1;
    }

    if (audio_config->device == AUDIO_DEVICE_NONE) {
        LOGI("Audio disabled");
    }
    else if ((audio_renderer = audio_init_func(render_logger, video_renderer, audio_config)) ==
        NULL) {
        LOGE("Could not init audio renderer");
        return -1;
    }

    if (video_renderer) video_renderer->funcs->start(video_renderer);
    if (audio_renderer) audio_renderer->funcs->start(audio_renderer);

    unsigned short port = 0;
    raop_start(raop, &port);
    raop_set_port(raop, port);

    int error;
    dnssd = dnssd_init(name.c_str(), strlen(name.c_str()), hw_addr.data(), hw_addr.size(), &error);
    if (error) {
        LOGE("Could not initialize dnssd library!");
        return -2;
    }

    raop_set_dnssd(raop, dnssd);

    dnssd_register_raop(dnssd, port);
    dnssd_register_airplay(dnssd, port + 1);

    return 0;
}

int stop_server() {
    raop_destroy(raop);
    dnssd_unregister_raop(dnssd);
    dnssd_unregister_airplay(dnssd);
    // If we don't destroy these two in the correct order, we get a deadlock from the ilclient library
    if (audio_renderer) audio_renderer->funcs->destroy(audio_renderer);
    if (video_renderer) video_renderer->funcs->destroy(video_renderer);
    logger_destroy(render_logger);
    return 0;
}

void HHAirPlaySetConnectedHandler(ConnectHandler handler)
{
    g_connectHandler = handler;
}

void HHAirPlaySetDisconnectedHandler(DisconnectHandler handler)
{
    g_disconnectHandler = handler;
}

void HHAirPlaySetVideoFrameHandler(VideoFrameHandler handler)
{
    g_videoFrameHandler = handler;
}

void HHAirPlaySetAudioFrameHandler(AudioFrameHandler handler)
{
    g_audioFrameHandler = handler;
}

int HHAirPlayStart(const char* deviceName)
{
    std::string server_name = DEFAULT_NAME;
    if (deviceName != nullptr && *deviceName != '\0')
    {
        server_name = deviceName;
    }
    
    std::vector<char> server_hw_addr = DEFAULT_HW_ADDRESS;
    bool debug_log = DEFAULT_DEBUG_LOG;

#ifdef _DEBUG
    debug_log = true;
#endif

    video_renderer_config_t video_config;
    video_config.background_mode = DEFAULT_BACKGROUND_MODE;
    video_config.low_latency = DEFAULT_LOW_LATENCY;
    video_config.rotation = DEFAULT_ROTATE;
    video_config.flip = DEFAULT_FLIP;

    audio_renderer_config_t audio_config;
    audio_config.device = DEFAULT_AUDIO_DEVICE;
    audio_config.low_latency = DEFAULT_LOW_LATENCY;

    // Default to the best available renderer
    video_init_func = video_renderers[0].init_func;
    audio_init_func = audio_renderers[0].init_func;

    std::string mac_address = find_mac();
    if (!mac_address.empty()) {
        server_hw_addr.clear();
        parse_hw_addr(mac_address, server_hw_addr);
    }

    if (start_server(server_hw_addr, server_name, debug_log, &video_config, &audio_config) != 0) {
        return -1;
    }

    return 0;
}

void HHAirPlayStop()
{
    stop_server();
}