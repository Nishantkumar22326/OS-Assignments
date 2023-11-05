#include <stdio.h>
#include <elf.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <signal.h>
#include <errno.h>
#include "loader.h"

Elf32_Phdr *phdr;
Elf32_Ehdr *ehdr;

int fd;

// Data structure for loaded segments
typedef struct Segment {
    Elf32_Phdr phdr;
    void *virtual_memory;
    size_t num_pages;
} Segment;

Segment *segments;

int num_segments = 0;
int page_size = 4096; // 4KB page size
int page_faults = 0;
int total_page_allocations = 0 ;
int total_internal_fragmentation = 0 ;

void loader_cleanup();
void load_and_run_elf(char **exe) ;
int load_segment (int index) ;

// signal handling function (catching and handling the segmentation fault)
void segfault_handler(int signo, siginfo_t *info, void *context) {
    if (signo == SIGSEGV) {
        page_faults++;
        //handling the segmentation fault here
        Elf32_Addr fault_addr = (Elf32_Addr)info->si_addr;
        // printf("Segmentation fault occurred at address: %p\n", info->si_addr);

        int segment_index = -1;
        for (int i = 0; i < num_segments; i++) {
            if (fault_addr >= (Elf32_Addr)segments[i].phdr.p_vaddr &&
                fault_addr < (Elf32_Addr)(segments[i].phdr.p_vaddr + segments[i].phdr.p_memsz)) {
                segment_index = i;
                break;
            }
        }

        if (segment_index != -1) {
            // calculating the no. of pages required for the segment
            Segment *segment = &segments[segment_index];
            size_t num_pages = (segment->phdr.p_memsz + page_size - 1) / page_size;

            // allocating the memory for the segment on the basis of number of pages
            void *memory = mmap((void *)segment->phdr.p_vaddr, num_pages *page_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS, -1, 0);
            //error handling
            if (memory == MAP_FAILED) {
                perror("mmap failed");
                exit(1);
            }

            total_page_allocations += num_pages;

            // Calculating internal fragmentation (fragmented page size)
            int internal_fragmentation = (int)(num_pages * page_size - segment->phdr.p_memsz);
            total_internal_fragmentation += internal_fragmentation;

            // Copying the segment content from the file
            if (lseek(fd, segment->phdr.p_offset, SEEK_SET) == -1) {
                perror("Error seeking to segment offset");
                exit(1);
            }
            if (read(fd, memory, segment->phdr.p_filesz) != segment->phdr.p_filesz) {
                perror("Error reading segment data");
                exit(1);
            }
            segment->virtual_memory = memory;
            segment->num_pages = num_pages;
            return;
        }
    }
}

// main function
int main(int argc, char **argv) {
    struct sigaction sa;
    printf("Executing the main function......\n");
    printf("Executing the file...\n");
    sa.sa_sigaction = segfault_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_SIGINFO;
    if (sigaction(SIGSEGV, &sa, NULL) == -1) {
        perror("Error setting up the signal handler");
        return 1;
    }

    if (argc != 2) {
        printf("Usage: %s <ELF Executable>\n", argv[0]);
        exit(1);
    }

    load_and_run_elf(argv);
    printf("Total page faults: %d\n", page_faults);
    printf("Total page allocations: %d\n", total_page_allocations);
    printf("Total internal fragmentation (KB): %f\n", total_internal_fragmentation/1024.0);
    loader_cleanup();

    return 0;
}

// function for elf
void load_and_run_elf(char **exe) {
    long long int answer = -1;
    char *file = exe[1];
    fd = open(file, O_RDONLY);

    if (fd < 0) {
        perror("There was an error opening the file\n");
        exit(1);
    }
    ehdr = (Elf32_Ehdr *)malloc(sizeof(Elf32_Ehdr));
    if (!ehdr) {
        perror("Memory allocation failed\n");
        exit(1);
    } else if (read(fd, ehdr, sizeof(Elf32_Ehdr)) != sizeof(Elf32_Ehdr)) {
        perror("There was an error reading ELF header\n");
        exit(1);
    }

    phdr = (Elf32_Phdr *)malloc(ehdr->e_phnum * sizeof(Elf32_Phdr));

    if (!phdr) {
        perror("Error allocating memory for program headers\n");
        exit(1);
    } else if (lseek(fd, ehdr->e_phoff, SEEK_SET) == -1) {
        perror("Error seeking to the program header table\n");
        exit(1);
    } else if (read(fd, phdr, ehdr->e_phnum * sizeof(Elf32_Phdr)) != ehdr->e_phnum * sizeof(Elf32_Phdr)) {
        perror("Error reading program headers");
        exit(1);
    }

    // allocating memory for all segments
    segments = (Segment *)malloc(ehdr->e_phnum * sizeof(Segment));
    if (!segments) {
        perror("Error allocating memory for segments\n");
        exit(1);
    }

    for (int i = 0; i < ehdr->e_phnum; i++) {
        

        // Storing the segment information
            segments[num_segments].phdr = phdr[i];
            segments[num_segments].virtual_memory = NULL;
            num_segments++;
        }


    

    // catching the entry point address and running the program and handling page faults
    int (*entry_address)() = (int (*)())(uintptr_t)(ehdr->e_entry);
    while (entry_address != NULL) {
        answer = entry_address();
        printf("The output of _start: %lld\n", answer);

        int page_fault = load_segment(num_segments);
        if (page_fault < 0) {
            // perror("Error handling page fault");
            // exit(1);
            break;
        }
    }
}

int load_segment(int index) {
    if (index >= num_segments) {
        // no segments for loading
        return -1;
    }
    Segment *segment = &segments[index];
    if (segment->virtual_memory == NULL) {
        void *memory = mmap(
            (void *)segment->phdr.p_vaddr,
            segment->phdr.p_memsz,
            PROT_READ | PROT_WRITE | PROT_EXEC,
            MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS,
            -1,
            0
        );

        if (memory == MAP_FAILED) {
            perror("mmap failed");
            exit(1);
        }
        if (lseek(fd, segment->phdr.p_offset, SEEK_SET) == -1) {
            perror("Error seeking to segment offset");
            exit(1);
        }
        if (read(fd, memory, segment->phdr.p_filesz) != segment->phdr.p_filesz) {
            perror("Error reading segment data");
            exit(1);
        }

        segment->virtual_memory = memory;
    }

    return 0;
}

// cleanup function
void loader_cleanup() {
    if (ehdr) {
        free(ehdr);
    }

    if (phdr) {
        free(phdr);
    }

    if (segments) {
        for (int i = 0; i < num_segments; i++) {
            if (segments[i].virtual_memory) {
                munmap(segments[i].virtual_memory, segments[i].phdr.p_memsz);
            }
        }
        free(segments);
    }

    close(fd);
}