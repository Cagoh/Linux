/* Minimal epoll/pipe benchmark for small messages */

#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <trace.h>

#ifdef ENABLE_PARSEC_HOOKS
#include <hooks.h>
#endif

#define MAX_PIPES 10000
#define BUF_SIZE 1024

/*
 * Introduced to determine whether a file should offload the wake up
 * operations to a dedicated hardware accelerator. Currently only
 * supported for epoll wake ups.
 */
#ifndef O_HW_WAKEUP
#define O_HW_WAKEUP	040000000
#endif

int *pipes;

const char f_malloc[] = "malloc";
const char f_memset[] = "memset";
const char f_register_pipe[] = "register_pipe";
const char f_pipe2[] = "pipe2";
const char f_epoll_ctl[] = "epoll_ctl";
const char f_epoll_create1[] = "epoll_create1";
const char f_registerThread[] = "registerThread";
const char f_write[] = "write";
const char f_read[] = "read";
const char f_epoll_wait[] = "epoll_wait";

const char f_clock[] = "clock";
const char f_clock1[] = "clock1";
const char f_clock2[] = "clock2";

//M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_io_uring_queue_init[0]);
//M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_io_uring_queue_init[0]);

/* Per poll data */
struct pipe_data {
    size_t written;
    size_t read;
    int readfd, writefd;
};

void register_pipe(int epfd, int index) {
    struct pipe_data *pd = malloc(sizeof(struct pipe_data));

    /* Create a pipe with two fds (read, write) */
    int fds[2];
    //M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_pipe2[0]);
    int p = pipe2(fds, O_NONBLOCK);
    //int p = pipe2(fds, O_NONBLOCK | O_CLOEXEC);
    //int p = pipe2(fds, 0);
    //M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_pipe2[0]);
    if(p<0){
        printf("\n %s \n",strerror(errno)); 
    }
    int readfd = fds[0];
    int writefd = fds[1];

    pd->readfd = readfd;
    pd->writefd = writefd;
    pd->written = 0;
    pd->read = 0;

    pipes[index * 2 + 1] = writefd;

    /* We only need to poll for readable as writing without is a known fast path */
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.ptr = pd;
    //M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_epoll_ctl[0]);
    epoll_ctl(epfd, EPOLL_CTL_ADD, readfd, &event);
    //M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_epoll_ctl[0]);
}

int main(int argc, char **argv) {
    int num_pipes = 0;
    int writes_per_pipe = 0;
    int useHwacc = 0;

#ifdef ENABLE_PARSEC_HOOKS
    __parsec_bench_begin(__micro_epoll_pipes_simple);
#endif

    if (argc>3){
        printf("Too many args!\n");
    }

    char* conf = getenv("CONFIG_USE_HWACC");
    if ( conf != NULL ) {
        if( strcmp(conf, "HW") == 0 ) {
            useHwacc = O_HW_WAKEUP;
        }
        else if ( strcmp(conf, "SW") == 0 ) {
            useHwacc = 0;
        }
    }

    pipes = malloc(sizeof(int) * MAX_PIPES * 2);

    sscanf(argv[1], "%d", &num_pipes);
    sscanf(argv[2], "%d", &writes_per_pipe);
    printf("Pipes: %d\n", num_pipes);
    printf("Writes per pipe: %d\n", writes_per_pipe);

    //M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_registerThread[0]);
    //registerThread();
    //M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_registerThread[0]);

    //M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_epoll_create1[0]);
    int epfd = epoll_create1(useHwacc);;
    //M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_epoll_create1[0]);
    for (int i = 0; i < num_pipes; i++) {
        //M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_register_pipe[0]);
        register_pipe(epfd, i);
        //M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_register_pipe[0]);
    }

    /* Some shared static data */
    //M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_malloc[0]);
    char *buf = malloc(BUF_SIZE);
    //M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_malloc[0]);
    
    //M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_memset[0]);
    memset(buf, 'A', BUF_SIZE);
    //M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_memset[0]);

    /* This is similar to MAX_BATCH in io_uring */
    struct epoll_event events[2000];

#ifdef ENABLE_PARSEC_HOOKS
    __parsec_roi_begin();
#endif

    /* Start time */
    //M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_clock1[0]);
    clock_t start = clock();
    M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_clock[0]);

    /* Begin iterating, starting by writing, then writing and reading */
    for (int i = 0; i < writes_per_pipe; i++) {

        /* First we write a message to all pipes */
        for (int j = 0; j < num_pipes; j++) {

            /* Pipes are ordered as readfd, writefd */
            int writefd = pipes[j * 2 + 1];

            /* Write data using syscall */
            M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_write[0]);
            int written = write(writefd, buf, BUF_SIZE);
            M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_write[0]);
            if (written != BUF_SIZE) {
                printf("Failed writing data in fast path: %d\n", written);
                return 0;
            }
        }

        /* We expect to read everything before we continue iterating */
        int remaining_reads = num_pipes;

        while (remaining_reads) {
            /* Ask epoll for events in batches */
            M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_epoll_wait[0]);
            int readyfd = epoll_wait(epfd, events, 2000, -1);
            M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_epoll_wait[0]);

            /* Handle the events by either writing or reading to the pipe */
            for (int j = 0; j < readyfd; j++) {
                /* Get associated per poll data */
                struct pipe_data *pd = events[j].data.ptr;

                /* If we can read, we read */
                if (events[j].events & EPOLLIN) {
                    M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_read[0]);
                    int r = read(pd->readfd, buf, BUF_SIZE);
                    M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_read[0]);
                    if (r != BUF_SIZE) {
                        printf("Read wrong amounts of data!\n");
                        return 0;
                    }
                    pd->read += r;
                }
            }

            /* We only keep readable fds so this is fine */
            remaining_reads -= readyfd;
        }
    }

#ifdef ENABLE_PARSEC_HOOKS
    __parsec_roi_end();
#endif

    /* Print time */
    M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_clock[0]);
    clock_t end = clock();
    //M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_clock1[0]);
    double cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Time: %f\n", cpu_time_used);

    /* Append this run */
    FILE *runs = fopen("epoll_runs", "a");
    fprintf(runs, "%f\n", cpu_time_used);

#ifdef ENABLE_PARSEC_HOOKS
    __parsec_bench_end();
#endif
    fprintf(stderr, "BENCHMARK DONE\n");
    //printf("buf: \n", buf);
    fclose(runs);
}
