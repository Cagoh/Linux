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
const char f_print_sq_poll_kernel_thread_status[] = "print_sq_poll_kernel_thread_status";
const char f_geteuid[] = "geteuid";
const char f_memset[] = "memset";

const char f_io_uring_queue_exit[] = "io_uring_queue_exit";
const char f_io_uring_queue_exit2[] = "io_uring_queue_exit2";

const char f_wait[] = "wait";
const char f_child_process[] = "child_process";
const char f_fork[] = "fork";

const char f_clock[] = "clock";
const char f_clock1[] = "clock1";
const char f_clock2[] = "clock2";

int child_process(struct io_uring *ring, struct io_uring_sqe *sqe, struct io_uring_cqe **cqes, struct io_uring *ring2, struct io_uring_sqe *sqe2, struct io_uring_cqe **cqes2) {

    
    // Prepare a read
    for (int i = 0; i < writes_per_pipe; i++) {
    int remaining_completions = num_pipes;
            //printf("Child[%d][%d], remaining_completions = %d\n", i, j, remaining_completions);
            int completions;
            while (remaining_completions) {
                // Poll until done in batches
                M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_io_uring_peek_batch_cqe[0]);
                completions = io_uring_peek_batch_cqe(ring, cqes, MAX_BATCH);
                //printf("Child[%d][%d], i, j, completions = %d\n", completions);
                M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_io_uring_peek_batch_cqe[0]);
                if (completions == 0) {
                    continue;
                }

                // Check and mark every completion
                for (int j = 0; j < completions; j++) {
                    //printf("cqes[%d]->user_data = %d\n", j, cqes[j]->user_data);
                    // Check for failire
                    if (cqes[j]->res < 0) {
                        //printf("i: %d, j: %d\n", i, j);
                        printf("Parent[%d][%d]: Async task failed: %s\n", i, j,  strerror(-cqes[j]->res));
                    }

                    // Did we really read/write everything?
                    if (cqes[j]->res != BUF_SIZE) {
                        //printf("i: %d, j: %d\n", i, j);
                        printf("Parent[%d][%d]: Mismatching read/write: %d\n", i, j,  cqes[j]->res);
                    }
                    // can prep_read
                    M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_io_uring_get_sqe2[0]);
                    sqe2 = io_uring_get_sqe(ring2);
                    M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_io_uring_get_sqe2[0]);
                    
                    if (!sqe2) {
                    //printf("i: %d, j: %d\n", i, j);
                    printf("Child[%d][%d]: Cannot allocate sqe!\n", i, j);
                    return 0;
                    }
            
                    // use io_uring_prep_read instead
            
                    //printf("Child[%d][%d]: io_uring_prep_read\n");
                    M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_io_uring_prep_read[0]);
                    io_uring_prep_read(sqe2, cqes[j]->user_data * 2, buffer.iov_base + BUF_SIZE * cqes[j]->user_data * 2, BUF_SIZE, 0);
                    M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_io_uring_prep_read[0]);
            
                    sqe2->flags |= IOSQE_FIXED_FILE;
                    //sqe2->flags |= IOSQE_FIXED_FILE|IOSQE_ASYNC;
                    //sqe2->flags |= IOSQE_IO_LINK;
                    
                    //else
                        //printf("Parent[%d][%d]: Matching read/write!: %d\n", i, j,  cqes[j]->res);
                    //printf("Parent[%d][%d]: cqes[%d]->res = %d\n", i, j, j, cqes[j]->res);
                }
                
                M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_io_uring_cq_advance[0]);
                io_uring_cq_advance(ring, completions);
                M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_io_uring_cq_advance[0]);


                // Update
                remaining_completions -= completions;
            }
        //printf("Child[%d]: io_uring_submit\n",i);
        M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_io_uring_submit2[0]);
        io_uring_submit(ring2);
        M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_io_uring_submit2[0]);
                   //https://www.sobyte.net/post/2022-04/epoll-efficiently/pipe2 //printf("Child, i: %d, j: %d\n", i, j);
        }

    M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_io_uring_queue_exit2[0]);
    io_uring_queue_exit(ring2);
    M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_io_uring_queue_exit2[0]);
    
    //printf("Child process: &parent_submitted = %d\n", &parent_submitted);
    //printf("Child: After, efd2 = %d\n", efd2);
    return 1;
}

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
    
    printf("Pipes: %d\n", num_pipes);
    for (int i = 0; i < num_pipes; i++) {
        //M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_pipe2[0]);
        //if (pipe2(&pipes[i * 2], O_NONBLOCK | O_CLOEXEC)) {
        //if (pipe2(&pipes[i * 2], O_CLOEXEC)) {
        if (pipe2(&pipes[i * 2], O_NONBLOCK | O_CLOEXEC | O_DIRECT)) {
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

        struct io_uring ring2;	// this for child process reading
        struct io_uring_params params2;
        struct io_uring_sqe *sqe2;
        struct io_uring_cqe **cqes2 = malloc(sizeof(struct io_uring_cqe *) * MAX_BATCH);
        
        //M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_memset[0]);
        memset(&params2, 0, sizeof(params2));
        //M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_memset[0]);
        
        //params2.flags |= IORING_SETUP_SQPOLL | IORING_SETUP_IOPOLL;
        params2.flags |= IORING_SETUP_SQPOLL;
        params2.sq_thread_idle = 2000;
        
        //M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_io_uring_queue_init[0]);
        if (io_uring_queue_init_params(num_pipes * 2, &ring2,&params2)) {
        //if (io_uring_queue_init(num_pipes * 2, &ring,&params)) {
            printf("Cannot create an io_uring!\n");
            printf("%s\n",strerror(errno));
            return 0;
        }
    //M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_io_uring_queue_init[0]);
    
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
    
    M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_fork[0]);
    int child_pid = fork();
    M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_fork[0]);
    
    //printf("PID:%d\n",child_pid);
    if (child_pid == 0) {  
    
        M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_child_process[0]);
        child_success = child_process(&ring, sqe, cqes, &ring2, sqe2, cqes2);
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
            //parents = i;
               
            /* Prepare a write */
            /*  struct io_uring_sqe *io_uring_get_sqe(struct io_uring *ring)

    	    This function returns a submission queue entry that can be used to submit an I/O operation. You can call this function multiple times to queue up I/O requests before calling io_uring_submit() to tell the kernel to process your queued requests.
    	    */
                M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_io_uring_get_sqe[0]);
                sqe = io_uring_get_sqe(&ring);
                M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_io_uring_get_sqe[0]);
                
                //printf("Parent[%d][%d]: sqe = %d\n", i, j, sqe);
                
                if (!sqe) {
                    //printf("i: %d, j: %d\n", i, j);
                    printf("Parent[%d][%d]: Cannot allocate sqe!\n");
                    return 0;
                }
                //else
                    //printf("Parent at event: %d, sqe_allocated: %d\n", i+1, j+1);
                //printf("Parent[%d][%d]: io_uring_prep_write\n");
                    
                M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_io_uring_prep_write[0]);
                //io_uring_prep_write(sqe, rand_write[j] * 2 + 1 , buffer.iov_base + BUF_SIZE * (rand_write[j]  * 2 + 1), BUF_SIZE, 0);
                io_uring_prep_write(sqe, j * 2 + 1 , buffer.iov_base + BUF_SIZE * (j  * 2 + 1), BUF_SIZE, 0);
                // use io_uring_prep_write
                M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_io_uring_prep_write[0]);
                //sqe->flags |= IOSQE_FIXED_FILE|IOSQE_IO_LINK;
                sqe->flags |= IOSQE_FIXED_FILE;
                //sqe->flags |= IOSQE_FIXED_FILE|IOSQE_ASYNC;
                //sqe->user_data = rand_write[j];
                sqe->user_data = j;
                //sqe->flags |= IOSQE_IO_LINK;
            }
            /* Submit */
            //printf("Parent[%d]: io_uring_submit\n",i);
            
            M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_io_uring_submit[0]);
            io_uring_submit(&ring);
            //parent_submitted = i;
            //printf("Parent[%d]: parent_submitted = %d\n",i, parent_submitted);
            M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_io_uring_submit[0]);

            int remaining_completions = num_pipes;
        
        //int completions;        
        while (remaining_completions) {
        
            M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_io_uring_peek_batch_cqe2[0]);
            int completions = io_uring_peek_batch_cqe(&ring2, cqes2, MAX_BATCH);
            M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_io_uring_peek_batch_cqe2[0]);
            
            if (completions == 0) {
                continue;
            }
            
            /* Check and mark every completion */
            for (int j = 0; j < completions; j++) {
                /* Check for failire */
                if (cqes2[j]->res < 0) {
                   //https://www.sobyte.net/post/2022-04/epoll-efficiently/pipe2 //printf("Child, i: %d, j: %d\n", i, j);
                    printf("Child[%d][%d]: Async task failed: %s\n", i, j, strerror(-cqes2[j]->res));
                }

                    // Did we really read/write everything?
                if (cqes2[j]->res != BUF_SIZE) {
                    //printf("Child, i: %d, j: %d\n", i, j);
                    printf("Child[%d][%d]: Mismatching read/write: %d\n",i ,j,  cqes2[j]->res);
                }
            }
            
            M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_io_uring_cq_advance2[0]);
            io_uring_cq_advance(&ring2, completions);
            M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_io_uring_cq_advance2[0]);
            
            remaining_completions -= completions;
        }
            
    
    }
    //printf("Parent: After, efd = %d\n", efd); 
    
    M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_io_uring_queue_exit[0]);
    io_uring_queue_exit(&ring);
    M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_io_uring_queue_exit[0]);
    
    /* Wait for the child process to return */
    M5OP_TRACEPOINT(INST_TP_FUNC_ENTRY,&f_wait[0]);
    while ((wpid = wait(&status)) > 0);
    M5OP_TRACEPOINT(INST_TP_FUNC_EXIT,&f_wait[0]);

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
