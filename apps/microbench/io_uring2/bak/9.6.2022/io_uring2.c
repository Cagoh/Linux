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
const char f_io_uring_prep_write[] = "io_uring_prep_write";
const char f_io_uring_prep_read[] = "io_uring_prep_read";
const char f_io_uring_submit[] = "io_uring_submit";
const char f_io_uring_peek_batch_cqe[] = "io_uring_peek_batch_cqe";
const char f_io_uring_cq_advance[] = "io_uring_cq_advance";

const char f_clock[] = "clock";
const char f_clock1[] = "clock1";
const char f_clock2[] = "clock2";

int main(int argc, char **argv) {

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

    /* Create an io_uring */
    struct io_uring ring;
    // Since we have two events per rotation, an io_ring twice the size of the number of pipes makes sense
    //M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_io_uring_queue_init[0]);
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

    struct io_uring_sqe *sqe;
    //M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_malloc[0]);
    struct io_uring_cqe **cqes = malloc(sizeof(struct io_uring_cqe *) * MAX_BATCH);
    //M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_malloc[0]);

    /* Start time */
    //M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_clock1[0]);
    clock_t start = clock();
    M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_clock[0]);
    

    for (int i = 0; i < writes_per_pipe; i++) {
        for (int j = 0; j < num_pipes; j++) {
            /* Prepare a write */
            
            /*  struct io_uring_sqe *io_uring_get_sqe(struct io_uring *ring)

    	    This function returns a submission queue entry that can be used to submit an I/O operation. You can call this function multiple times to queue up I/O requests before calling io_uring_submit() to tell the kernel to process your queued requests.
    	    */
            
            M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_io_uring_get_sqe[0]);
            sqe = io_uring_get_sqe(&ring);
            M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_io_uring_get_sqe[0]);
            if (!sqe) {
                printf("Cannot allocate sqe!\n");
                return 0;
            }
            M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_io_uring_prep_write[0]);
            io_uring_prep_write(sqe, j * 2 + 1, buffer.iov_base + BUF_SIZE * (j * 2 + 1), BUF_SIZE, 0);
            // use io_uring_prep_write
            M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_io_uring_prep_write[0]);
            sqe->flags |= IOSQE_FIXED_FILE|IOSQE_IO_LINK;

            /* Prepare a read */
            M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_io_uring_get_sqe[0]);
            sqe = io_uring_get_sqe(&ring);
            M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_io_uring_get_sqe[0]);
            if (!sqe) {
                printf("Cannot allocate sqe!\n");
                return 0;
            }
            M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_io_uring_prep_read[0]);
            io_uring_prep_read(sqe, j * 2, buffer.iov_base + BUF_SIZE * j * 2, BUF_SIZE, 0);
            // use io_uring_prep_read instead
            M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_io_uring_prep_read[0]);
            sqe->flags |= IOSQE_FIXED_FILE;
        }
        /* Submit */
        M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_io_uring_submit[0]);
        //io_uring_submit(&ring);
        M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_io_uring_submit[0]);

        for (int j = 0; j < num_pipes * 2; j++) {
        // https://github.com/axboe/liburing/issues/347
                if (io_uring_wait_cqe(&ring, cqes) < 0) {
                    printf("error waiting for any completions\n");
                    return 0;
                }
                /* Check for failire */
                //printf("Now at i j: %d %d\n",i,j);
                printf("cqes[0]: %p\n", cqes[j]);
                //if (cqes[i*num_pipes+j]->res < 0) {
                if (cqes[0]->res < 0) {
                    printf("Async task failed: %s\n", strerror(-cqes[j]->res));
                    return 0;
                }

                /* Did we really read/write everything? */
                //if (cqes[i*num_pipes+j]->res != BUF_SIZE) {
                if (cqes[0]->res != BUF_SIZE) {
                    printf("Mismatching read/write: %d\n", cqes[j]->res);
                    return 0;
                }
            }
        
        
        
        /* Does the same as io_uring_cqe_seen (that literally does io_uring_cq_advance(&ring, 1)), except all at once */
        M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_io_uring_cq_advance[0]);
        io_uring_cq_advance(&ring, num_pipes * 2);
        M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_io_uring_cq_advance[0]);

        /* Update */
    }
    

    /* Print time */
    M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_clock[0]);
    clock_t end = clock();
    //M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_clock1[0]);
    double cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Time: %f\n", cpu_time_used);

    /* Append this run */
    FILE *runs = fopen("io_uring_runs", "a");
    fprintf(runs, "%f\n", cpu_time_used);
    fprintf(stderr, "BENCHMARK DONE\n");
    fclose(runs);
}
