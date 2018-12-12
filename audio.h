/*
Copyright (C) 2018 Thomas Bertschinger

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <stdint.h>
#include "alsa/asoundlib.h"

struct global_info {
    snd_pcm_t *alsa_handle;
    uint alsa_buf_size;
    uint sample_rate;
    float *file_buffer;
    uint64_t file_buf_size;
    uint64_t *file_offset;  /* keeps track of playback position */
};

int set_alsa_hw_params(snd_pcm_t *handle, snd_pcm_uframes_t *buffer_size, uint *sample_rate);
void *audio_entrypoint(void *args);
