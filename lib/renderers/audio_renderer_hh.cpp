/**
 *  Created by WangLv on 2024/04/26
 */

#include "audio_renderer.h"

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

AudioFrameHandler g_audioFrameHandler = NULL;

typedef struct audio_renderer_hh_s {
    audio_renderer_t base;
} audio_renderer_hh_t;

extern const audio_renderer_funcs_t audio_renderer_hh_funcs;

audio_renderer_t *audio_renderer_hh_init(logger_t *logger, video_renderer_t *video_renderer, audio_renderer_config_t const *config) {
    audio_renderer_hh_t *renderer;
    renderer = (audio_renderer_hh_t*)calloc(1, sizeof(audio_renderer_hh_t));
    if (!renderer) {
        return NULL;
    }
    renderer->base.logger = logger;
    renderer->base.funcs = &audio_renderer_hh_funcs;
    renderer->base.type = AUDIO_RENDERER_HH;
    return &renderer->base;
}

static void audio_renderer_hh_start(audio_renderer_t *renderer) {
}

static void audio_renderer_hh_render_buffer(audio_renderer_t *renderer, raop_ntp_t *ntp, unsigned char *data, int data_len, uint64_t pts, unsigned int streamId)
{
    if (g_audioFrameHandler)
        g_audioFrameHandler(streamId, data, data_len);
}

static void audio_renderer_hh_set_volume(audio_renderer_t *renderer, float volume) {
}

static void audio_renderer_hh_flush(audio_renderer_t *renderer) {
}

static void audio_renderer_hh_destroy(audio_renderer_t *renderer) {
    if (renderer) {
        free(renderer);
    }
}

const audio_renderer_funcs_t audio_renderer_hh_funcs = {
    audio_renderer_hh_start,
    audio_renderer_hh_render_buffer,
    audio_renderer_hh_set_volume,
    audio_renderer_hh_flush,
    audio_renderer_hh_destroy,
};
