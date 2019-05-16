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
check if the waiting queue exists, exiting if not and assigning a negative
number whose absolute value is the count of the number of threads currently
blocked in sem_down().

##### thread private storage
The thread private storage (tps from here on) data structure contains a
pthread_t signifying the owning thread's id, a void* pointing to the data region
that the tps represents, and an integer, used as a boolean to signify whether it
is a reference to another thread's tps.

We initially tried to have an array of tps structs indexed by the thread id.
However, this quickly became problematic. The values of the thread ids were huge
and it doesn't make sense to allocate such a huge array when only a few elements
may even be filled in. We decided to make use of the queue again in order to
keep track of our tps structs. This meant that when we were to go searching for
tps structs, we would be doing a linear search through the queue using the
queue_iterate function already available.

A better approach would have been a hash table or something but we didn't think
of that until writing this report. It would have been easy to simply have an
array represent the hash table and use the modulo operator with the thread id to
index into it. We could have then used chaining as a collision avoidance
mechanism. If we had more time and weren't so close to the deadline, this is the
first thing we would have liked to fix.

## Functions

##### find_item
This function is a callback function we used with queue_iterate to find the tps
in question.

##### addr_in_tps
This function is a callback function we used with queue_iterate to find a tps
that had an address range surrounding the given address. We used this in the
segv_handler to determine whether a particular segmentation fault was due to tps
errors or not.

##### get_tps and has_tps
These functions are simply wrapper functions around queue_iterate to help make
code a little bit easier to read.

##### tps_io_check
Another helper function which does some basic sanity checks when we are trying
to perform io on a particular memory region. Some of the sanity checks are
things like whether the offset and length result in memory that is within the
bounds of the tps data region.

##### tps_mmap_set_prot, tps_enable_*, tps_disable_read_write
These functions are helper functions as well to make it easier to change the
permissions on a given memory region. We introduced it to our code to make
things a bit easier to read and avoid code repetition.

# Testing
For P1, we utilized the test cases provided to us - sem_prime.c, sem_count.c,
and sem_buffer.c We also added the segfault test given to us in class.

# Resources
Piazza post @426 brought our attention to utilizing pthread.
(https://pubs.opengroup.org/onlinepubs/7908799/xsh/pthread.h.html). Also
consulted the lecture 7 slides and semaphore man page for semaphore
implementation.
