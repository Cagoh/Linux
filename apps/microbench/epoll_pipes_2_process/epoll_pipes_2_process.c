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
#include <sys/wait.h>
#include <trace.h>

#ifdef ENABLE_PARSEC_HOOKS
#include <hooks.h>
#endif

#define MAX_PIPES 10000
#define BUF_SIZE 1024

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
const char f_epoll_wait2[] = "epoll_wait2";

const char f_fork[] = "fork";
const char f_child_process[] = "child_process";
const char f_wait[] = "wait";

const char f_clock[] = "clock";
const char f_clock1[] = "clock1";
const char f_clock2[] = "clock2";

/*
 * Introduced to determine whether a file should offload the wake up
 * operations to a dedicated hardware accelerator. Currently only
 * supported for epoll wake ups.
 */
#ifndef O_HW_WAKEUP
#define O_HW_WAKEUP	040000000
#endif

/*
// 100 pipes
ran = {79, 89, 93, 99, 46, 52, 49, 76, 95, 48, 64, 9, 42, 73, 8, 32, 88, 29, 56, 81, 65, 27, 94, 16, 60, 68, 10, 98, 47, 11, 61, 18, 75, 69, 15, 91, 54, 37, 77, 50, 5, 97, 44, 36, 0, 67, 83, 31, 74, 14, 41, 84, 57, 70, 82, 86, 24, 3, 59, 12, 20, 33, 28, 45, 55, 71, 1, 78, 30, 17, 63, 96, 87, 7, 34, 19, 92, 25, 90, 23, 43, 72, 13, 4, 26, 38, 62, 39, 53, 21, 85, 35, 58, 51, 22, 66, 40, 6, 2, 80}
*/
/*
// 50 pipes
int rand_write[50] = {27, 0, 45, 25, 46, 14, 47, 8, 33, 6, 21, 49, 38, 1, 15, 19, 35, 43, 37, 5, 20, 29, 32, 26, 39, 41, 23, 17, 16, 48, 4, 18, 12, 22, 24, 10, 44, 7, 28, 40, 34, 3, 31, 30, 36, 11, 9, 42, 2, 13};

int rand_read[50] = {29, 2, 24, 8, 37, 23, 18, 6, 11, 44, 12, 43, 33, 20, 27, 9, 41, 25, 46, 38, 42, 39, 13, 10, 22, 21, 36, 34, 47, 4, 5, 45, 48, 28, 7, 30, 31, 1, 40, 26, 49, 35, 14, 17, 0, 3, 19, 15, 16, 32};
*/

int *rand_write;

// 50 pipes
int rand_write_50[50] = {39, 31, 10, 11, 2, 45, 21, 4, 41, 30, 28, 6, 13, 35, 7, 44, 1, 14, 48, 40, 9, 26, 32, 17, 0, 5, 3, 22, 46, 38, 49, 29, 27, 25, 42, 36, 47, 23, 34, 37, 33, 43, 12, 8, 20, 16, 15, 18, 19, 24};

// 100 pipes
int rand_write_100[100] = {84, 69, 79, 41, 26, 99, 64, 97, 50, 74, 45, 58, 12, 2, 25, 81, 31, 78, 60, 38, 4, 43, 76, 42, 83, 66, 29, 82, 94, 27, 86, 44, 80, 7, 92, 8, 59, 52, 55, 73, 17, 89, 15, 19, 47, 1, 34, 20, 63, 22, 51, 85, 70, 61, 0, 62, 36, 68, 9, 18, 32, 35, 23, 67, 48, 53, 98, 3, 56, 93, 14, 28, 6, 96, 72, 13, 21, 40, 87, 77, 24, 37, 5, 75, 11, 39, 95, 71, 90, 30, 49, 16, 65, 54, 57, 91, 46, 10, 33, 88};

// 200 pipes
int rand_write_200[200] = {57, 132, 25, 187, 47, 144, 45, 27, 135, 154, 105, 156, 175, 2, 9, 126, 73, 195, 192, 160, 59, 44, 36, 185, 152, 161, 117, 110, 141, 193, 133, 93, 43, 142, 76, 137, 70, 26, 40, 102, 35, 165, 164, 99, 191, 183, 84, 30, 10, 172, 121, 149, 86, 83, 112, 134, 120, 155, 176, 18, 194, 181, 116, 148, 196, 80, 94, 3, 109, 87, 33, 46, 123, 55, 14, 38, 101, 85, 100, 81, 92, 71, 104, 198, 145, 67, 180, 74, 56, 186, 4, 65, 39, 113, 136, 54, 1, 124, 151, 11, 128, 169, 139, 131, 58, 173, 66, 129, 15, 91, 78, 72, 0, 32, 114, 88, 31, 115, 106, 89, 190, 153, 77, 7, 197, 63, 97, 12, 150, 146, 49, 138, 98, 159, 103, 95, 8, 75, 20, 119, 167, 29, 69, 158, 17, 28, 199, 96, 108, 23, 24, 127, 61, 168, 90, 42, 19, 22, 162, 130, 107, 178, 50, 179, 48, 62, 184, 6, 157, 5, 52, 68, 21, 147, 182, 64, 189, 125, 188, 37, 111, 51, 13, 163, 79, 170, 166, 122, 118, 171, 177, 143, 53, 174, 140, 82, 34, 16, 41, 60};

// 300 pipes
int rand_write_300[300] = {226, 100, 280, 242, 121, 158, 245, 159, 113, 74, 287, 183, 49, 284, 130, 163, 13, 84, 116, 218, 289, 65, 282, 187, 279, 91, 41, 23, 124, 214, 128, 199, 197, 54, 33, 235, 254, 202, 209, 92, 264, 273, 281, 77, 117, 156, 165, 129, 172, 241, 50, 259, 196, 53, 40, 19, 232, 285, 278, 146, 212, 244, 115, 11, 176, 45, 277, 181, 194, 15, 105, 243, 138, 55, 52, 27, 155, 86, 148, 75, 90, 147, 20, 201, 126, 253, 258, 191, 5, 239, 133, 9, 247, 60, 47, 188, 220, 135, 136, 167, 154, 263, 230, 210, 108, 78, 118, 200, 295, 76, 296, 132, 299, 123, 234, 109, 61, 206, 88, 141, 82, 29, 0, 71, 95, 189, 207, 185, 94, 93, 81, 8, 250, 37, 139, 260, 195, 223, 66, 217, 56, 190, 265, 262, 85, 69, 83, 192, 275, 211, 112, 170, 288, 12, 30, 110, 271, 31, 142, 237, 17, 248, 140, 184, 101, 43, 298, 216, 127, 119, 256, 35, 149, 137, 286, 39, 225, 175, 261, 228, 73, 294, 174, 290, 48, 1, 153, 4, 107, 276, 173, 7, 205, 238, 10, 251, 79, 38, 122, 46, 150, 104, 97, 6, 68, 34, 168, 57, 143, 180, 106, 22, 252, 272, 292, 221, 16, 215, 267, 63, 64, 89, 182, 274, 224, 204, 25, 171, 233, 134, 246, 283, 186, 3, 166, 144, 203, 240, 120, 32, 96, 161, 36, 14, 177, 269, 21, 24, 26, 255, 178, 268, 114, 62, 293, 249, 162, 270, 219, 58, 98, 222, 157, 125, 51, 231, 227, 72, 80, 99, 131, 151, 266, 42, 297, 145, 2, 164, 291, 179, 229, 152, 87, 193, 198, 28, 70, 67, 160, 236, 111, 213, 103, 257, 18, 169, 44, 102, 208, 59};

// 400 pipes
int rand_write_400[400] = {268, 16, 13, 259, 300, 179, 124, 149, 253, 306, 359, 288, 151, 260, 278, 335, 106, 190, 102, 72, 113, 211, 193, 392, 54, 286, 22, 233, 195, 275, 157, 126, 371, 274, 1, 139, 395, 3, 57, 238, 60, 27, 241, 71, 85, 120, 176, 43, 294, 344, 378, 170, 30, 380, 383, 276, 36, 394, 245, 386, 194, 225, 353, 254, 307, 84, 387, 310, 41, 207, 94, 173, 375, 224, 175, 327, 140, 314, 96, 104, 150, 343, 304, 146, 153, 339, 77, 367, 212, 112, 87, 61, 163, 50, 127, 143, 115, 93, 180, 32, 23, 301, 141, 399, 313, 334, 255, 26, 117, 361, 298, 125, 318, 201, 183, 49, 177, 181, 156, 79, 216, 90, 232, 237, 222, 136, 134, 369, 110, 385, 39, 273, 188, 199, 82, 331, 354, 111, 20, 281, 317, 358, 80, 46, 252, 271, 91, 261, 251, 319, 148, 364, 29, 28, 95, 246, 365, 63, 338, 360, 129, 347, 108, 340, 65, 342, 103, 290, 169, 220, 137, 262, 158, 75, 189, 388, 366, 221, 312, 265, 121, 284, 187, 172, 135, 308, 393, 240, 123, 397, 205, 329, 64, 38, 48, 363, 5, 166, 128, 92, 323, 133, 345, 398, 192, 9, 299, 155, 99, 208, 285, 174, 182, 209, 98, 83, 147, 257, 114, 324, 283, 24, 396, 239, 165, 219, 101, 256, 269, 178, 200, 68, 291, 230, 144, 210, 217, 58, 234, 376, 295, 325, 368, 0, 191, 12, 311, 4, 348, 374, 70, 337, 341, 132, 215, 152, 266, 244, 280, 297, 10, 160, 8, 336, 186, 247, 373, 321, 53, 107, 206, 346, 55, 305, 171, 320, 270, 223, 272, 86, 119, 21, 14, 302, 138, 44, 351, 197, 322, 263, 218, 350, 242, 45, 142, 185, 277, 131, 250, 326, 76, 249, 37, 328, 184, 122, 25, 214, 293, 382, 349, 33, 2, 264, 204, 370, 74, 384, 40, 228, 357, 213, 167, 17, 164, 35, 154, 162, 292, 303, 362, 52, 97, 267, 356, 243, 18, 130, 73, 67, 118, 51, 355, 231, 379, 47, 377, 226, 81, 66, 168, 56, 315, 69, 309, 289, 287, 352, 7, 34, 279, 6, 332, 89, 105, 258, 161, 282, 316, 145, 202, 109, 391, 390, 330, 159, 381, 236, 42, 59, 15, 372, 198, 19, 389, 62, 333, 227, 248, 100, 235, 229, 296, 116, 203, 11, 31, 196, 88, 78};

// 500 pipes
int rand_write_500[500] = {196, 262, 250, 64, 114, 241, 386, 209, 492, 169, 342, 390, 351, 219, 246, 173, 137, 223, 416, 226, 273, 453, 92, 424, 57, 46, 252, 409, 177, 429, 353, 436, 53, 437, 10, 324, 371, 245, 291, 374, 406, 41, 43, 352, 478, 400, 16, 200, 277, 270, 449, 336, 108, 365, 240, 428, 321, 477, 208, 210, 283, 194, 454, 188, 483, 198, 309, 2, 52, 421, 470, 395, 8, 44, 260, 153, 102, 271, 33, 345, 257, 377, 65, 19, 104, 244, 313, 186, 79, 215, 263, 370, 143, 59, 446, 70, 282, 212, 217, 417, 125, 456, 289, 75, 174, 17, 368, 9, 23, 201, 305, 450, 405, 99, 296, 24, 35, 318, 146, 162, 69, 225, 163, 490, 176, 251, 22, 312, 164, 138, 348, 402, 28, 27, 479, 485, 234, 343, 303, 62, 235, 172, 473, 471, 12, 320, 481, 278, 258, 26, 124, 495, 118, 154, 88, 459, 66, 329, 158, 15, 335, 30, 3, 337, 204, 247, 5, 414, 199, 317, 261, 266, 58, 364, 178, 49, 333, 467, 248, 72, 397, 256, 373, 106, 171, 265, 165, 126, 239, 491, 298, 202, 42, 339, 255, 151, 105, 300, 25, 81, 175, 206, 379, 411, 111, 149, 480, 419, 237, 182, 127, 297, 366, 427, 48, 110, 20, 487, 493, 224, 185, 332, 383, 74, 420, 213, 141, 315, 61, 253, 190, 426, 157, 139, 31, 290, 412, 38, 346, 287, 396, 286, 344, 54, 193, 236, 7, 484, 307, 462, 90, 231, 355, 442, 39, 281, 120, 326, 494, 115, 360, 166, 97, 314, 170, 415, 375, 80, 51, 302, 439, 233, 113, 145, 465, 67, 385, 357, 103, 398, 63, 331, 438, 91, 372, 422, 96, 187, 159, 191, 45, 85, 119, 474, 295, 128, 36, 60, 156, 441, 267, 387, 323, 78, 376, 133, 272, 482, 393, 384, 394, 455, 86, 150, 292, 447, 0, 107, 279, 264, 410, 230, 445, 155, 458, 475, 301, 311, 243, 350, 167, 444, 430, 218, 71, 259, 6, 294, 4, 94, 269, 18, 242, 388, 161, 134, 77, 197, 55, 56, 304, 101, 112, 100, 179, 222, 220, 274, 268, 184, 68, 121, 401, 122, 214, 195, 285, 84, 116, 382, 276, 211, 123, 275, 306, 369, 327, 407, 460, 499, 322, 47, 358, 129, 319, 457, 37, 452, 443, 227, 1, 299, 338, 468, 11, 180, 431, 310, 361, 14, 432, 391, 359, 148, 142, 232, 293, 73, 463, 280, 440, 144, 34, 40, 228, 378, 288, 340, 254, 160, 109, 238, 367, 29, 207, 389, 168, 341, 349, 192, 497, 418, 435, 284, 469, 496, 434, 413, 203, 83, 216, 347, 229, 93, 130, 152, 136, 50, 140, 381, 403, 399, 21, 380, 423, 425, 308, 334, 87, 498, 95, 328, 89, 325, 404, 433, 13, 189, 476, 117, 76, 316, 135, 408, 466, 354, 451, 472, 356, 131, 392, 486, 98, 464, 488, 82, 132, 362, 147, 181, 183, 461, 249, 489, 363, 205, 448, 221, 330, 32};

/* Per poll data */
struct pipe_data {
    size_t written;
    size_t read;
    int readfd, writefd;
};

struct pipe_data *pipes;

void register_pipe(int epfd_read, int epfd_write, int index) {
    struct pipe_data *pd = malloc(sizeof(struct pipe_data));

    /* Create a pipe with two fds (read, write) */
    int fds[2];
    int p = pipe2(fds, O_NONBLOCK);
    if(p<0){
        printf("\n %s \n",strerror(errno)); 
    }
    int readfd = fds[0];
    int writefd = fds[1];

    pd->readfd = readfd;
    pd->writefd = writefd;
    pd->written = 0;
    pd->read = 0;

    pipes[index] = *pd;
    /* This is the read epoll */
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.ptr = &pipes[index];
    epoll_ctl(epfd_read, EPOLL_CTL_ADD, readfd, &event);
    
    /* This is the write epoll */
    event.events = EPOLLOUT;
    event.data.ptr = &pipes[index];
    epoll_ctl(epfd_write, EPOLL_CTL_ADD, writefd, &event);
}

// The child process is responsible for reading the data from the pipe
int child_process(int epfd_read, int num_pipes, unsigned int writes_per_pipe){

    int j;
    struct epoll_event events[2000];
    char *buf2 = malloc(BUF_SIZE);
    /* We expect to read everything before we continue iterating */
    int remaining_reads = num_pipes * writes_per_pipe;

    while (remaining_reads) {
        /* Ask epoll for events in batches */
        
        M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_epoll_wait2[0]);
        int readyfd = epoll_wait(epfd_read, events, 2000, 1000);
        M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_epoll_wait2[0]);
        
        if (readyfd == 0){
            printf("TIMEOUT read side\n");
            printf("remaining reads = %d\n", remaining_reads);
            return 1;
        }

        /* Handle the events by reading from the pipe */
        for (j = 0; j < readyfd; j++) {
            /* Get associated per poll data */
            struct pipe_data *pd = events[j].data.ptr;

            /* If we can read, we read */
            if (events[j].events & EPOLLIN) {
            
                M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_read[0]);
                int r = read(pd->readfd, buf2, BUF_SIZE);
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
    printf("Reading done!\n");
    
    /* 
     * Check if all pipes were read correctly
     * cant check writes here because we are in a different process/adress space
     */
    for(j=0;j<num_pipes;j++){
        if(pipes[j].read != BUF_SIZE*writes_per_pipe)
            printf("Error wrong reads! Pipe: %d #reads: %ld\n",j,pipes[j].read);
    }
    close(epfd_read);
    return 0;
}

int main(int argc, char **argv) {
    int i,j,wpid;
    int status = 0;
    int num_pipes = 0;
    int written;
    unsigned int writes_per_pipe = 0;
    int useHwacc = 0;
    
    

#ifdef ENABLE_PARSEC_HOOKS
    __parsec_bench_begin(__micro_epoll_pipes);
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

    sscanf(argv[1], "%d", &num_pipes);
    sscanf(argv[2], "%d", &writes_per_pipe);
    printf("Pipes: %d\n", num_pipes);
    printf("Writes per pipe: %d\n", writes_per_pipe);
    

    switch(num_pipes) {
        case 50:
            rand_write = rand_write_50;
            break;
        case 100:
            rand_write = rand_write_100;
            break;
        case 200:
            rand_write = rand_write_200;
            break;    
        case 300:
            rand_write = rand_write_300;
            break;
        case 400:
            rand_write = rand_write_400;
            break;
        case 500:
            rand_write = rand_write_500;
            break;
    }
    pipes = malloc(sizeof(struct pipe_data *) * num_pipes);
    struct pipe_data *pd;

    int epfd_read = epoll_create1(useHwacc);;
    int epfd_write = epoll_create1(useHwacc);;
    for (int i = 0; i < num_pipes; i++) {
        register_pipe(epfd_read, epfd_write, i);
    }
    /* Some shared static data */
    char *buf = malloc(BUF_SIZE);
    memset(buf, 'A', BUF_SIZE);

    /* This is similar to MAX_BATCH in io_uring */
    struct epoll_event events[2000];

    /* The child is the reader while the parent is the writer */
    //registerThread();
    
    


#ifdef ENABLE_PARSEC_HOOKS
    __parsec_roi_begin();
#endif

    /* Start time */
    clock_t start = clock();
    M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_clock[0]);
    
    M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_fork[0]);
    int child_pid = fork();
    M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_fork[0]);
    
    //registerThread();
    if (child_pid == 0) {	//means when calling fork(), return 0 means its at the child processes, other positive number means still remains in parents process
    
        M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_child_process[0]);
        child_process(epfd_read,num_pipes,writes_per_pipe);
        M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_child_process[0]);
        
        //printf("1:\n");
        /* close the read side of the pipe */
        for (i = 0; i < num_pipes; i++){
            close(pipes[i].readfd);
        }
        return 0;
    }
    
    /* Begin iterating, starting by writing, then writing and reading */
    //for (int i = 0; i < writes_per_pipe; i++) {
    /* First we write a message to all pipes */
        for (j = 0; j < num_pipes; j++) {

            /* Write data using syscall */
            
            M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_write[0]);
            //written = write(pipes[rand_write[j]].writefd, buf, BUF_SIZE);
            written = write(pipes[j].writefd, buf, BUF_SIZE);
            M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_write[0]);
            
            
            
            if (written != BUF_SIZE) {
                printf("Failed writing data in fast path: %d\n", written);
                printf("\n %s \n",strerror(errno)); 
                return 0;
            }
            /* Increment the write counter in this process */
            //pipes[j].written += written;
            pipes[j].written += written;
        }

        /* 
         * Determine the total number of writes based 
         * on the amount of writes per pipe and number of pipes
         * -num_pipes because we have already performed one write to all pipes above
         */
        int remaining_writes = (num_pipes*writes_per_pipe)-num_pipes;
        /* While we still have remaining writes, do them */
        while (remaining_writes >0){
            /* Use epoll_wait to determine on which pipes a write is possible */
            
            M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_epoll_wait[0]);
            int readyfd = epoll_wait(epfd_write, events, 2000, 1000);
            M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_epoll_wait[0]);
            
            /* Error checking */
            if (readyfd < 0){
                printf("\n %s\n",strerror(errno));
                goto out;
            }
            /* If we timed out, something probably went wrong */
            else if (readyfd == 0){
                printf("TIMEOUT write side\n");
                printf("remaining_writes: %d\n",remaining_writes);
                goto out;
            }
            /* "Normal" case when epoll_wait return valid entries */
            else{
                /* 
                 * Go through the different pipes that are now ready to 
                 * be written to and write to them
                 */
                for(j=0; j<readyfd; j++){
                    /* 
                     * Pipe info is stored in events[i].data.ptr
                     * Only writefd and readfd "valid"
                     * written and read not updated for other process
                     */
                    pd = events[j].data.ptr;
                    /* Check if event is correct */
                    if (events[j].events & EPOLLOUT) {
                        /* Check if we still need to write to this pipe */
                        if (pd->written<BUF_SIZE*writes_per_pipe){
                            /* Do the actual write */
                            
                            M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_write[0]);
                            written = write(pd->writefd, buf, BUF_SIZE);
                            M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_write[0]);
                            
                                        
                            /* Error checking */
                            if (written != BUF_SIZE) {
                                printf("Read wrong amounts of data!\n");
                                return 0;
                            }
                            /* Update the written value for this pipe */
                            pd->written += written;
                        }
                        /* 
                         * In case the pipe already complete all writes,
                         * increment remaining writes because we would skip it as
                         * we use readyfd to update the remaining writes later
                         */
                        else{
                            remaining_writes++;
                        }

                    }
                }
                /* Update the remaining writes value */
                remaining_writes -= readyfd;
            }
        }
    //}

out:
    /* Wait for the child process to return */
    
    M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_wait[0]);
    while ((wpid = wait(&status)) > 0);
    M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_wait[0]);
    

#ifdef ENABLE_PARSEC_HOOKS
    __parsec_roi_end();
#endif

 

    /*
     * Check if the amount of writtes performed on each pipe is correct.
     * Cant check reads here as they are only updated in the same process!
     */
    for (i=0; i<num_pipes; i++){
        if(pipes[i].written != BUF_SIZE*writes_per_pipe)
            printf("Error wrong writes! Pipe: %d #writes: %ld\n",i,pipes[i].written);
    }
    /* close the write side of the pipe */
    for (i = 0; i<num_pipes; i++){
        close(pipes[i].writefd);
    }
    close(epfd_read);
    close(epfd_write);
    
    /* Record time */
    M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_clock[0]);
    clock_t end = clock();
    /* Print time */
    double cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Time: %f\n", cpu_time_used);

    /* Append this run */
    FILE *runs = fopen("epoll_runs", "a");
    fprintf(runs, "%f\n", cpu_time_used);

#ifdef ENABLE_PARSEC_HOOKS
    __parsec_bench_end();
#endif
    fprintf(stderr, "BENCHMARK DONE\n");
    fclose(runs);
}
