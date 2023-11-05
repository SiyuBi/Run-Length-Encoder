#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include "functions.h"
EncodedOutput *outputs;

int *taskNum = 0;

EncodedOutput* encode_chunk(Task *task) {
    // printf("start encoding\n");
    char* buffer = task->buffer;
    EncodedOutput* output = malloc(sizeof(EncodedOutput));
    //first char
    output->first_char = buffer[0];
    output->first_count = 1;
    size_t i = 1;
    while (i < task->buffer_size && buffer[i] == output->first_char){
        output->first_count++;
        i++;
    }

    //middle
    int short_case = 0;
    int previous = EOF;
    int count = 1;
    char* output_buffer = malloc(task->buffer_size * 2 * sizeof(char));
    int output_pos = 0;
    while (i < task->buffer_size) {
        int current = task->buffer[i];
        // printf("current %c\n",current);
        if (current == previous) {
            count++;
            // printf("count ++\n");
        } else {
            short_case = 1;
            // printf("previous %c\n",previous);
            // printf("count %d\n",count);
            if (previous != EOF) {
                output_buffer[output_pos] = previous;
                output_pos++;
                output_buffer[output_pos] = count;
                output_pos++;
            }
            previous = current;
            count = 1;
        }
        i++;
    }

    if (short_case == 0) {
        // only one character exists in the task
        // printf("short case\n");
        // printf("char: %c\n",output->first_char);
        output->last_char = EOF;
        output->last_count = 0;
    } else {
        // normal long case
        output->last_char = previous;
        output->last_count = count;
    }
    output->buffer = output_buffer;
    output->size = output_pos;
    output->is_filled = 1;
    output->index = task->index;
    return output;
}

void init_task_queue(TaskQueue *queue) {
    queue->head = NULL;
    queue->tail = NULL;
    pthread_cond_init(&queue->empty, NULL);
    pthread_mutex_init(&queue->lock, NULL);
}

void init_output_queue(OutputQueue *queue) {
    queue->head = NULL;
    queue->tail = NULL;
    pthread_cond_init(&queue->empty, NULL);
    pthread_mutex_init(&queue->lock, NULL);
}

void push_task(TaskQueue *queue, Task *task) {
    task->next = NULL;
    pthread_mutex_lock(&queue->lock);

    if (queue->tail) {
        queue->tail->next = task;
    }
    queue->tail = task;

    if (!queue->head) {
        queue->head = task;
    }
    pthread_cond_signal(&queue->empty);
    // printf("pushed tail #%d\n",queue->tail->index);
    pthread_mutex_unlock(&queue->lock);
}

void push_output(OutputQueue *queue, EncodedOutput *output) {
    output->next = NULL;
    pthread_mutex_lock(&queue->lock);

    if (queue->tail) {
        queue->tail->next = output;
    }
    queue->tail = output;

    if (!queue->head) {
        queue->head = output;
        pthread_cond_signal(&queue->empty);
    }
    pthread_mutex_unlock(&queue->lock);
}

Task* pop_task(TaskQueue *queue) {
    pthread_mutex_lock(&queue->lock);
    while (queue->head == NULL) {
        pthread_cond_wait(&queue->empty, &queue->lock); // wait for queue to become non-empty
    }
    //printf("thread %d locked\n",myIndex);
    // printf("head index %d\n",queue->head->index);
    Task* task = queue->head;
    //printf("popped task #%d\n",task->index);
    // Task *old_head = queue->head;
    queue->head = queue->head->next;
    if (queue->head == NULL) {
        queue->tail = NULL;
        queue->is_empty = 1;
    }

    pthread_mutex_unlock(&queue->lock);
    // free(old_head);
    //printf("thread %d unlocked\n",myIndex);
    // printf("return index %d\n",task->index);
    return task;
}

EncodedOutput* pop_output(OutputQueue *queue, int index) {
    // printf("popping output #%d\n",index);
    pthread_mutex_lock(&queue->lock);
    while (queue->head == NULL) {
        pthread_cond_wait(&queue->empty, &queue->lock); // wait for queue to become non-empty
    }

    //printf("want to pop %d\n",index);
    EncodedOutput *output = NULL;
    EncodedOutput *current = queue->head;
    // printf("current head is %d\n",current->index);
    EncodedOutput *prev = NULL;
    while (current) {
            // printf("searching... index is %d\n",current->index);
        //printf("popped %d\n",current->index);
        if (current->index == index) {
            output = current;
            if (prev) {
                prev->next = current->next;
            } else {
                queue->head = current->next;
            }
            if (!current->next) {
                queue->tail = prev;
            }
            break;
        }
        prev = current;
        current = current->next;
    }
    // if (output == NULL){
    //     printf("didn't find output #%d\n",index);
    // }
    // else{
    //     printf("popped output #%d\n",output->index);
    // }
    pthread_mutex_unlock(&queue->lock);
    return output;
}

void *thread_task(void* arg) {
    ThreadArgs* argument = (ThreadArgs *)arg;
    TaskQueue *task_queue = argument->task_queue;
    OutputQueue *output_queue = argument->output_queue;
    // int myIndex = argument->index;
    //printf("thread #%d created\n",myIndex);
    while (1) {
        //printf("thread %d trying to pop task\n",myIndex);
        Task* task = pop_task(task_queue);
        //printf("thread %d popped task # %d\n",myIndex,task->index);
        if (task->buffer_size == 0) {
            //printf("thread %d breaking...............................................\n",myIndex);
            //free(task.buffer); // Free the task buffer memory
            break;
        }
        // printf("encode task # %d\n",task->index);
        EncodedOutput* output = encode_chunk(task);
        //printf("encoded task # %d\n",output->index);
        push_output(output_queue,output);
        // printf("pushed result # %d\n",output->index);
        free(task);
        //printf("got here\n");
    }
    return NULL;
}

