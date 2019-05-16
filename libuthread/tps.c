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

struct tps
{
  pthread_t owner_tid;
  void* data;
};

// overload the usage of a queue for storing our tps list
queue_t tps_list = NULL;

// HELPER FUNCTIONS ------------------------------------------------------------

// callback function that finds a certain item according to its owner thread id
static int find_item(void *data, void *arg)
{
  struct tps* current_tps = (struct tps*) data;
  pthread_t* target_tid = (pthread_t*) arg;

  if (current_tps->owner_tid == *target_tid) {
    return 1;
  }

  return 0;
}

// callback function to find a tps that contains the given address
static int addr_in_tps(void *data, void *arg) {
  struct tps* current_tps = (struct tps*) data;
  void* addr = arg;

  // get difference in addresses from start of current_tps's region and where
  // addr sits in memory. If that is less than TPS_SIZE, we are within the tps
  // range
  if (addr - current_tps->data <= TPS_SIZE) {
    return 1;
  }

  return 0;
}

// returns a pointer to the tps found, NULL if not found
static struct tps* get_tps(pthread_t tid)
{
  struct tps* tps = NULL;
  int ret = queue_iterate(tps_list, find_item, &tid, (void**)&tps);
  if (ret == -1) {
    return NULL;
  }

  if (tps == NULL) {
    return NULL;
  }

  return tps;
}

// return 1 if the tid has a tps, 0 if not
static int has_tps(pthread_t tid)
{
  struct tps* tps = get_tps(tid);
  if (tps == NULL) {
    return 0;
  }

  return 1;
}

// return 1 if initial checks on this io operation are valid, 0 otherwise
static int tps_io_check(size_t offset, size_t length, char* buffer)
{
  // check whether the read operation within bounds
  if (offset + length >= TPS_SIZE) {
    return 0;
  }

  // can't read or write from a null buffer
  if (buffer == NULL) {
    return 0;
  }

  return 1;
}

// change protection on a particular tps. Returns 1 on success, 0 otherwise.
static int tps_mmap_set_prot(struct tps* tps, int prot)
{
  if (tps == NULL) {
    return 0;
  }

  static const int flags = MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS;

  // calling mmap on the same region can change its protectrion
  void* ret = mmap(tps->data, TPS_SIZE, prot, flags, 0, 0);
  if (ret == MAP_FAILED) {
    return 0;
  }

  tps->data = ret;
  return 1;
}

static int tps_enable_read(struct tps* tps)
{
  return tps_mmap_set_prot(tps, PROT_READ);
}

static int tps_enable_write(struct tps* tps)
{
  return tps_mmap_set_prot(tps, PROT_WRITE);
}

static int tps_disable_read_write(struct tps* tps)
{
  return tps_mmap_set_prot(tps, PROT_NONE);
}

// TPS LIBRARY FUNCTIONS -------------------------------------------------------

static void segv_handler(int sig, siginfo_t *si, void *context)
{

  // get the address corresponding to the beginning of the page where the
  // fault occurred
  void *p_fault = (void*)((uintptr_t)si->si_addr & ~(TPS_SIZE - 1));

  // iterate through all the TPS areas and find if p_fault matches one of them
  struct tps* tps = NULL;
  queue_iterate(tps_list, addr_in_tps, &p_fault, (void**)&tps);

  // found a tps that has p_fault in its data range
  if (tps != NULL) {
    fprintf(stderr, "TPS protection error!\n");
  }

  // in any case, restore the default signal handlers
  signal(SIGSEGV, SIG_DFL);
  signal(SIGBUS, SIG_DFL);

  // and transmit the signal again in order to cause the program to crash
  raise(sig);
}

int tps_init(int segv)
{
  // protect from reinitialization
  if (tps_list != NULL) {
    return -1;
  }

  tps_list = queue_create();

  if (segv == 0) {
   // user didn't ask for segv handling, nothing more to do here
    return 0;
  }

  struct sigaction sa;
  sigemptyset(&sa.sa_mask);

  sa.sa_flags = SA_SIGINFO;
  sa.sa_sigaction = segv_handler;
  sigaction(SIGBUS, &sa, NULL);
  sigaction(SIGSEGV, &sa, NULL);

  return 0;
}

int tps_create(void)
{
  pthread_t tid = pthread_self();

  // can't create a tps for a thread that already has one
  if (has_tps(tid)) {
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

  // create storage for the tps
  struct tps* tps = malloc(sizeof(struct tps));
  memset(tps, 0, sizeof(struct tps));

  tps->owner_tid = tid;
  tps->data = data;

  // add the tps to the tps list
  int ret = queue_enqueue(tps_list, tps);
  if (ret == -1) {
    return -1;
  }

  return 0;
}

int tps_destroy(void)
{
  pthread_t tid = pthread_self();
  struct tps* tps = get_tps(tid);

  // can't destroy a tps that doesn't exist
  if (tps == NULL) {
    return -1;
  }

  int ret = queue_delete(tps_list, tps);
  if (ret == -1) {
    return -1;
  }

  // munmap is the mmap equivalent of free
  ret = munmap(tps->data, TPS_SIZE);
  if (ret == -1) {
    return -1;
  }

  free(tps);
  return 0;
}

int tps_read(size_t offset, size_t length, char *buffer)
{
  pthread_t tid = pthread_self();

  // do basic sanity checks that input is valid
  int ret = tps_io_check(offset, length, buffer);
  if (ret == -1) {
    return -1;
  }

  // can't read from a tps that doesn't exist
  struct tps* tps = get_tps(tid);
  if (tps == NULL) {
    return -1;
  }

  if (!tps_enable_read(tps)) {
    return -1;
  }

  memcpy(buffer, tps->data + offset, length);

  if (!tps_disable_read_write(tps)) {
    return -1;
  }

  return 0;
}

int tps_write(size_t offset, size_t length, char *buffer)
{
  pthread_t tid = pthread_self();

  // do basic sanity checks that input is valid
  int ret = tps_io_check(offset, length, buffer);
  if (ret == -1) {
    return -1;
  }

  // can't write to a tps that doesn't exist
  struct tps* tps = get_tps(tid);
  if (tps == NULL) {
    return -1;
  }

  if (!tps_enable_write(tps)) {
    return -1;
  }

  memcpy(tps->data + offset, buffer, length);

  if (!tps_disable_read_write(tps)) {
    return -1;
  }

  return 0;
}

int tps_clone(pthread_t tid)
{
  // ensure that the target thread even has a tps
  struct tps* target_tps = get_tps(tid);
  if (target_tps == NULL) {
    return -1;
  }

  pthread_t self = pthread_self();

  // cannot overwrite our tps if we already have one
  struct tps* self_tps = get_tps(self);
  if (self_tps == NULL) {
    return -1;
  }

  // enable reads from the target thread's TPS
  if (!tps_enable_read(target_tps)) {
    return -1;
  }

  // create new memory region for current thread's tps
  void* data = mmap(NULL, TPS_SIZE, PROT_NONE, MAP_PRIVATE|MAP_ANONYMOUS, 0, 0);
  if (data == MAP_FAILED) {
    if (!tps_disable_read_write(target_tps)) {
      fprintf(stderr, "failed disabling read/write on target thread's tps\n");
      fprintf(stderr, "library reached unrecoverable error\n");
      abort();
    }

    return -1;
  }

  // copy the data over
  self_tps->data = data;
  memcpy(self_tps->data, target_tps->data, TPS_SIZE);

  // disable reads and writes from the target thread's TPS
  if (!tps_disable_read_write(target_tps)) {
    return -1;
  }

  return 0;
}
