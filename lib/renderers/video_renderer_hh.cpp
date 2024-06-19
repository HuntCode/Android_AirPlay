/**
 *  Created by WangLv on 2024/04/26
 */

#include "video_renderer.h"

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#ifdef WIN32
#include <inttypes.h>
#include <io.h>
#else
#include <unistd.h>
#endif

#include "../common/common_defines.h"

ConnectHandler g_connectHandler = NULL;
DisconnectHandler g_disconnectHandler = NULL;
VideoFrameHandler g_videoFrameHandler = NULL;

typedef struct video_renderer_hh_s {
    video_renderer_t base;
} video_renderer_hh_t;

 extern  const video_renderer_funcs_t video_renderer_hh_funcs;

video_renderer_t *video_renderer_hh_init(logger_t *logger, video_renderer_config_t const *config) {
    video_renderer_hh_t *renderer;
    renderer = (video_renderer_hh_t*)calloc(1, sizeof(video_renderer_hh_t));
    if (!renderer) {
        return NULL;
    }
    renderer->base.logger = logger;
    renderer->base.funcs = &video_renderer_hh_funcs;
    renderer->base.type = VIDEO_RENDERER_HH;
    return &renderer->base;
}

static void video_renderer_hh_start(video_renderer_t *renderer) {
}

static void video_renderer_hh_render_buffer(video_renderer_t *renderer, raop_ntp_t *ntp, unsigned char *data, int data_len, uint64_t pts, int type, unsigned int streamId)
{
    if (g_videoFrameHandler)
        g_videoFrameHandler(streamId, data, data_len);
}

static void video_renderer_hh_flush(video_renderer_t *renderer) {
}

static void video_renderer_hh_destroy(video_renderer_t *renderer)
{
    if (renderer) {
        free(renderer);
    }
}

static void video_renderer_hh_update_background(video_renderer_t *renderer, int type, unsigned int streamId)
{
    if (type == 1)
    {
        if(g_connectHandler)
            g_connectHandler(streamId);
    }
    else
    {
        if(g_disconnectHandler)
            g_disconnectHandler(streamId);
    }
}

const video_renderer_funcs_t video_renderer_hh_funcs = {
    video_renderer_hh_start,
    video_renderer_hh_render_buffer,
    video_renderer_hh_flush,
    video_renderer_hh_destroy,
    video_renderer_hh_update_background,
};
