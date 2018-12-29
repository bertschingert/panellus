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

#include <stdio.h>
#include <pthread.h>

#include "sndfile.h"
#include "alsa/asoundlib.h"

#include "audio.h"
#include "graphics.h"

#define DFT_FRAMES_PER_BUFFER (256)

/*
 * Updates file_buffer_size with the size of the allocated file buffer
 */
float* load_sound_file(char *infile_name, SF_INFO *sfinfo, uint64_t *file_buffer_size) {
    SNDFILE *infile;

    if (! (infile = sf_open(infile_name, SFM_READ, sfinfo))) {
        puts(sf_strerror (NULL));
        exit(1);
    }

    *file_buffer_size = sfinfo->channels * sfinfo->frames;
    float *file_buffer = (float*) malloc(*file_buffer_size * sizeof(float));
    sf_readf_float(infile, file_buffer, sfinfo->frames);

    sf_close(infile);
    return file_buffer;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("enter filename as first argument\n");
        return -1;
    }

    SF_INFO sfinfo;
    uint64_t file_buffer_size;
    float *file_buffer = load_sound_file(argv[1], &sfinfo, &file_buffer_size);

    int err;
    const char *device = "default";
    snd_pcm_t *handle;

    if ((err = snd_pcm_open(&handle, device, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        fprintf(stderr, "cannot open audio device %s (%s)\n", device, snd_strerror(err));
        return -1;
    }

    uint sample_rate = sfinfo.samplerate;
    uint channels = sfinfo.channels;
    snd_pcm_uframes_t alsa_buffer_size = DFT_FRAMES_PER_BUFFER;
    if (set_alsa_hw_params(handle, &alsa_buffer_size, channels, &sample_rate) != 0) {
        return -1;
    }

    uint64_t file_offset = 0;

    struct global_info global_info;
    global_info.alsa_handle = handle;
    global_info.alsa_buf_size = alsa_buffer_size;
    global_info.sample_rate = sample_rate;
    global_info.channels = channels;
    global_info.file_buffer = file_buffer;
    global_info.file_buf_size = file_buffer_size;
    global_info.file_offset = &file_offset;

    pthread_t audio_thread;
    pthread_create(&audio_thread, NULL, audio_entrypoint, &global_info);

    graphics_entrypoint(&global_info);

    void *status;
    pthread_join(audio_thread, &status);

    snd_pcm_drain(handle);
    snd_pcm_close(handle);
    free(file_buffer);
    return 0;
}
