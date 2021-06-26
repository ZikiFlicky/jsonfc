/* Copyright (c) 2021, Yuval Tasher (ziki.flicky@gmail.com)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "types.h"
#include "filereader.h"

#include <assert.h>
#include <stdio.h>

static bool filereader_load_buffer(struct FileReader *fr);

bool filereader_stream_exists(struct FileReader *fr) {
    if (fr->stream == NULL)
        return false;

    return true;
}

/*
  returns the current index relative to ftell.
  used to check if something advanced
 */
size_t filereader_current_idx(struct FileReader *fr) {
    if (!filereader_stream_exists(fr))
        assert(false); /* TODO */

    return ftell(fr->stream) - BUFFER_SIZE + fr->buff_idx;
}

/*
  opens the stream on *fr, loads the buffer and returns a boolean specifying
  if opening the buffer was successful
 */
bool filereader_open_stream(struct FileReader *fr, const char *filename) {
    fr->stream = fopen(filename, "r");

    /* failed to open? */
    if (!filereader_stream_exists(fr))
        return false;

    /* load the buffer once, so we can start working with the data */
    filereader_load_buffer(fr);

    return true;
}

/*
  fills the buffer with 0's, moves the buffer index to 0 and
  the loaded counter to 0
 */
static void filereader_reset_buffer(struct FileReader *fr) {
    size_t i;

    for (i = 0; i < BUFFER_SIZE+1; ++i)
        fr->buffer[i] = '\0';

    fr->buff_idx = 0;
}

/*
  a helper for loading a new buffer.
  returns true if it succeeded loading a new buffer
 */
static bool filereader_load_buffer(struct FileReader *fr) {
    if (!filereader_stream_exists(fr))
        return false;

    if (feof(fr->stream))
        return false;

    filereader_reset_buffer(fr);
    fread(fr->buffer, BUFFER_SIZE, 1, fr->stream);

    return true;
}

/*
  closes the stream and returns a boolean saying if it was
  successful.
 */
bool filereader_close_stream(struct FileReader *fr) {
    if (!filereader_stream_exists(fr))
        return false;

    if (fclose(fr->stream) != 0)
        return false;

    filereader_reset_buffer(fr);

    return true;
}

/*
  returns a boolean value indicating if you had reached a \0 on file and on buffer,
  meaning that you can't advance anymore, not even one
 */
bool filereader_readable(struct FileReader *fr) {
    if (!filereader_stream_exists(fr))
        return false;

    if (FR_CURRENT_CHAR(*fr) == '\0' && feof(fr->stream))
        return false;

    return true;
}

/*
  advance steps times. i.e move the file index forward `steps` times
  will return false on fail and true on success
 */
bool filereader_advance(struct FileReader *fr, size_t steps) {
    size_t start;

    if (!filereader_stream_exists(fr))
        return false;

    start = filereader_current_idx(fr);

    while (steps --> 0) {
        if (!filereader_readable(fr)) {
            filereader_set_idx(fr, start);
            return false;
        }

        ++fr->buff_idx;

        if (FR_CURRENT_CHAR(*fr) == '\0')
            filereader_load_buffer(fr);
    }

    return true;
}

/*
  given a FileReader and a relative index, this function should tell you
  what character is present in a certain index.
  will return \0 at fail, or at end of reading.
 */
char filereader_char_at(struct FileReader *fr, size_t rel_idx) {
    size_t start;
    char c;

    /* validate that there is fr->stream */
    if (!filereader_stream_exists(fr))
        return '\0';

    /* set the place to return to */
    start = filereader_current_idx(fr);

    if (filereader_advance(fr, rel_idx))
        c = FR_CURRENT_CHAR(*fr);
    else
        c = '\0';

    /* go back to original place and load it into the buffer */
    filereader_set_idx(fr, start);

    /* finally, return the character */
    return c;
}

/*
  FIXME: check failure
 */
bool filereader_set_idx(struct FileReader *fr, size_t idx) {
    if (!filereader_stream_exists(fr))
        return false;

    /* move to the new location and load the buffer */
    fseek(fr->stream, idx, SEEK_SET);
    filereader_load_buffer(fr);

    return true;
}

