#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#define MAX_PROCESSES 100

struct SharedMemory {
    int process_ids[MAX_PROCESSES];
    int ind;
};

#endif