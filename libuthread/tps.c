#include <assert.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "queue.h"
#include "thread.h"
#include "tps.h"

/* TODO: Phase 2 */

int tps_init(int segv)
{
	return 0;
}

int tps_create(void)
{
	return 0;
}

int tps_destroy(void)
{
	return 0;
}

int tps_read(size_t offset, size_t length, char *buffer)
{
	return 0;
}

int tps_write(size_t offset, size_t length, char *buffer)
{
	return 0;
}

int tps_clone(pthread_t tid)
{
	return 0;
}

