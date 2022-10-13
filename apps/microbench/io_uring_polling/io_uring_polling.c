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
const char f_print_sq_poll_kernel_thread_status[] = "print_sq_poll_kernel_thread_status";
const char f_geteuid[] = "geteuid";
const char f_memset[] = "memset";

const char f_io_uring_queue_exit[] = "io_uring_queue_exit";

const char f_clock[] = "clock";
const char f_clock1[] = "clock1";
const char f_clock2[] = "clock2";

void print_sq_poll_kernel_thread_status() {    
    if (system("sudo ps aux | grep iou-sqp" ) == 0 || system("sudo ps aux | grep io_uring-sq" ) == 0)
        printf("Kernel thread io_uring-sq found running...\n");
    else {
        //printf("Kernel thread io_uring-sq is not running.\n");
        printf("Kernel thread iou-sqp is not running.\n");
        exit(1);
    }
    
}

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
        //if (pipe2(&pipes[i * 2], O_CLOEXEC)) {
            printf("Cannot create pipes!\n");
            return 0;
        }
        //M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_pipe2[0]);
    }

    /* Create an io_uring */
    struct io_uring ring;
    // Since we have two events per rotation, an io_ring twice the size of the number of pipes makes sense
    struct io_uring_params params;
    
    //M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_geteuid[0]);
    if (geteuid()) {
        fprintf(stderr, "You need root privileges to run this program.\n");
        return 1;
    }
    //M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_geteuid[0]);

    
    //M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_memset[0]);
    memset(&params, 0, sizeof(params));
    //M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_memset[0]);
    
    params.flags |= IORING_SETUP_SQPOLL;
    params.sq_thread_idle = 2000;
    
    //M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_io_uring_queue_init[0]);
    if (io_uring_queue_init_params(num_pipes * 2, &ring,&params)) {
    //if (io_uring_queue_init(num_pipes * 2, &ring,&params)) {
        printf("Cannot create an io_uring!\n");
        printf("%s\n",strerror(errno));
        return 0;
    }
    //M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_io_uring_queue_init[0]);
    
    
    //M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_print_sq_poll_kernel_thread_status[0]);
    print_sq_poll_kernel_thread_status();
    //M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_print_sq_poll_kernel_thread_status[0]);
    
    
    
    //M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_io_uring_register_files[0]);
    /* Register our pipes with io_uring */
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
    
    //M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_memset[0]);
    memset(buffer.iov_base, 'R', BUF_SIZE * 2 * num_pipes);
    //M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_memset[0]);

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
            
            M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_io_uring_get_sqe[0]);
            sqe = io_uring_get_sqe(&ring);
            M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_io_uring_get_sqe[0]);
            if (!sqe) {
                printf("Cannot allocate sqe!\n");
                return 0;
            }
            
            
            M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_io_uring_prep_write[0]);
            io_uring_prep_write(sqe, j * 2 + 1, buffer.iov_base + BUF_SIZE * (j * 2 + 1), BUF_SIZE, 0);
            M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_io_uring_prep_write[0]);
            
            sqe->flags |= IOSQE_FIXED_FILE|IOSQE_IO_LINK;

            /* Prepare a read */
            M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_io_uring_get_sqe[0]);
            sqe = io_uring_get_sqe(&ring);
            if (!sqe) {
                printf("Cannot allocate sqe!\n");
                return 0;
            }
            M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_io_uring_get_sqe[0]);
            
            M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_io_uring_prep_read[0]);
            io_uring_prep_read(sqe, j * 2, buffer.iov_base + BUF_SIZE * j * 2, BUF_SIZE, 0);
            M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_io_uring_prep_read[0]);
            
            sqe->flags |= IOSQE_FIXED_FILE;
        }
        /* Submit */
        M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_io_uring_submit[0]);
        io_uring_submit(&ring);
        M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_io_uring_submit[0]);

        /* Poll for completions */
        int remaining_completions = num_pipes * 2;
        while (remaining_completions) {
            
            /* Poll until done in batches */
            
            M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_io_uring_peek_batch_cqe[0]);
            int completions = io_uring_peek_batch_cqe(&ring, cqes, MAX_BATCH);
            // try io_uring_wait_cqe(&ring, cqes)
            M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_io_uring_peek_batch_cqe[0]);
            
            if (completions == 0) {
                continue;
            }

            /* Check and mark every completion */
            for (int j = 0; j < completions; j++) {

                /* Check for failire */
                if (cqes[j]->res < 0) {
                    printf("Async task failed: %s\n", strerror(-cqes[j]->res));
                    return 0;
                }

                /* Did we really read/write everything? */
                if (cqes[j]->res != BUF_SIZE) {
                    printf("Mismatching read/write: %d\n", cqes[j]->res);
                    return 0;
                }
            }

            /* Does the same as io_uring_cqe_seen (that literally does io_uring_cq_advance(&ring, 1)), except all at once */
            
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
