#include <assert.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <math.h>

#include "queue.h"
#include "thread.h"
#include "tps.h"

// make it easy to look up a particular thread's tps data with a table
#define MAX_THREADS 256

struct tps {
  void* data;
};

struct tps tps_table[MAX_THREADS];

static pthread_t get_tid()
{
  // ensure that we don't accidentally index too far into the tps table
  pthread_t tid = pthread_self();
  if (tid >= MAX_THREADS) {
    fprintf(stderr, "tps library only supports a maximum of %d threads\n",
        MAX_THREADS);
    fprintf(stderr, "current thread id: %ld\n", tid);
    abort();
  }

  return tid;
}

// return 0 if the tid has a tps
static int tid_has_tps(pthread_t tid) {
  // data == NULL if the tps for thread tid doesn't exist
  if (tps_table[tid].data == NULL) {
    return -1;
  }

  return 0;
}

// returns 0 if initial checks on this io operation are valid, -1 otherwise
static int tps_io_check(size_t offset, size_t length, char* buffer) {
  pthread_t tid = get_tid();

  // ensure the thread even has a tps
  int ret = tid_has_tps(tid);
  if (ret == 0) {
    return -1;
  }

  // check whether the read operation within bounds
  if (offset + length >= TPS_SIZE) {
    return -1;
  }

  // can't read or write from a null buffer
  if (buffer == NULL) {
    return -1;
  }

  return 0;
}

// helper function to change protection on a particular tps. Returns -1 if there
// was an error, 0 if success.
static int tps_mmap_set_prot(pthread_t tid, int prot) {
  // calling mmap on the same region can change its protectrion
  int flags = MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS;
  void* ret = mmap(tps_table[tid].data, TPS_SIZE, prot, flags, 0, 0);
  if (ret == MAP_FAILED) {
    return -1;
  }

  tps_table[tid].data = ret;
  return 0;
}

static int tps_enable_read(pthread_t tid) {
  return tps_mmap_set_prot(tid, PROT_READ);
}

static int tps_enable_write(pthread_t tid) {
  return tps_mmap_set_prot(tid, PROT_WRITE);
}

static int tps_disable_read_write(pthread_t tid) {
  return tps_mmap_set_prot(tid, PROT_NONE);
}

int tps_init(int segv)
{
  // protect from reinitialization
  static int initialized = 0;
  if (initialized != 0) {
    return -1;
  }
  initialized = 1;

  for (int i = 0; i < MAX_THREADS; i++) {
    tps_table[i].data = NULL;
  }

  if (segv == 0) {
   // user didn't ask for segv handling, nothing more to do here
    return 0;
  }

  return 0;
}

int tps_create(void)
{
  pthread_t tid = get_tid();

  // data != NULL if the tps for this thread already exists
  if (tps_table[tid].data != NULL) {
    return -1;
  }

  // MAP_ANONYMOUS: This flag tells the system to create an anonymous mapping,
  // not connected to a file. filedes and offset are ignored, and the region is
  // initialized with zeros.
  //
  // MAP_PRIVATE: a copy is made for th eprocess and no other process will see
  // the changes.
  //
  // mmap already fills the data region with zeros so no need to do that.
  void* data = mmap(NULL, TPS_SIZE, PROT_NONE, MAP_PRIVATE|MAP_ANONYMOUS, 0, 0);
  if (data == MAP_FAILED) {
    return -1;
  }

  tps_table[tid].data = data;
  return 0;
}

int tps_destroy(void)
{
  pthread_t tid = get_tid();

  // can't destroy a tps that doesn't exist
  int ret = tid_has_tps(tid);
  if (ret == 0) {
    return -1;
  }

  // munmap is the mmap equivalent of free
  ret = munmap(tps_table[tid].data, TPS_SIZE);
  if (ret == -1) {
    return -1;
  }

  tps_table[tid].data = NULL;
  return 0;
}

int tps_read(size_t offset, size_t length, char *buffer)
{
  int ret = tps_io_check(offset, length, buffer);
  if (ret == -1) {
    return -1;
  }

  pthread_t tid = get_tid();
  tps_enable_read(tid);
  memcpy(buffer, tps_table[tid].data + offset, length);
  tps_disable_read_write(tid);
  return 0;
}

int tps_write(size_t offset, size_t length, char *buffer)
{
  int ret = tps_io_check(offset, length, buffer);
  if (ret == -1) {
    return -1;
  }

  pthread_t tid = get_tid();
  tps_enable_write(tid);
  memcpy(tps_table[tid].data + offset, buffer, length);
  tps_disable_read_write(tid);
  return 0;
}

int tps_clone(pthread_t tid)
{
  // don't want to read outside of tps_table
  if (tid >= MAX_THREADS) {
    return -1;
  }

  // ensure that the target thread even has a tps
  int ret = tid_has_tps(tid);
  if (ret == -1) {
    return -1;
  }

  // cannot overwrite our tps if we already have one
  pthread_t self = get_tid();
  ret = tid_has_tps(self);
  if (ret == 0) {
    return -1;
  }

  // enable reads from the target thread's TPS
  ret = tps_enable_read(tid);
  if (ret == -1) {
    return -1;
  }

  // create new memory region for current thread's tps
  void* data = mmap(NULL, TPS_SIZE, PROT_NONE, MAP_PRIVATE|MAP_ANONYMOUS, 0, 0);
  if (data == MAP_FAILED) {
    ret = tps_disable_read_write(tid);
    if (ret == -1) {
      fprintf(stderr, "failed disabling read/write on target thread's tps\n");
      fprintf(stderr, "library reached unrecoverable error\n");
      abort();
    }

    return -1;
  }

  // copy the data over
  tps_table[self].data = data;
  memcpy(tps_table[self].data, tps_table[tid].data, TPS_SIZE);

  // disable reads and writes from the target thread's TPS
  ret = tps_disable_read_write(tid);
  if (ret == -1) {
    return -1;
  }

  return 0;
}
