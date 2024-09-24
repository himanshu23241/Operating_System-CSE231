#include<stdio.h>
#include<string.h>
#include<sys/wait.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<time.h>
#include<stdbool.h>
#include<readline/readline.h>
#include<readline/history.h>
#include<signal.h>

 // clear the the screen after entering clear command
void clear_screen()
{
    system("clear");
}

 // show shell name and user name who are accessing the shell as a welcome screen
void launch_shell()
{   
    clear_screen();  // clear the screen after launching the Dam_Shell 
    printf("\n\n<---------Dam_Shell---------->\n\n");
    char* username=getenv("USER") ;  // to find username of current enviroment variable you are working
    if (username==NULL){
        perror("getenv");
        exit(1);
    }
    printf("USER :- @%s",username);
    printf("\n");

}


#define max_command_length 1000     // maximum length of a entered command
#define max_history_size 100       // maximum number of commands stored in history


 // declear some variables in a single name command detail using struct data types 
typedef struct command_detail
{
    char** command;
    int argu_count;
    int pid;
    long start_time;
    long end_time;
    long exec_time;
} command_detail;



command_detail*commands[max_history_size];     // creat an array commands to store command_details pointers
int command_counter=0;                         // keep track of how many commands have been executed and stored


 // function to execute a command with some parameters
int launch(char** argus, int argu_number, bool it_pipe, int size_history, char**command_history)
{
    int fd[2];                                 //  initilize the file descriptor for the pipe. f[0]=read end, f[1]=write end
    
    if(it_pipe) pipe(fd);                      //

    int start_time=clock();                    // record the start time of the command execution

 // Create a new process by forking the current process.
    int level=fork();                          

    if (level<0)                               // if fork() return a negative value then it shows that fork failed
    {
        printf("forking is failed\n");         
    }
    
    else if (level==0)                          // if fork returns 0 then it creat child process
    {
        if(strcmp("history", argus[0])==0){     // check if the command is the built-in "history" command
            if (size_history != 0){                         // If there are commands in the history iterate and print them
                for(int i=0; i<size_history; i++){
                    printf("%s",command_history[i]);
                }
            }
        }
        else{                                               // if the command is not "history" proceed to execute it
            if(it_pipe){                                    // if the command is part of a pipeline, set up the pipe for output
                close(fd[0]);                               // close the read end of the pipe in the child process
                dup2(fd[1], STDOUT_FILENO);                 // duplicate the write end of the pipe
            }
            if(execvp(argus[0], argus)<0){                  // execute the command using execvp
                perror("command not found");                // if command not found print error
            }
        }
        exit(1);   // exit the child process
    }
    else{   //parent process after a successful fork
        wait(NULL);  // wait for the child process to finish execution

        if (it_pipe){                       // if the command is part of a pipeline, set up the pipe for output
            close(fd[1]);                   // close the read end of the pipe in the child process
            dup2(fd[0],STDIN_FILENO);       // duplicate the write end of the pipe        
        }

        command_detail*detail=malloc(sizeof(command_detail));     // allocate memory to store command execution details

        if(detail==NULL){       // check malloc is success
            perror("malloc");
            exit(1);
        }

        detail->pid=level;                  // store the Process ID of the child process
        detail->end_time=clock();              // record the end time of the command execution
        detail->start_time=start_time;         // store the previously start time
        detail->exec_time=detail->end_time-detail->start_time;          // calculate the total execution time of the command
        detail->command=argus;                  // store the array of command arguments
        detail->argu_count=argu_number;         // store the number of arguments in the command

        commands[command_counter]=detail;       // add the command_detail struct to the commands history array
        command_counter++;          // increment the command_counter

        return level;       // return the PID of the child process
    }
    return 1;
}

 

// function to handle the SIGINT signal
void control_C_handler(int signum){
    if (signum==SIGINT){                                         // check the signal is SIGINT or not
        printf("\n");
        printf("\ncommand_history:\n");
        printf("PID\t\t command\t\t starttime\t exectime\n");
        for (int i = 0;  i<command_counter ; i++){               // Iterating Through Command History
            printf("%d\t\t", commands[i]-> pid);
            for(int j=0; j<commands[i] -> argu_count; j++){
                printf("%s",commands[i]->command[j]);
            }
            printf("\t\t%ld \t\t",commands[i]->start_time);      // print start time of ith command
            printf("\t\t%ld \t\t",commands[i]->exec_time);       // print execution time of ith comand
            printf("\n");
        }
        exit(1);
    }
}


// function to execute a single command
int execute(char*command, char**command_history, int size_history, bool it_pipe){
    signal(SIGINT, control_C_handler);

    int level=1;                                                  // initialize the status variable to 1 and this will store the execution status of the command

// splitting the command into individual arguments
    char**argus;
    int argu_number=0;

    char*token=strtok(command," ");                              // using strtok to tokenize the command string                       
    argus=malloc(sizeof(char*));                                 // allocate initial memory for the args array to hold command arguments and allocate memory for char* pointer
    if(argus==NULL){                                             // check if malloc is success If not print an error and exit
        perror("malloc");
        exit(1);
    }
    while (token!=NULL){                                         // process each argument extracted by strtok

        argus[argu_number]=strdup(token);                        // duplicate the token string and strdup allocates memory and copies the string
        if(argus[argu_number]==NULL){                            // check if strdup success If not print an error and exit
            perror("atrdup");
            exit(1);
        }
        argu_number++;                                           //  increment the argument count
        argus=realloc(argus,(argu_number+1)*sizeof(char*));      //  reallocate memory for the args array to accommodate the next argument
        if(argus==NULL){
            perror("realloc");                                   //  check if realloc success if not print error and exit 
            exit(1);
        }
        token=strtok(NULL," ");                                 // give the next token if no more tokens are found strtok returns NULL
    }
// remove the trailing newline character from the last argument 
    argus[argu_number-1][strcspn(argus[argu_number-1], "\n")] = '\0';   // replace '\n' with '\0' to terminate the string properly
    
    // Check for built-in shell commands to handle them internally.
     if(strcmp(argus[0], "exit") == 0) {                                // if the first argument is "exit", handle the shell exit procedure
        printf("\ncommand_history:\n");
        printf("PID\t\t command\t\t startTime\t exectime\n");
        for (int i = 0; i < command_counter; i++){                     // iterate through each recorded command in the history
            printf("%d \t\t",commands[i] -> pid);                      // print the Process ID (PID) of the i-th command
            for (int j = 0; j < commands[i] -> argu_count; j++){       // iterate through each argument of the i-th command
                printf("%s ",commands[i] -> command[j]);
            }
            printf("\t\t%ld \t\t",commands[i] -> start_time);           // print the start time of the command
            printf("\t\t%ld \t\t",commands[i] -> exec_time);            // Print the execution time of the command
            printf("\n");
        }
        exit(0);
        return 0;
    }
    else{                                                               // if the command is not a built-in command
        level=launch(argus, argu_number, it_pipe, size_history, command_history);      // Call the launch function to handle the execution of the command
    }
    return level;
}

// function for executing the command conating pipe
int execute_pipe(char* command, char** command_history, int size_history) {            
    int level=1;            // initilize the level to 1
    char** argus_pipe = NULL;   //  declare a pointer for array of string to store command seperated by '|'
    int pipe_number = 0;        //  initilize a variable for counting commands seperated '|'

    char* token = strtok(command, "|");      // tockenize the command seperated by '|'
    // loop for command sring, splitting at each '|' and store in argus_pipe 
    while (token != NULL) {
        argus_pipe = realloc(argus_pipe, (pipe_number+1) * sizeof(char*));          // reallocate memory to store one more command in the array
        argus_pipe[pipe_number] = strdup(token);            // Duplicate the current token and store it in the arg_pipe array
        pipe_number++;              // increment the pipe _number
        token = strtok(NULL, "|");   //  continue splitting the original command string at each '|'
    }
    for (int i = 0; i<pipe_number; i++) {    // iterate over each command part separated by '|' and execute them sepretly
        char* sub_line = argus_pipe[i];     // get the current command part
        while (*sub_line == ' ') {           // remove space in command part
            sub_line++;
        }
        int len = strlen(sub_line);         //  find the length of sub_line
        // remove the space by adjust the length
        while (len > 0 && sub_line[len - 1] == ' ') {
            len--;
        }
        sub_line[len] = '\0';            // string after removing the space
// if this is not the last command in the pipeline, pass the current command to `execute()` with the `is_pipe` flag set to true
        if (i < pipe_number - 1) {
            level = execute(sub_line, command_history, size_history, true);
            // for the last command in the pipeline, call `execute()` with the `is_pipe` flag set to false
        } else level = execute(sub_line, command_history, size_history, false);
        free(argus_pipe[i]);        // Free the memory allocated 
    }

    free(argus_pipe);                // Free the memory allocated 
    return level;
}

 // this function for execute command in background '&'
int execute_background_commands(char* command, char** command_history, int size_history) {
    
    int level=1;    //  set the level to 1
    char** argus_ampersand = NULL;        // array that will store each individual command split by the &.
    int ampersand_number = 0;             // count the number of command splited

    char* token = strtok(command, "&");  // split the command and tockenize it
    while (token != NULL) {              // continue till command are ended
        argus_ampersand = realloc(argus_ampersand, (ampersand_number+1) * sizeof(char*));   // resizes the arg_ampersand array to accommodate one more command string using realloc
        argus_ampersand[ampersand_number] = strdup(token);    // duplicate the current command and store it in array
        ampersand_number++;            
        token = strtok(NULL, "&");
    }
    // loop that runs through all the commands stored in arg_ampersand
    for (int i=0; i<ampersand_number; i++) {  
        char* sub_line = argus_ampersand[i];   // exteract the command from array for processing
        
        while (*sub_line == ' ') {      // remove leading space
            sub_line++;  
        }
        int len = strlen(sub_line);     // calculates the length of the removed command string.
        while (len > 0 && sub_line[len - 1] == ' ') {       // remove the spaces from the command by adjusting the length.
            len--;
        }
        sub_line[len] = '\0';  // terminate the string after removing
        level = execute_pipe(sub_line, command_history, size_history);      // calls execute_pip passing the remove command to execute it
        free(argus_ampersand[i]);    //  free the memory allocated for the command string
    }
    free(argus_ampersand);      // free the memory
    return level;
}

 
// function for reading a shell script file line by line
int shell_interpreter(char* shell_File, char** command_history, int size_history){
    FILE* file = fopen(shell_File, "r");                                            // Open the shell script file for reading
    if (file == NULL) {                                                             // check if the file was successfully opened or not
        printf("File not found\n");
        return 1;
    }

    char* line = NULL;                                                              // declare a pointer to hold each line from the file 
    size_t len = 0;                                                                 // declare variables for reading lines from the file
    ssize_t read;
// read the file line by line
    while ((read = getline(&line, &len, file)) != -1) {
        if (line[strlen(line) - 1] == '\n') line[strlen(line) - 1] = '\0';          // if the line ends with a newline character replace it with '\0'
        
        execute_background_commands(line, command_history, size_history);           // calls the execute_ampersand function to execute the command
    }
    // close the file after reading all the lines
    fclose(file);  
    // free the allocated memory for 'line' if it was allocated during the getline call
    if (line) free(line);
    return 1;
}


int main()
{
    launch_shell();  // call the launch function to show shell and user name

    int level;    // variable to store the status of the command execution
    char** command_history=NULL;   // array to store the history of commands entered
    int size_history=0;         // variable to track the size of the command history

// initlize the do loop 
    do{
        char*user = getenv("USER");    // get the current username from the environment
        printf("\n%s@Dam_Shell:~$ > ",user);        // print user name
        char command[max_command_length];       // command input of size COMLEN (1000)

        if(fgets(command,max_command_length,stdin)==NULL){      // fgets read a line of input from stdin into the 'command' array

            if (feof(stdin)) {                      // if end of input is reached continue to the next iteration of the loop
                level=1;        // set the level 1  to continue the loop
                
                if (freopen("/dev/tty", "r", stdin) == NULL) {          // if stdin has been closed this freopen() call reopens it
                    perror("freopen");
                    exit(1);
                }

            }
            else if (ferror(stdin)) {               // if some other error occurs during input
                level =1;                           // set the level 1  to continue the loop
                command[strcspn(command, "\n")] = '\0';    // remove newline from input
                printf("we found some error. please enter again.\n");   // Print an error message
            }
        }
        // if fgets() successfully reads the input
        else{
            
            command_history = realloc(command_history, (size_history + 1) * sizeof(char *));        // add the command to the command history
            command_history[size_history] = strdup(command);        // duplicate the command and store it in history
            size_history++;     // increment the history 

            // if the command starts with './' and ends with '.sh'
            if(command[0]=='.' && command[1]=='/' && command[strlen(command)-1]=='h' && command[strlen(command)-2]=='s'){
                level = shell_interpreter(command, command_history, size_history);   // call the shell_interpreter
            }
            else{         // execute the command for ampersand (&) and pipes (|)
                level = execute_background_commands(command,command_history,size_history);
                lseek(STDIN_FILENO, 0, SEEK_END);   // this moves the file offset of the stdin to the end
            }
        }
    }
    while(level);       // continue the loop while the status variable
return 0;
}









