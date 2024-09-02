#include "loader.h"

Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
int fd;

/*
 * release memory and other cleanups
 */
void loader_cleanup() {
  // using free function to release the allocated memory
  free(ehdr);
  free(phdr);
}

/*
 * Load and run the ELF executable file
 */

void load_and_run_elf(char** exe) {

  fd = open(exe[1], O_RDONLY);
  // checking if file is open or not
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

  // 1. Load entire binary content into the memory from the ELF file.

  // calculating size of binary file
  int start_ind=0;
  int end_ind=SEEK_END;
  off_t b_size=lseek(fd,start_ind,end_ind);  

  // allocating memory and loading whole binary file
  int total_b_size=b_size+1;
  char *b_file=(char *)malloc(total_b_size);

  ssize_t com_b_file=read(fd,b_file,b_size);

  lseek(fd,start_ind,SEEK_SET);       // set the offset to start of elf file

  // reading and allocating space for elf header
  ehdr=(Elf32_Ehdr*)malloc(sizeof(Elf32_Ehdr));
  ssize_t size_of_elf_header=read(fd,ehdr,sizeof(Elf32_Ehdr));  
  // checking if memeory is allocate for the elf header or not
  if(ehdr==NULL){
    printf("Error loading the elf header\n");
    exit(EXIT_FAILURE);
  }
  
  lseek(fd,start_ind,SEEK_SET);        // set the offset to start of elf file
  read(fd,ehdr,sizeof(Elf32_Ehdr));    // using read system call to read the file allocated in heap memory

  
  // 2. Iterate through the PHDR table and find the section of PT_LOAD 

  // memory allocation for the program header 
  phdr=(Elf32_Phdr*)malloc(ehdr->e_phentsize);
  
  // setting the offset to e_phoff -> starting address of the program header
  lseek(fd,ehdr->e_phoff,SEEK_SET);
 
  // e_phnum is the number of elements in program header
  // phdr->vaddr is the virtual address allocated in the program header in the memory
  // phdr->p_memsz is the size of the virtual address allocated
  int n1=0;
  while(n1 < ehdr->e_phnum){
    if (read(fd,phdr,ehdr->e_phentsize) != ehdr->e_phentsize){
      printf("Error reading program header\n");
      free(phdr);
      exit(EXIT_FAILURE);
    }
    if((phdr->p_type==PT_LOAD) && (ehdr->e_entry >=phdr->p_vaddr) && (ehdr->e_entry < (phdr->p_vaddr + phdr->p_memsz))){
      break;
    }
    n1++;
  }
  
  // if entry point is not found in the PT_LOAD segement
  if((phdr->p_type!=PT_LOAD) || (ehdr->e_entry <phdr->p_vaddr) || (ehdr->e_entry >= (phdr->p_vaddr + phdr->p_memsz))){
    printf("Error finding the entry point in the PT_LOAD segemant\n");
    exit(EXIT_FAILURE);
    }

  //    type that contains the address of the entrypoint method in fib.c
  // 3. Allocate memory of the size "p_memsz" using mmap function 

  // for allocating memory
  void *mem_allo;
  // address of entry point
  void *e_point;
  // variable to check if entry point is found or not
  int found_entry_point=0;
  mem_allo=mmap(NULL,phdr->p_memsz,PROT_READ | PROT_WRITE | PROT_EXEC,MAP_PRIVATE | MAP_ANONYMOUS,-1,0);


  //    and then copy the segment content
  // 4. Navigate to the entrypoint address into the segment loaded in the memory in above step

  // moving to offset to the starting of segement
  lseek(fd,phdr->p_offset,SEEK_SET);
  // reading the load segement data
  read(fd,mem_allo,phdr->p_filesz);
  // the entry point address
  e_point=(ehdr->e_entry - phdr->p_vaddr)+ mem_allo;

  // 5. Typecast the address to that of function pointer matching "_start" method in fib.c.

  // defining function pointer tupe
  typedef int(*func1)();
  // typecasting function
  func1 _start=(func1)e_point;


  // 6. Call the "_start" method and print the value returned from the "_start"
  int result = _start();
  printf("User _start return value = %d\n",result);

  free(b_file);

}



int main(int argc, char** argv) 
{
  if(argc != 2) {
    printf("Usage: %s <ELF Executable> \n",argv[0]);
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