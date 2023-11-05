#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

#define CHUNK_SIZE 4096

typedef struct Task Task;

struct Task {
    char *file;
    char *buffer;
    size_t buffer_size;
    int index;
    Task *next;
};

typedef struct {
    Task *head;
    Task *tail;
    pthread_cond_t empty;
    pthread_mutex_t lock;
    int is_empty;
} TaskQueue;

// Forward declaration of the EncodedOutput struct
struct EncodedOutput;

typedef struct EncodedOutput {
    char *buffer;
    int size;
    char last_char;
    int last_count;
    char first_char;
    int first_count;
    int is_filled;  //sets to true when the task has been submitted
    int index;
    struct EncodedOutput *next;
} EncodedOutput;

typedef struct {
    EncodedOutput *head;
    EncodedOutput *tail;
    pthread_cond_t empty;
    pthread_mutex_t lock;
} OutputQueue;

typedef struct {
    TaskQueue *task_queue;
    OutputQueue *output_queue;
    int index;
} ThreadArgs;

//extern TaskQueue task_queue;

//void encode_chunk(Task *task, int output_fd);
//void encode_chunk(Task *task, EncodedOutput *output);
void init_task_queue(TaskQueue *queue);
void init_output_queue(OutputQueue *queue);
void push_task(TaskQueue *queue, Task *task);
EncodedOutput* pop_output(OutputQueue *queue, int index);
void *thread_task(void* arg);








