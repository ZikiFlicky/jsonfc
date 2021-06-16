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

static bool filereader_load_buffer(struct FileReader *fr);

bool filereader_open_stream(struct FileReader *fr, const char *filename) {
    fr->stream = fopen(filename, "r");
    /* failed to to open? */
    if (fr->stream == NULL)
        return false;
    /* FIXME: not sure this is the best approach */
    /* load the buffer once, so we can start working with the data */
    filereader_load_buffer(fr);
    return true;
}

static void filereader_reset_buffer(struct FileReader *fr) {
    size_t i;
    for (i = 0; i < BUFFER_SIZE+1; ++i)
        fr->buffer[i] = '\0';
}

static bool filereader_load_buffer(struct FileReader *fr) {
    if (feof(fr->stream))
        return false;
    fr->buff_idx = 0;
    filereader_reset_buffer(fr);
    fread(fr->buffer, 1, BUFFER_SIZE, fr->stream);
    return true;
}

bool filereader_can_advance(struct FileReader *fr) {
    if (FR_CURRENT_CHAR(*fr) == '\0' && feof(fr->stream))
        return false;
    return true;
}

bool filereader_advance(struct FileReader *fr) {
   /* move forward once */
    ++fr->buff_idx;
   /* is end of buffer? */
    if (FR_CURRENT_CHAR(*fr) == '\0') {
        /* can you load into the buffer? */
        if (filereader_load_buffer(fr))
            return true;
        else
            return false;
    }
    return true;
}

