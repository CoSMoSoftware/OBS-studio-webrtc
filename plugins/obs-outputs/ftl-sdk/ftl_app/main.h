/**
 * charon.h - Global Header for the Charon client
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

/* Include definitions */
#include "ftl.h"

/* Windows specific includes */
#ifdef _WIN32

/**
 * We have to define LEAN_AND_MEAN because winsock will pull in duplicate
 * header definitions without it as the winsock 1.1 API is in windows.h vs
 * ws2_32.h.
 *
 * Yay for backwards source compatability
 **/

#define WIN32_LEAN_AND_MEAN 1
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "win32/xgetopt.h"
#else
/* POSIX headers */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#endif 

/***
 * Functions related to Ctrl-C handling
 **/

void charon_install_ctrlc_handler();
void charon_loop_until_ctrlc();
int ctrlc_pressed();
