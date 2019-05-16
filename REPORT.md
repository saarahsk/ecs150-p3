# P3 Report

# User-level Thread Library (part 2)

## Data Structures

##### semaphore
The semaphore data structure contains two variables - an internal count of
type int and a queue of threads waiting to access to access a critical section.

## Functions

##### sem_create
This function allocates and initializes a semaphore's count and thread queue.

##### sem_destroy
This function frees a semaphore from memory, assuming the semaphore exists or
there are no threads still being blocked on it.

##### sem_down
This takes a resource from the semaphore in question.  It gave a bit of trouble
at first due to a mistake in putting the enqueue statement prior to
the thread block statement. Upon success, we decrement the count and exit
the critical section.

##### sem_up
This function releases a resource to the semaphore by dequeueing it from
waiting, then unblocking it.  If for some reason these actions fail, we exit
the critical section and return -1. On success, we do the same, but return 0.

#### sem_value
In the case of the semaphore count being greater than 0, we set the count
value to the sval passed into the function.  And if the count equates to 0, we
check if the waiting queue exists, exiting if not and assigning a negative number
whose absolute value is the count of the number of threads currently blocked
in sem_down().

# Testing
For P1, we utilized the test cases provided to us - sem_prime.c, sem_count.c,
and sem_buffer.c We also added the segfault test given to us in class.

# Resources
Piazza post @426 brought our attention to utilizing pthread.
(https://pubs.opengroup.org/onlinepubs/7908799/xsh/pthread.h.html). Also
consulted the lecture 7 slides and semaphore man page for semaphore
implementation.

