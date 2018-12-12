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

#include <pthread.h>
#include "audio.h"

int set_alsa_hw_params(snd_pcm_t *handle, snd_pcm_uframes_t *buffer_size,
                        uint *sample_rate) {
    int dir;
    int err;
    snd_pcm_uframes_t frames = 256;
    snd_pcm_hw_params_t *params;

    if ((err = snd_pcm_hw_params_malloc (&params)) < 0) {
        fprintf (stderr, "cannot allocate hardware parameter structure (%s)\n", snd_strerror(err));
        return -1;
    }
    /* Fill it in with default values. */
    err = snd_pcm_hw_params_any(handle, params);
    if (err < 0) {
        fprintf(stderr, "unable to set hw params any: %s\n", snd_strerror(err));
        return -1;
    }

    err = snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (err < 0) {
        fprintf(stderr, "unable to set access: %s\n", snd_strerror(err));
        return -1;
    }
    err = snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_FLOAT_LE);
    if (err < 0) {
        fprintf(stderr, "unable to set format: %s\n", snd_strerror(err));
        return -1;
    }
    err = snd_pcm_hw_params_set_channels(handle, params, 2);
    if (err < 0) {
        fprintf(stderr, "unable to set channels: %s\n", snd_strerror(err));
        return -1;
    }
    err = snd_pcm_hw_params_set_rate_near(handle, params, sample_rate, &dir);
    if (err < 0) {
        fprintf(stderr, "unable to set rate: %s\n", snd_strerror(err));
        return -1;
    }
    err = snd_pcm_hw_params_set_period_size_near(handle, params, &frames, &dir);
    if (err < 0) {
        fprintf(stderr, "unable to set period size: %s\n", snd_strerror(err));
        return -1;
    }
    err = snd_pcm_hw_params_set_buffer_size_near(handle, params, buffer_size);
    if (err < 0) {
        fprintf(stderr, "unable to set buffer size: %s\n", snd_strerror(err));
        return -1;
    }
    err = snd_pcm_hw_params(handle, params);
    if (err < 0) {
        fprintf(stderr, "unable to set hw parameters: %s\n", snd_strerror(err));
        return -1;
    }

    snd_pcm_hw_params_free(params);

    return 0;
}

int play_sound(snd_pcm_t *handle,
                uint alsa_buf_size,
                float *audio_buffer,
                uint64_t file_buf_size,
                uint64_t *file_offset) {
    float *index = audio_buffer;
    while(*file_offset < file_buf_size) {
        if (snd_pcm_writei(handle, index, alsa_buf_size) < 0) {
            snd_pcm_prepare(handle);
            //fprintf(stderr, "buffer underrun\n");
        }
        *file_offset += 2 * alsa_buf_size;
        index += 2 * alsa_buf_size;
    }
    return 0;
}

void *audio_entrypoint(void *args) {
    struct global_info *audio_args = (struct global_info *) args;
    play_sound(audio_args->alsa_handle,
                audio_args->alsa_buf_size,
                audio_args->file_buffer,
                audio_args->file_buf_size,
                audio_args->file_offset);
    pthread_exit(NULL);
}
