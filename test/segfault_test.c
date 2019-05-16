#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include <sem.h>
#include <tps.h>

void *latest_mmap_addr;

void *__real_mmap(void *addr, size_t len, int prot, int flags, int fildes, off_t off);
void *__wrap_mmap(void *addr, size_t len, int prot, int flags, int fildes, off_t off)
{
	latest_mmap_addr = __real_mmap(addr, len, prot, flags, fildes, off);
	return latest_mmap_addr;
}

void *my_thread(void *wow)
{
	char *tps_addr;

	tps_create();
	tps_addr = latest_mmap_addr;
	tps_addr[0] = '\0';
	return NULL;
} 

int main()
{
	pthread_t tid[2] = {0};
	tps_init(1);
	pthread_create(&tid[0], NULL, my_thread, NULL);
	pthread_join(tid[0], NULL);
} 
