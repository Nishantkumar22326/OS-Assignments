/* LOADER IMPLEMENTATION USING "C" */

#include "loader.h"

Elf32_Phdr *phdr;
Elf32_Ehdr *ehdr;


int fd;

/*
    Release memory and other cleanups
 */

void loader_cleanup();

// function for running and loading the elf file

void load_and_run_elf(char **exe);

int main(int argc, char **argv)
{
    if (argc != 2)
    {

        printf("Usage: %s <ELF Executable>\n", argv[0]);
        exit(1);
    }

    load_and_run_elf(argv);

    loader_cleanup();

    return 0;
}

void load_and_run_elf(char **exe)
{
    long long int answer = -1;
    char *file = exe[1];
    fd = open(file, O_RDONLY);

    // ERROR HANDLING CASES

    if (fd < 0)
    {
        perror("There was an error opening the file\n");
        exit(1);
    }

    ehdr = (Elf32_Ehdr *)malloc(sizeof(Elf32_Ehdr)); // ALLOCATING memory to ehdr  

    if (!ehdr) // if memory allocation fails 
    {
        perror("Memory allocation failed\n");
        exit(1);
    }

    else if (read(fd, ehdr, sizeof(Elf32_Ehdr)) != sizeof(Elf32_Ehdr)) // if the size isn't right 
    {
        perror("There was an error reading ELF header\n");
        exit(1);
    }

    phdr = (Elf32_Phdr *)malloc(ehdr->e_phnum * sizeof(Elf32_Phdr)); // allocating memory to phdr  

    if (!phdr) // if memory allocation fails 
    {
        perror("Error allocating memory for program headers\n");
        exit(1);
    }

    else if (lseek(fd, ehdr->e_phoff, SEEK_SET) == -1) 
    {
        perror("Error seeking to program header table\n");
        exit(1);
    }

    else if (read(fd, phdr, ehdr->e_phnum * sizeof(Elf32_Phdr)) != ehdr->e_phnum * sizeof(Elf32_Phdr))
    {
        perror("Error reading program headers");
        exit(1);
    }

    int x = 0;
    while (x < ehdr->e_phnum)
    {
        if (phdr[x].p_type == PT_LOAD) 
        {
            // Allocating memory of size “p_memsz” using mmap function and copying the segment content 
            void *virtual_memory = mmap((void *)phdr[x].p_vaddr, phdr[x].p_memsz, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_FIXED, fd, phdr[x].p_offset);
            if (virtual_memory == MAP_FAILED) 
            {
                perror("mmap failed : There has occurred an error");
                exit(1);
            }
        }
        x++;
    }

    // Calling the entry point of the loaded ELF program

    int (*entry_address)() = (int (*)())(uintptr_t)(ehdr->e_entry);
    answer = entry_address(); // Update final answer 

    printf("%lld\n", answer);
}

void loader_cleanup()
{
    if (ehdr)
    {

        free(ehdr);
        return; // free up the memory
    }

    if (phdr)
    {

        free(phdr);
        return; // free up the memory
    }

    close(fd);
    return; // free up the memory
}
