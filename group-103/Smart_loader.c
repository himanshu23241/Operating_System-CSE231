#include "loader.h"


Elf32_Ehdr *ehdr;   // elf header
Elf32_Phdr *phdr;   // program header
int fd;             // file discriptor
unsigned char *p_alloc =NULL;
unsigned int max_add=0;
unsigned long Cumulative_memory_allocated=0;
unsigned long Full_memory_allocated=0;
// variables for page fault, allocation, and fragmentation
int Page_fault=0;
int Page_allocation=0;
long Full_internal_fragmentation=0;

/*
 * release memory and other cleanups
 */
void loader_cleanup(){
    // for elf header
    if (ehdr !=NULL) {
        free(ehdr);
        // good practice
        ehdr=NULL;
    }
    // for program header
    if (phdr != NULL) {
        free(phdr);
        phdr = NULL;
    }
    // for file descriptor
    if (fd != -1) {
        close(fd);
        fd = -1;
    }
    // for allocated pages
    if (p_alloc != NULL) {
        free(p_alloc);
        p_alloc = NULL;
    }
}


// Segmentation fault handler -> when SIGSEGV signal is received (due to invalid memory access)
void Handeling_segmentation_fault(int signo,siginfo_t *si,void *context){
    // checking if signal is indeed SIGSEGV
    if(si->si_signo == SIGSEGV){
        // finding fault address
        void *fault_address= si->si_addr;
        unsigned int page_size = 4096;
        // finding the start address of the page where the fault occurs
        void *start_of_page= (void *)((uintptr_t)fault_address & ~(page_size - 1));
        // finding the page index
        size_t ind_of_page= (uintptr_t)start_of_page / page_size;

        if(p_alloc[ind_of_page] != 1) {
            Page_fault++;    // increment the page fault coung
            // iterate through each program header to find the segment for the page fault
            for(int i=0;i < ehdr->e_phnum;i++){
                Elf32_Phdr phdr;
                off_t offset= ehdr->e_phoff + (ehdr->e_phentsize*i);
                // Error handeling while setting offest to program header
                if(lseek(fd, offset, SEEK_SET) == -1){
                    perror("Error setting offset to program header offset");
                    exit(EXIT_FAILURE);
                }
                // Error handeling while reading program header
                if(read(fd,&phdr,ehdr->e_phentsize) == -1){
                    perror("Error reading program header");
                    exit(EXIT_FAILURE);
                }
                // loading the segement and checking if it is within fault address
                if((phdr.p_type == PT_LOAD) && ((uintptr_t)start_of_page >= phdr.p_vaddr) && ((uintptr_t)start_of_page < (phdr.p_vaddr + phdr.p_memsz)) ){
                    // mapping only the page fault
                    void *segment_memory= mmap(start_of_page,page_size,PROT_READ | PROT_WRITE | PROT_EXEC,MAP_PRIVATE | MAP_ANONYMOUS,-1,0);
                    // Error while mapping the faulted memory segement
                    if(segment_memory == MAP_FAILED){
                        perror("Error while mapping memory segement");
                        exit(EXIT_FAILURE);
                    }
                    // finding the offset for the page fault within the segment
                    size_t offset_in_segment= (uintptr_t)start_of_page - phdr.p_vaddr;
                    // Error handeling while setting the offset to the segement
                    if(lseek(fd,phdr.p_offset + offset_in_segment, SEEK_SET) == -1){
                        perror("Error while setting the offset to program header");
                        exit(EXIT_FAILURE);
                    }
                    size_t bytes_to_read= (offset_in_segment + page_size > phdr.p_filesz) ? (phdr.p_filesz - offset_in_segment) : page_size;

                    // Error handeling while reading program header
                    if(read(fd,segment_memory,bytes_to_read)== -1){
                        perror("Error while reading program header");
                        exit(EXIT_FAILURE);
                    }
                    // changing the flag for pages that has been checked
                    p_alloc[ind_of_page]= 1;
                    // increment allocation count
                    Page_allocation++;
                    // in case of last page of the segement track total internal framentation
                    if(page_size+offset_in_segment >= phdr.p_memsz){
                        Full_internal_fragmentation += page_size - (phdr.p_memsz % page_size);
                    }
                    break;
                }
            }
        }
    }
}

/*
 * Load and run the ELF executable file
 */

void load_and_run_elf(char **exe) {
    fd = open(exe[1], O_RDONLY);
    // Error checking while opening file
    // flag for checking if file is open or not
    int is_file_open;
    // if file not open correctly fd will be negative
    if(fd < 0){         
        is_file_open=0;
    }
    else{
        is_file_open=1;
    }
    if(is_file_open== 0){
        printf("Error opening the file\n");
        exit(EXIT_FAILURE);
    }

    // reading and allocating memory for elf header
    ehdr=malloc(sizeof(Elf32_Ehdr));
    // Error handeling while allocating memory for elf header
    if (ehdr==NULL){
        perror("Error loading the elf header\n");
        loader_cleanup();
        return;
    }

    ssize_t count_b_file=read(fd,ehdr,sizeof(Elf32_Ehdr));
    // Error handeling while reading elf header
    if(count_b_file<0){
    perror("Error reading the elf header");
    loader_cleanup();
    exit(EXIT_FAILURE);
    }
    else if(count_b_file!=sizeof(Elf32_Ehdr)){
    fprintf(stderr,"Error elf header is incomplete\n");
    loader_cleanup();
    exit(EXIT_FAILURE);
    }

    // reading and allocating memory for program header
    phdr=malloc(ehdr->e_phnum * sizeof(Elf32_Phdr)); 
    // e_phnum is the number of elements in program header
    // Error checking during memory allocation for program header
    if(phdr==NULL){
        perror("Error during Memory allocation for program header");
        loader_cleanup();
        exit(EXIT_FAILURE);
    }
    
    // Iterate through the PHDR table and find the section of PT_LOAD
    for(int i=0;i < ehdr->e_phnum;i++){
        Elf32_Phdr phdr;
        // calculating and finding the program header offset
        off_t offset= ehdr->e_phoff +(ehdr->e_phentsize*i);
        // setting the offset to the program header's position in the file
        if(lseek(fd,offset,SEEK_SET) == -1){
            perror("Error while seeking to program header offset");
            exit(EXIT_FAILURE);
        }
        // reading the program header 
        if(read(fd, &phdr, ehdr->e_phentsize) == -1){
            perror("Error while reading program header");
            exit(EXIT_FAILURE);
        }
        // checking if phdr is loadable and if needed ,updating maximum address
        if(phdr.p_type == PT_LOAD){
            // phdr->vaddr is the virtual address allocated in the program header in the memory
            // phdr->p_memsz is the size of the virtual address allocated
            unsigned int e_add = phdr.p_vaddr + phdr.p_memsz;
            if(e_add > max_add){
                max_add=e_add;
            }
        }
    }

    // calculating number of pages needed and allocate tracking memory
    size_t num_page= (size_t)(max_add / getpagesize()) + 1;
    // allocating memory for tracking of allocated pages
    p_alloc=(unsigned char *)calloc(num_page,sizeof(unsigned char));
    // Error handeling while allocating memory for pages
    if(p_alloc == NULL){
        perror("Error while allocating memory for page tracking");
        exit(EXIT_FAILURE);
    }

    // initilizing signal handler for segmentation faults
    struct sigaction sa;
    sa.sa_flags=SA_SIGINFO;
    sa.sa_sigaction=Handeling_segmentation_fault;
    sigaction(SIGSEGV,&sa,NULL);

    // typecasting entry point address to that of function pointer
    typedef int(*Start_Func)();
    Start_Func _start=(Start_Func)(uintptr_t)(ehdr->e_entry);
    // starting execution
    int result=_start();
    // cleaning
    loader_cleanup();

    // printing the details of page fault,page allocation and fragmentation
    printf("***************************************************\n");
    printf("Summary of the program EXECUTION:\n");
    printf("***************************************************\n");
    printf("Result from _start           : %d\n",result);
    printf("Total Page Faults            : %d\n",Page_fault);
    printf("Total Page Allocations       : %d\n",Page_allocation);
    printf("Internal Fragmentation       : %.3f KB\n",(float)Full_internal_fragmentation / 1024);
    printf("***************************************************\n");
}

// main function
int main(int argc, char **argv){
    if (argc != 2){
        printf("Usage: %s <ELF Executable>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    // 1. carry out necessary checks on the input ELF file

    // using the access function which is necessary to check file accessibility
    int n1=access(argv[1],F_OK);
    int n2=access(argv[1],R_OK);

    if(n1==-1){
        printf("The file is not in the directory\n");
        exit(EXIT_FAILURE);
    }
    else if (n2==-1){
        printf("The file does not have read permission\n");
        exit(EXIT_FAILURE);
    }
    // 2. passing it to the loader for carrying out the loading/execution
    load_and_run_elf(argv);
    // 3. invoke the cleanup routine inside the loader  
    loader_cleanup();
    return 0;
}
