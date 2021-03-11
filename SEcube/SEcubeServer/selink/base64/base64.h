/**
  ******************************************************************************
  * File Name          : base64.h
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

/* This header file is not part of the original libb64 code. It has been created
 * to so that SElink can use wrappers around the functions of libb64. */

#ifndef SELINK_BASE64_BASE64_H_
#define SELINK_BASE64_BASE64_H_

size_t decode_b64(const char* input, size_t input_len, char* output);
size_t encode_b64(const char* input, size_t input_len, char* output);

#endif /* SELINK_BASE64_BASE64_H_ */
