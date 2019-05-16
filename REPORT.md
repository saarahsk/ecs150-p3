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

##### segv_handler
This function was mostly copied from the assignment handout aside from the parts
we had to fill in. The main change here was how to find a tps that was within
the memory region that just had a page fault. We used the addr_in_tps function
to do this.

##### tps_init
We have a global tps_list pointer to a queue which if it contains a non-null
address, this function has been called before. This is how we ensured that it
isn't called twice and we don't overwrite what we did before. The signal
handling code was taken from the assignment handout.

##### tps_create
This function mmaps new memory with no read or write permissions initially and
creates the memory to hold the tps data region along with the supporting
informatino around it (e.g. which thread owns it, whether it's a reference). We
then add the tps struct to the queue so that we can search for it later.

##### tps_destroy
This function not only removes the tps struct from the queue, but uses munmap to
as the equivalent of free with mmap'ed data. We then free the tps struct that
was used to house the tps data region.

##### tps_read, tps_write
These functions do the basic santity checks that we spoke about earlier, find
the tps in the queue (based on the owning thread id), and use the helper
functions to enable read/write permissions. We use memcpy to copy the data over
and pointer arithmetic to make sure that the source and destination pointers
into memory are accurate.

##### tps_clone
tps_clone doesn't call mmap but rather creates storage for the tps data region
in the form of a tps struct and adds it into the list of tps structs we have. We
set the data region to be the pointer to the target thread's data region and
also set a marker that this tps data region is a reference.

##### copy on write
The assignment told us to dissociate the TPS object from the memory page with an
extra level of indirection. It said we needed a page structure containing the
memory page's address, and TPS can only point to such page structures. This way,
two or more TPSes can point to the same page structure. Then, in order to
keep track of how many TPSes are currently "sharing" the same memory
page, the struct page must also contain a reference counter.

When we were thinking about this, we felt it was too complicated to do it that
way. Rather, we just kept a boolean in the tps struct on whether this tps struct
had a reference to another thread's tps data region. When we were actually
trying to write to the tps data regino, we just checked this boolean to see if
it was a reference. If it was, we then did the work of creating a new tps data
region and copying the memory over before allowing the write.

We felt this was much easier than what the assignment proposed and it seemed to
produce the same results.

# Testing
For P1, we utilized the test cases provided to us - sem_prime.c, sem_count.c,
and sem_buffer.c We also added the segfault test given to us in class.

# Resources
Piazza post @426 brought our attention to utilizing pthread.
(https://pubs.opengroup.org/onlinepubs/7908799/xsh/pthread.h.html). Also
consulted the lecture 7 slides and semaphore man page for semaphore
implementation.
