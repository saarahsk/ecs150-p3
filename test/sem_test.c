#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include <sem.h>
sem_t sem1, sem2;

void threadA(void)
{
	printf("A in\n");
	sem_up(sem1);
	sem_down(sem2);
	printf("A out\n");
}

void threadB(void)
{
	sem_down(sem1);
	printf("B in\n");
	sem_up(sem2);
}

int main(){
	sem1 = sem_create(0);
	sem2 = sem_create(0);
	threadA();
	threadB();
	sem_destroy(sem1);
	sem_destroy(sem2);
	return 0;
}
