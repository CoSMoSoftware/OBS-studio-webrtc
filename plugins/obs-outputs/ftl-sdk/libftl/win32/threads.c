/**
* \file threads.c - Windows Threads Abstractions
*
* Copyright (c) 2015 Stefan Slivinski
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

#include <stdlib.h>
#include "threads.h"

int os_init(){
    return 0;
}

int os_create_thread(OS_THREAD_HANDLE *handle, OS_THREAD_ATTRIBS *attibs, OS_THREAD_START_ROUTINE func, void *args) {
  HANDLE thread;
  thread = CreateThread(NULL, 0, func, args, 0, NULL);

  if (thread == NULL) {
    return -1;
  }

  *handle = thread;

  return 0;
}

int os_destroy_thread(OS_THREAD_HANDLE handle) {
  CloseHandle(handle);

  return 0;
}

int os_wait_thread(OS_THREAD_HANDLE handle) {
  WaitForSingleObject(handle, INFINITE);

  return 0;
}

int os_init_mutex(OS_MUTEX *mutex) {

  InitializeCriticalSection(mutex);

  return 0;
}

int os_lock_mutex(OS_MUTEX *mutex) {

  EnterCriticalSection(mutex);

  return 0;
}

int os_trylock_mutex(OS_MUTEX *mutex) {

  return TryEnterCriticalSection(mutex);
}

int os_unlock_mutex(OS_MUTEX *mutex) {

  LeaveCriticalSection(mutex);

  return 0;
}

int os_delete_mutex(OS_MUTEX *mutex) {

  DeleteCriticalSection(mutex);

  return 0;
}

int os_semaphore_create(OS_SEMAPHORE *sem, const char *name, int oflag, unsigned int value, BOOL is_global) {

  char *internal_name = NULL;
  int retval = 0;

  do {
    if (name == NULL) {
      retval = -1;
      break;
    }

    //if the semaphore is intended to only be used by the same process and not across processes, give it unique name
    if(!is_global) {
      size_t name_len = strlen(name);
      size_t max_name = name_len + 20;

      if ((internal_name = (char*)malloc(max_name * sizeof(char))) == NULL) {
        retval = -2;
        break;
      }

      sprintf_s(internal_name, max_name, "%s_%d", name, (unsigned int)rand());
    }
    else {
      if ((internal_name = _strdup(name)) == NULL) {
        retval = -2;
        break;
      }
    }  

    if ( (*sem = CreateSemaphoreA(NULL, value, MAX_SEM_COUNT, (LPCSTR)internal_name)) == NULL){
      retval = -3;
      break;
    }
  }while(0);

  if(internal_name != NULL){
    free(internal_name);
  }

  return retval;
}

int os_semaphore_pend(OS_SEMAPHORE *sem, int ms_timeout) {

  if (WaitForSingleObject(*sem, ms_timeout) != WAIT_OBJECT_0) {
    return -1;
  }

  return 0;
}

int os_semaphore_post(OS_SEMAPHORE *sem) {
  if (ReleaseSemaphore(*sem, 1, NULL)) {
    return 0;
  }

  return -1;
}

int os_semaphore_delete(OS_SEMAPHORE *sem) {
  if (CloseHandle(*sem)) {
    return 0;
  }

  return -1;
}

void sleep_ms(int ms)
{
        Sleep(ms);
}


