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
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h>

#include "ncurses.h"
#include "fftw3.h"

#include "graphics.h"
#include "audio.h"

#define PI (3.1415926535)
#define FFT_BUF_SIZE (1024)
#define MAX_DISPLAY_FREQ (10000.0)

double hann_window[FFT_BUF_SIZE];

uint complex_abs(double a, double b) {
    return (uint) sqrt(pow(a, 2) + pow(b, 2));
}

uint num_display_cols() {
    int n = FFT_BUF_SIZE / 2;
    // COLS is ncurses global
    return (n < COLS) ? n : COLS;
}

double get_target_frequency(uint column) {
    return (column * MAX_DISPLAY_FREQ / (num_display_cols() - 1));
}

double get_fft_bin_frequency(uint bin, uint sample_rate) {
    double fft_bin_resolution =  sample_rate / (double) FFT_BUF_SIZE;
    return (bin * fft_bin_resolution);
}


void transform_fft_output_for_display(fftw_complex *out, uint sample_rate,
                                    int *display_values, uint columns, uint rows) {
    uint fft_index = 0;
    for (int col = 0; col < columns; col++) {
        while (get_fft_bin_frequency(fft_index, sample_rate) < get_target_frequency(col)) {
            fft_index++;
        }
        uint val = complex_abs(out[fft_index][0], out[fft_index][1]);
        val = val * (rows / 60.0);
        if (val > rows) {
            val = rows;
        }
        display_values[col] = val;
    }
}

void draw_data(int *display_values, uint cols, uint rows) {
    clear();
    for (int col = 0; col < cols; col++) {
        int row = rows - display_values[col];
        for (int j = rows; j >= row; j--) {
            mvaddch(j, col, '^');
        }
    }
    refresh();
}

void display_periodic_output(float *file_buffer, uint64_t *file_offset,
                            uint64_t file_buf_size, uint sample_rate,
                            uint channels) {
    double *in;
    fftw_complex *out;
    fftw_plan p;
    uint display_values[FFT_BUF_SIZE / 2];

    in = (double*) fftw_malloc(sizeof(double) * FFT_BUF_SIZE);
    out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * FFT_BUF_SIZE);
    p = fftw_plan_dft_r2c_1d(FFT_BUF_SIZE, in, out, FFTW_ESTIMATE);

    while(*file_offset == 0);

     while(*file_offset < file_buf_size) {
        for(int i = 0; i < FFT_BUF_SIZE; i++) {
            in[i] = hann_window[i] * (double) file_buffer[*file_offset + channels * i];
        }
        fftw_execute(p);
        uint cols = num_display_cols();
        uint rows = LINES - 1; // LINES is ncurses global
        transform_fft_output_for_display(out, sample_rate, display_values,
                                        cols, rows);
        draw_data(display_values, cols, rows);

        usleep(100 * 1000);
    }

    fftw_free(in);
    fftw_free(out);
}

void screen_setup(void) {
    initscr();
    noecho();
    curs_set(0);
}

void screen_cleanup(void) {
    endwin();
}

void compute_window_function(void) {
    for (int i = 0; i < FFT_BUF_SIZE; i++) {
        double val = 2 * PI * i / (FFT_BUF_SIZE - 1);
        hann_window[i] = 0.5 * (1 - cos(val));
    }
}

void graphics_entrypoint(void *args) {
    screen_setup();
    compute_window_function();

    struct global_info *graphics_args = (struct global_info *) args;
    display_periodic_output(graphics_args->file_buffer,
                        graphics_args->file_offset,
                        graphics_args->file_buf_size,
                        graphics_args->sample_rate,
                        graphics_args->channels);

    screen_cleanup();
}
