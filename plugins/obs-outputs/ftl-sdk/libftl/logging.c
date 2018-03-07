/**
 * \file logging.c - Contains debug log functions
 *
 * Copyright (c) 2015 Michael Casadevall
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 **/

#include "ftl.h"
#include "ftl_private.h"

void ftl_log_msg(ftl_stream_configuration_private_t *ftl, ftl_log_severity_t log_level, const char * file, int lineno, const char * fmt, ...) {
  va_list args;
  ftl_status_msg_t m;
  m.type = FTL_STATUS_LOG;

  m.msg.log.log_level = log_level;
  va_start(args, fmt);
  vsnprintf(m.msg.log.string, sizeof(m.msg.log.string), fmt, args);
  va_end(args);

  enqueue_status_msg(ftl, &m);
}

