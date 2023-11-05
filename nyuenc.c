#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "functions.h"

TaskQueue* task_queue;
OutputQueue* output_queue;

int main(int argc, char *argv[]) {
    int jobs = 1;
    int opt;
    while ((opt = getopt(argc, argv, "j:")) != -1) {
        if (opt == 'j') {
            jobs = atoi(optarg);
        }
    }

    // task_queue = (TaskQueue *)malloc(sizeof(TaskQueue));
    // init_task_queue(task_queue);
    // output_queue = (OutputQueue *)malloc(sizeof(OutputQueue));
    // init_output_queue(output_queue);
    // ThreadArgs* threadArg = (ThreadArgs *)malloc(sizeof(ThreadArgs));
    // threadArg->task_queue = task_queue;
    // threadArg->output_queue = output_queue;

    //create threads
    pthread_t *threads = malloc(jobs * sizeof(pthread_t));
    task_queue = (TaskQueue *)malloc(sizeof(TaskQueue));
    init_task_queue(task_queue);
    output_queue = (OutputQueue *)malloc(sizeof(OutputQueue));
    init_output_queue(output_queue);
    //int output_fd = STDOUT_FILENO;
    for (int i = 0; i < jobs; i++) {
        ThreadArgs* threadArg = (ThreadArgs *)malloc(sizeof(ThreadArgs));
        threadArg->task_queue = task_queue;
        threadArg->output_queue = output_queue;
        threadArg->index = i;
        pthread_create(&threads[i], NULL, thread_task, threadArg);
    }

    int task_index = 0;
    for (int i = optind; i < argc; i++) {
        // Open file
        int fd = open(argv[i], O_RDONLY);
        if (fd == -1) {
            perror("open");
            return 1;
        }
        // Get file size
        struct stat sb;
        if (fstat(fd, &sb) == -1) {
            perror("fstat");
            return 1;
        }
        // Map file into memory
        //char *addr = malloc(sb.st_size*sizeof(char*));
        char* addr = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (addr == MAP_FAILED) {
            perror("mmap");
            return 1;
        }
        // Create tasks for each chunk of the file
        off_t offset = 0;
        for (int i = 0; i < sb.st_size/CHUNK_SIZE; i++){
            Task* task = (Task*)malloc(sizeof(Task));
            task->file = addr;
            task->buffer_size = CHUNK_SIZE;
            task->buffer = addr + i*CHUNK_SIZE;
            task->index = task_index;
            //printf("full size task #%d\n",task_index);
            task_index++;
            push_task(task_queue, task);
            offset += task->buffer_size;
        }
        //fill the remainders into another task
        if (sb.st_size%CHUNK_SIZE > 0){
            Task* task = (Task*)malloc(sizeof(Task));
            task->file = addr;
            task->buffer_size = sb.st_size%CHUNK_SIZE;
            // printf("edge size is %ld\n",task->buffer_size);
            task->buffer = addr + offset;
            task->index = task_index;
            task_index ++;
            //printf("edge task #%d\n",task_index);
            push_task(task_queue, task);
            if (close(fd) == -1) {
                perror("close");
                return 1;
            }
        }
    }
    //printf("totally %d tasks\n",task_index);

    // Signal worker threads to terminate by pushing empty tasks
    for (int i = 0; i < jobs; i++) {
        Task *empty_task = (Task *)malloc(sizeof(Task));
        empty_task->buffer_size = 0;
        empty_task->index = -1;
        push_task(task_queue, empty_task);
    }
    
    int indexOfNextOutput = 0;
    char prev_encoded_char = EOF;
    int prev_encoded_count = 0;
    //put in a while loop, while indexOfOutputPrinted < task_index (number of tasks)
    while (indexOfNextOutput < task_index){
        EncodedOutput *output = NULL;
        if (output_queue->head) {
            // printf("popping output #%d\n",indexOfNextOutput);
            output = pop_output(output_queue,indexOfNextOutput);
        }
        if (output) {
            if (output->last_char != EOF){
                //check and write the last prev_encoded_char and output->first_char
                if (prev_encoded_char != output->first_char && prev_encoded_char != EOF) {
                    write(STDOUT_FILENO, &prev_encoded_char, sizeof(char));
                    write(STDOUT_FILENO, &prev_encoded_count, sizeof(char));
                } else {
                    output->first_count += prev_encoded_count;
                }
                write(STDOUT_FILENO, &output->first_char, sizeof(char));
                write(STDOUT_FILENO, &output->first_count, sizeof(char));
                //write everything but the last char
                write(STDOUT_FILENO, output->buffer, output->size);
                //update the prev_encoded_char and prev_encoded_count by last_char and last_count
                prev_encoded_char = output->last_char;
                prev_encoded_count = output->last_count;
            }
            //short case
            else{
                if (prev_encoded_char != output->first_char) {
                    if (prev_encoded_char != EOF){
                        write(STDOUT_FILENO, &prev_encoded_char, sizeof(char));
                        write(STDOUT_FILENO, &prev_encoded_count, sizeof(char));
                    }
                    prev_encoded_char = output->first_char;
                    prev_encoded_count = output->first_count;
                }
                else{
                    prev_encoded_count += output->first_count;
                }
            }
            free(output->buffer);
            free(output);
            //printf("task #%d out of %d\n",indexOfNextOutput,task_index);
            indexOfNextOutput++;
        }
    }
    if (prev_encoded_char != EOF){
        write(STDOUT_FILENO, &prev_encoded_char, sizeof(char));
        write(STDOUT_FILENO, &prev_encoded_count, sizeof(char));
    }
    
    // printf("waiting for threads to terminate\n");
    for (int i = 0; i < jobs; i++) {
        pthread_join(threads[i], NULL);
    }

    // EncodedOutput* iterator = output_queue->head;
    // int j = 0;
    // while (iterator->next){
    //     printf("%d. %d\n",j,iterator->index);
    //     iterator = iterator->next;
    //     j++;
    // }
    // printf("%d. %d\n",j,iterator->index);

    free(task_queue);
    free(output_queue);
    //free(threadArg);
    free(threads);
}
