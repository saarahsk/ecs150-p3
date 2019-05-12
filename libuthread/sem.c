#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#include "queue.h"
#include "sem.h"
#include "thread.h"


struct semaphore {
	int count;
    queue_t waiting;
};


sem_t sem_create(size_t count)
{
	struct semaphore *sem = malloc(sizeof(struct semaphore));
    //checks for malloc error
    if (!sem) {
        return NULL;
    }
    //creates queue of waiting threads
    sem->waiting = queue_create();
    sem->count = count;

    return sem;
}

int sem_destroy(sem_t sem)
{
	if (!sem || sem->count == 0){
        return -1;
    }
    free(sem);
    return 0;
}

int sem_down(sem_t sem)
{
	if (!sem){
        return -1;
    }

    //enter the critical section nad check if we can take a resource
    //if it fails for whatever reason, we exit the CS and return -1
    enter_critical_section();
    if(sem->count == 0){
        //if the count is 0, we block the thread
        if (thread_block() == -1){
            fprintf(stderr, "Error: could not block thread.");
            exit_critical_section();
            return -1;
        }
        //and then add the blocked thread to the waiting threads
        if (queue_enqueue(sem->waiting, (void*) pthread_self()) == -1){
            fprintf(stderr, "Error: could not add thread to queue");
            exit_critical_section();
            return -1;
        }
    }
    //on success, we decrement the count and leave the crit section
    sem->count--;
    exit_critical_section();
    return 0;
}

int sem_up(sem_t sem)
{
	if (!sem){
        return -1;
    }
    //enter the critical section so we can potentially release a sem
    enter_critical_section();
    sem->count++;

    //initialize a thread tid so we can check the tid of the head thread
    pthread_t p_tid;

    //if this fails we exit the crit section and return -1
    if (sem->count == 0){
        if (queue_dequeue(sem->waiting, (void**)&p_tid) == -1){
            fprintf(stderr, "Couldn't dequeue!");
            exit_critical_section();
            return -1;
        }
        if (thread_unblock(p_tid) == -1){
            fprintf(stderr, "This tid does not correspond to a currently blocked thread!");
            exit_critical_section();
            return -1;
        }
    }
    exit_critical_section();
    return 0;
}

int sem_getvalue(sem_t sem, int *sval)
{
	if (!sem || !sval){
        return -1;
    }
    if (sem->count > 0){
        sem->count = *sval;
        return 0;
    }

    if (sem->count == 0){
        if (!sem->waiting){
            fprintf(stderr, "There are no waiting threads!");
            return -1;
        }
        *sval = queue_length(sem->waiting) * -1;
        return 0;
    }
    return 0;
}
