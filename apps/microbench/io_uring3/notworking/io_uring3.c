/* Minimal io_uring/pipe benchmark for small messages */

#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <trace.h>

#include "liburing.h"

#define MAX_BATCH 2000

#define BUF_SIZE 1024
struct iovec buffer;

#define MAX_PIPES 10000
int *pipes;
int num_pipes = 0;
int writes_per_pipe = 0;

int count = 0;
int write_status;	// to check whether the write side (parents process) submitted the sqe

// constant strings to be printed in the trace file
//const char f_receiving[] = "receiving_msg";
//const char f_sending[] = "sending_msg";
//const char f_shr_read[] = "shr_read";
//const char f_shr_write[] = "shr_write";

const char f_pipe2[] = "pipe2";
const char f_malloc[] = "malloc";
const char f_io_uring_queue_init[] = "io_uring_queue_init";
const char f_io_uring_register_files[] = "io_uring_register_files";
const char f_io_uring_register_buffers[] = "io_uring_register_buffers";
const char f_io_uring_get_sqe[] = "io_uring_get_sqe";
const char f_io_uring_get_sqe2[] = "io_uring_get_sqe2";
const char f_io_uring_prep_write[] = "io_uring_prep_write";
const char f_io_uring_prep_read[] = "io_uring_prep_read";
const char f_io_uring_submit[] = "io_uring_submit";
const char f_io_uring_submit2[] = "io_uring_submit2";
const char f_io_uring_peek_batch_cqe[] = "io_uring_peek_batch_cqe";
const char f_io_uring_peek_batch_cqe2[] = "io_uring_peek_batch_cqe2";
const char f_io_uring_cq_advance[] = "io_uring_cq_advance";
const char f_io_uring_cq_advance2[] = "io_uring_cq_advance2";
const char f_io_uring_queue_exit[] = "io_uring_queue_exit";
const char f_io_uring_queue_exit2[] = "io_uring_queue_exit2";

const char f_wait[] = "wait";
const char f_child_process[] = "child_process";
const char f_fork[] = "fork";

const char f_clock[] = "clock";
const char f_clock1[] = "clock1";
const char f_clock2[] = "clock2";

    int child_process(struct io_uring *ring2, struct io_uring_sqe *sqe2, struct io_uring_cqe **cqes2) {

    int child_completions = 0;
    // Prepare a read
    for (int i = 0; i < writes_per_pipe; i++) {
        for (int j = 0; j < num_pipes; j++) {
                
            M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_io_uring_get_sqe2[0]);
            sqe2 = io_uring_get_sqe(ring2);
            M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_io_uring_get_sqe2[0]);
            
            if (!sqe2) {
                printf("i: %d, j: %d\n", i, j);
                printf("Child, Cannot allocate sqe!\n", j);
                return 0;
            }
            // use io_uring_prep_read instead
            
            M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_io_uring_prep_read[0]);
            io_uring_prep_read(sqe2, j * 2, buffer.iov_base + BUF_SIZE * j * 2, BUF_SIZE, 0);
            M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_io_uring_prep_read[0]);
            
            sqe2->flags |= IOSQE_FIXED_FILE;
        }

        M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_io_uring_submit2[0]);
        io_uring_submit(ring2);
        M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_io_uring_submit2[0]);
    
        int remaining_completions = num_pipes;
        
        //int completions;
        while (remaining_completions) {
        
            M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_io_uring_peek_batch_cqe2[0]);
            int completions = io_uring_peek_batch_cqe(ring2, cqes2, MAX_BATCH);
            M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_io_uring_peek_batch_cqe2[0]);
            
            if (completions == 0) {
                continue;
            }
            
            /* Check and mark every completion */
            for (int j = 0; j < completions; j++) {
                /* Check for failire */
                if (cqes2[j]->res < 0) {
                    //printf("i: %d, j: %d\n", i, j);
                    printf("Child, Async task failed: %s\n", strerror(-cqes2[j]->res));
                    //printf("cqes2[%d]->res = %d\n", j, cqes2[j]->res);
                    return 0;
                }

                    /* Did we really read/write everything? */
                if (cqes2[j]->res != BUF_SIZE) {
                    //printf("i: %d, j: %d\n", i, j);
                    printf("Child, Mismatching read/write: %d\n", cqes2[j]->res);
                    //printf("cqes2[%d]->res = %d\n", j, cqes2[j]->res);
                    return 0;
                }
            }
            
            M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_io_uring_cq_advance2[0]);
            io_uring_cq_advance(ring2, completions);
            M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_io_uring_cq_advance2[0]);
            
            remaining_completions -= completions;
        }
    }
    M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_io_uring_queue_exit2[0]);
    io_uring_queue_exit(ring2);
    M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_io_uring_queue_exit2[0]);
    return 1;
}
    
    

int main(int argc, char **argv) {
    int wpid;
    int status = 0;

    /* We have a static roof */
    //M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_malloc[0]);
    pipes = malloc(sizeof(int) * 2 * MAX_PIPES);	// I guess *2 is because https://man7.org/linux/man-pages/man2/pipe.2.html pipefd[0] refers to the read end of the pipe.  pipefd[1] refers to the write end of the pipe.
    //M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_malloc[0]);

    /* Pipes is first arg */
    sscanf(argv[1], "%d", &num_pipes);
    /* Messages is second arg */
    sscanf(argv[2], "%d", &writes_per_pipe);
    if (num_pipes > MAX_PIPES) {
        printf("Pipes: %d, exceeds MAX_PIPES: %d\n", num_pipes, MAX_PIPES);
        exit(1);
    }

    printf("Pipes: %d\n", num_pipes);
    for (int i = 0; i < num_pipes; i++) {
        //M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_pipe2[0]);
        if (pipe2(&pipes[i * 2], O_NONBLOCK | O_CLOEXEC)) {
            printf("Cannot create pipes!\n");
            return 0;
        }
        //M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_pipe2[0]);
    }

    ///* Create an io_uring */2652 Segmentation fault
    struct io_uring ring;	// this for parent process writing

    // Since we have two events per rotation, an io_ring twice the size of the number of pipes makes sense
    //M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_io_uring_queue_init[0]);
    

    
    
    
    //if (child_process > 0) {	//parent process
    
    if (io_uring_queue_init(num_pipes * 2, &ring,0)) {
       printf("Cannot create an io_uring!\n");
       printf("%s\n",strerror(errno));
        return 0;
    }
    //M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_io_uring_queue_init[0]);

    /* Register our pipes with io_uring */
    //M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_io_uring_register_files[0]);
    if (io_uring_register_files(&ring, pipes, num_pipes * 2)) {
        printf("Cannot register files with io_uring!\n");
        return 0;
    }
    //M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_io_uring_register_files[0]);
    
    /* Register our pipes with io_uring */
    //M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_io_uring_register_files[0]);
    
    //M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_io_uring_register_files[0]);

    /* Create and register buffer with io_uring */
    //M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_malloc[0]);
    buffer.iov_base = malloc(BUF_SIZE * 2 * num_pipes);
    //M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_malloc[0]);
    buffer.iov_len = BUF_SIZE * 2 * num_pipes;
    memset(buffer.iov_base, 'R', BUF_SIZE * 2 * num_pipes);

    //M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_io_uring_register_buffers[0]);
    if (io_uring_register_buffers(&ring, &buffer, 1)) {
        printf("Cannot register buffer with io_uring!\n");
        return 0;
    }
    //M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_io_uring_register_buffers[0]);
    
    //M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_io_uring_register_buffers[0]);
    
    //M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_io_uring_register_buffers[0]);

    struct io_uring_sqe *sqe;
    //M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_malloc[0]);
    struct io_uring_cqe **cqes = malloc(sizeof(struct io_uring_cqe *) * MAX_BATCH);
    //M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_malloc[0]);
    
        struct io_uring ring2;	// this for child process reading
        struct io_uring_sqe *sqe2;
        struct io_uring_cqe **cqes2 = malloc(sizeof(struct io_uring_cqe *) * MAX_BATCH);
        
        if (io_uring_queue_init(num_pipes * 2, &ring2,0)) {
            printf("Cannot create an io_uring!\n");
            printf("%s\n",strerror(errno));
            return 0;
        }
        if (io_uring_register_files(&ring2, pipes, num_pipes * 2)) {
            printf("Cannot register files with io_uring!\n");
            return 0;
        }
        if (io_uring_register_buffers(&ring2, &buffer, 1)) {
            printf("Cannot register buffer with io_uring!\n");
            return 0;
        }
    
    int child_success = 0;
    
    /* Start time */
    //M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_clock1[0]);
    clock_t start = clock();
    M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_clock[0]);
    
    /* The child is the reader while the parent is the writer */

    M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_fork[0]);
    int child_pid = fork();
    M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_fork[0]);
    
    //printf("PID:%d\n",child_pid);
    if (child_pid == 0) {  
        //child_success = child_process(num_pipes, writes_per_pipe);
        
        M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_child_process[0]);
        child_success = child_process(&ring2, sqe2, cqes2);
        M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_child_process[0]);
        
        if (child_success)
            printf("CHILD PROCESS SUCCESSFUL\n");
        exit(0);	// ensure that it return to the parent without executing the code which is below
    }
    if (child_pid < 0) {
        printf("error creating child process\n");
        return 1;
    }

    

    /* Begin iterating, starting by writing, then writing and reading */
    for (int i = 0; i < writes_per_pipe; i++) {
    /* First we write a message to all pipes */
    
        for (int j = 0; j < num_pipes; j++) {
               
            /* Prepare a write */
            /*  struct io_uring_sqe *io_uring_get_sqe(struct io_uring *ring)

    	    This function returns a submission queue entry that can be used to submit an I/O operation. You can call this function multiple times to queue up I/O requests before calling io_uring_submit() to tell the kernel to process your queued requests.
    	    */
            
                M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_io_uring_get_sqe[0]);
                sqe = io_uring_get_sqe(&ring);
                M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_io_uring_get_sqe[0]);
                if (!sqe) {
                    printf("i: %d, j: %d\n", i, j);
                    printf("Parent, Cannot allocate sqe!\n");
                    return 0;
                }
                //else
                    //printf("Parent at event: %d, sqe_allocated: %d\n", i+1, j+1);
                M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_io_uring_prep_write[0]);
                io_uring_prep_write(sqe, j * 2 + 1, buffer.iov_base + BUF_SIZE * (j * 2 + 1), BUF_SIZE, 0);
                // use io_uring_prep_write
                M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_io_uring_prep_write[0]);
                sqe->flags |= IOSQE_FIXED_FILE|IOSQE_IO_LINK;
            }
            /* Submit */
            M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_io_uring_submit[0]);
            io_uring_submit(&ring);
            

            M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_io_uring_submit[0]);

            /* Poll for completions */
            // since only need the read part, no num_pipes*2
            int remaining_completions = num_pipes;
        
        
            while (remaining_completions) {
                int completions;


                /* Poll until done in batches */
                M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_io_uring_peek_batch_cqe[0]);
                completions = io_uring_peek_batch_cqe(&ring, cqes, MAX_BATCH);
                M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_io_uring_peek_batch_cqe[0]);
                if (completions == 0) {
                    continue;
                }

                /* Check and mark every completion */
                for (int j = 0; j < completions; j++) {
                    /* Check for failire */
                    if (cqes[j]->res < 0) {
                        //printf("i: %d, j: %d\n", i, j);
                        printf("Parent, Async task failed: %s\n", strerror(-cqes[j]->res));
                        //printf("cqes2[%d]->res = %d\n", j, cqes2[j]->res);
                        return 0;
                    }

                    /* Did we really read/write everything? */
                    if (cqes[j]->res != BUF_SIZE) {
                        //printf("i: %d, j: %d\n", i, j);
                        printf("Parent, Mismatching read/write: %d\n", cqes[j]->res);
                        //printf("cqes2[%d]->res = %d\n", j, cqes2[j]->res);
                        return 0;
                    }
                }
                
                M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_io_uring_cq_advance[0]);
                io_uring_cq_advance(&ring, completions);
                M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_io_uring_cq_advance[0]);


                /* Update */
                remaining_completions -= completions;
            }
                
    
    }
    
    M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_io_uring_queue_exit[0]);
    io_uring_queue_exit(&ring);
    M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_io_uring_queue_exit[0]);
    /* Wait for the child process to return */
    
    M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_wait[0]);
    while ((wpid = wait(&status)) > 0)
    //printf("wait(NULL) = %d\n", wait(NULL));
        //printf("Waiting for Child process to return\n");
    //printf("wait(&status) = %d\n", wait(&status));
    //printf("wait(&status) = %d\n", wait(&status));
    //printf("wait(&status) = %d\n", wait(&status));
    M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_wait[0]);
    
    /* Print time */
    M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_clock[0]);
    
    clock_t end = clock();

    double cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Time: %f\n", cpu_time_used);

    /* Append this run */
    FILE *runs = fopen("io_uring_runs", "a");
    fprintf(runs, "%f\n", cpu_time_used);
    fprintf(stderr, "BENCHMARK DONE\n");
    //printf("buffer:\n",buffer);
    exit(0);

    
}
