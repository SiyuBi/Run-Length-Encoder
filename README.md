A multi-threaded encoding application written in C. It is designed to perform encoding tasks on chunks of data using multiple threads to improve performance and efficiency.

## Features

- Multi-threaded encoding of data.
- Task and output queue management for threads.
- Use of POSIX threads (pthreads) for concurrent task processing.
- Memory mapping for efficient file handling.
- Signal handling for graceful shutdown and cleanup.

## Compilation

To compile the project, use the included Makefile:

```sh
make all
```

This will compile the source files and produce the `nyuenc` executable.

## Usage

To run the encoder, use the following command:

```sh
./nyuenc [options] <file>
```

Options:
- `-j <number>`: Set the number of jobs (threads) to use for encoding.

## Implementation Details

- `Makefile`: Contains the build instructions for the project.
- [`nyuenc.c`](https://github.com/SiyuBi/nyuenc/blob/main/nyuenc.c): The main program file that sets up threading and file processing.
- [`functions.c`](https://github.com/SiyuBi/nyuenc/blob/main/functions.c): Implements the core encoding logic and queue operations.
- [`functions.h`](https://github.com/SiyuBi/nyuenc/blob/main/functions.h): Header file with declarations for tasks, queues, and thread arguments.
