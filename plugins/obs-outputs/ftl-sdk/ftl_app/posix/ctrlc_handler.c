/**
* ctrlc_handler.cpp - POSIX handler for Ctrl-C behavior
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

#include "main.h"

/**
 * On POSIX platforms, we need to catch SIGINT, and and change the state
 * of the shutdown flag. When we're in a signal handler, we've very limited
 * in the type of calls we can safely make due to the state of the stack
 * and potentially being anywhere in the calling program's execution. 
 *
 * As such, the "safest" thing to do is simply set a global variable which
 * breaks us out of the loop, and brings us to our shutdown code. 
 */

volatile sig_atomic_t shutdown_flag = 0;

void charon_shutdown_stream(int sig) {
  shutdown_flag = 1;
}

void charon_install_ctrlc_handler() {
  signal(SIGINT, charon_shutdown_stream);
}

void charon_loop_until_ctrlc() {
  while (shutdown_flag != 1) {
    sleep(1);
  }
}

int ctrlc_pressed() {
  return shutdown_flag;
}
