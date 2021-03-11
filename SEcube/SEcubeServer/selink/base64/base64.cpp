/**
  ******************************************************************************
  * File Name          : base64.cpp
  * Description        : Interface between SElink and libb64.
  ******************************************************************************
  *
  * Copyright © 2016-present Blu5 Group <https://www.blu5group.com>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of the GNU Lesser General Public
  * License as published by the Free Software Foundation; either
  * version 3 of the License, or (at your option) any later version.
  *
  * This library is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  * Lesser General Public License for more details.
  *
  * You should have received a copy of the GNU Lesser General Public
  * License along with this library; if not, see <https://www.gnu.org/licenses/>.
  *
  ******************************************************************************
  */

/* This code is not part of the original libb64 code. It has been created
 * to so that SElink can use wrappers around the functions of libb64. */

extern "C"{
#include "cdecode.h"
#include "cencode.h"
};

size_t decode_b64(const char* input, size_t input_len, char* output)
{
    base64_decodestate s;
    size_t cnt;
    base64_init_decodestate(&s);
    cnt = base64_decode_block(input, input_len, output, &s);
    output[cnt] = 0;
    return cnt;
}

size_t encode_b64(const char* input, size_t input_len, char* output)
{
    base64_encodestate s;
    size_t cnt;
    base64_init_encodestate(&s);
    s.chars_per_line = 72;
    cnt = base64_encode_block(input, input_len, output, &s);
    cnt += base64_encode_blockend(output + cnt, &s);
    output[cnt] = 0;
    return cnt;
}
