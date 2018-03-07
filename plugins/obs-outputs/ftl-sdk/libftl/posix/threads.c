/**
* \file threads.c - Posix Threads Abstractions
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

#include "threads.h"

pthread_mutexattr_t ftl_default_mutexattr;

int os_create_thread(OS_THREAD_HANDLE *handle, OS_THREAD_ATTRIBS *attibs, OS_THREAD_START_ROUTINE func, void *args) {

  return pthread_create(handle, NULL, func, args);
}

int os_destroy_thread(OS_THREAD_HANDLE handle) {
  return 0;
}

int os_wait_thread(OS_THREAD_HANDLE handle) {
  return pthread_join(handle, NULL);
}

int os_init() {
    pthread_mutexattr_init(&ftl_default_mutexattr);
    // Set pthread mutexes to recursive to mirror Windows mutex behavior
    return pthread_mutexattr_settype(&ftl_default_mutexattr, PTHREAD_MUTEX_RECURSIVE);
}

int os_init_mutex(OS_MUTEX *mutex) {
  return pthread_mutex_init(mutex, &ftl_default_mutexattr);
}

int os_lock_mutex(OS_MUTEX *mutex) {
  return pthread_mutex_lock(mutex);
}

int os_trylock_mutex(OS_MUTEX *mutex) {
  int ret = pthread_mutex_trylock(mutex);
  return (ret == 0) ? 1 : 0;
}

int os_unlock_mutex(OS_MUTEX *mutex) {
  return pthread_mutex_unlock(mutex);
}

int os_delete_mutex(OS_MUTEX *mutex) {
  return 0;
}

int os_semaphore_create(OS_SEMAPHORE *sem, const char *name, int oflag, unsigned int value, BOOL is_global) {

  int retval = 0;

  sem->name = NULL;
  sem->sem = NULL;

  do {
    if (name == NULL || name[0] != '/') {
      retval = -1;
      break;
    }

    //if the semaphore is intended to only be used by the same process and not across processes, give it unique name
    if(!is_global) {
      int name_len = strlen(name);

      if ((sem->name = (char*)malloc( (name_len + 20) * sizeof(char))) == NULL) {
        retval = -2;
        break;
      }

      sprintf(sem->name, "%s_%d", name, (unsigned int)rand());
    }
    else {
      if ((sem->name = strdup(name)) == NULL) {
        return -2;
      }
    }

#ifdef __ANDROID__
    if ((sem->sem = (sem_t*)malloc(sizeof(sem_t))) == NULL) {
      retval = -4;
      break;
    }

    if (sem_init(sem->sem, 0 /* pshared */, 0 /* value */)) {
#else
    if ((sem->sem = sem_open(sem->name, oflag, 0644, value)) == SEM_FAILED) {
#endif
      retval = -3;
      break;
    }

    return retval;
  }while(0);

  if(sem->name != NULL){
    free(sem->name);
  }

  if(sem->sem != NULL){
    free(sem->sem);
  }

  return retval;
}

int os_semaphore_pend(OS_SEMAPHORE *sem, int ms_timeout) {

  if (ms_timeout < 0) {
    return sem_wait(sem->sem);
  }
  else {
#ifdef __APPLE__
    int sleep_interval = 50;
    int retval;
    //TODO find a better solution
    /*OSX doesnt have a timedwait so this is an ugly polling solution since this SDK doesnt currently use timedwait for performance critical things*/
    while (ms_timeout > 0) {
      if ((retval = sem_trywait(sem->sem)) == 0) {
        break;
      }
      sleep_ms(sleep_interval);
      ms_timeout -= sleep_interval;
    }

    return retval;
#else
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    timespec_add_ms(&ts, ms_timeout);
    return sem_timedwait(sem->sem, &ts);

#endif
  }
}

int os_semaphore_post(OS_SEMAPHORE *sem) {
  return sem_post(sem->sem);
}

int os_semaphore_delete(OS_SEMAPHORE *sem) {
  
  int retval = 0;

#ifdef __ANDROID__
  retval = sem_destroy(sem->sem);
  free(sem->sem);

#else
  if ( (retval = sem_close(sem->sem)) == 0) {
    
    retval = sem_unlink(sem->name);
  }
#endif

  free(sem->name);

  return retval;
}

void sleep_ms(int ms)
{
    usleep(ms * 1000);
}


