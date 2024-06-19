/*
 * MIT License
 *
 * Copyright (c) 2019 dsafa22, All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


#include <cstring>
#include <signal.h>
#include <string>
#include <vector>
#include <fstream>
#include <stddef.h>
#include "log.h"
#include <malloc.h>
#include <cstring>
#include <math.h>
#include "raop_server.h"
#include <windows.h>

extern "C" void
audio_process(void *cls, void *opaque, pcm_data_struct *data)
{
    audio_session_t *session = (audio_session_t *)opaque;
    for (int i = 0; i < data->data_len; i++) {
        data->data[i] = data->data[i] * session->volume;
    }   
}

extern "C" void
video_process(void *cls, h264_decode_struct *data)
{
    
}

static bool is_running = false;
void SignalHandler(int signal)
{
	switch (signal)
	{
	case SIGABRT:
	case SIGTERM:
		is_running = false;
		break;
	}
}

int main(int argc, char* argv[]) {
	typedef void (*SignalHandlerPointer)(int);
	SignalHandlerPointer previousHandler;
	char hw_addr[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 };
	int hw_addr_len = sizeof(hw_addr);
	raop_server_t* raop_server = raop_server_init(NULL, audio_process, video_process);
	
	if (raop_server_start(raop_server, "win32", (char*)hw_addr, hw_addr_len) != 0) {
		return 1;
	}
	LOGI("starting...");
	is_running = true;
	while (is_running) {
		Sleep(100);
	}
	raop_server_destroy(raop_server);
	return 0;
}