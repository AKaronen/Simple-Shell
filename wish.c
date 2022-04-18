#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

// Let's define different kind of error messages
#define std_error "An error has occurred\n"
#define file_error "Cannot open file\n"
#define proc_error "Process could not be executed\n"
#define malloc_error "Error allocating memory\n"
#define usage "usage: ./wish [batch-file]\n"
#define cd_error "usage: cd 'path'\n"
#define chdir_error "Couldn't change directory\n"

// Introducing all the functions
void shell();
char *read_input();
char **parse_input(char *input);
int execute_input(char **args);
int builtIn_exit(char **args);
int builtIn_cd(char **args);
int builtIn_path(char **args);
int runProcess(char *path,char **args);
int run_cmd(char **args);



int main(int argc, char *argv[]){
    // standard PATH
    putenv("PATH=/bin");
    // Start the shell
    if(argc == 1){
        // if user wants to run the shell in interactive mode
        shell(0);
    }
    else if(argc == 2){
        // if user want to run the shell in batch mode
        // Changing the input pipe for users file
        FILE* fileInput = fopen(argv[1], "r");
        if (fileInput == NULL){
            write(STDERR_FILENO, file_error, strlen(file_error));
            exit(1);
        }
        dup2(fileno(fileInput), STDIN_FILENO);
        shell(1);
        fclose(fileInput);

    }else{
        write(STDERR_FILENO, usage, strlen(usage));
        exit(1);
    }
    

    return 0;
}

void shell(int mode){
    // running the shell
    int s = 1;
    // interactive mode
    if(mode == 0){
        while (s){ // printing wish>  until user writes exit
            printf("wish> ");
            char *input = read_input(); // reading users input
            int save = dup(1); // saving the original output stream if user wants results to other output stream
            char **args = parse_input(input); // parseing users input (finding the system call to execute)
            s = execute_input(args); // executing the system call
            // changing output steam back to stdout if changed
            dup2(save,1);
            close(save); // closing the output stream
        }
    
    }// Batch-mode
    if(mode == 1){
        while (s){
            char *input = read_input(); // reading users input
            char **args = parse_input(input); // parseing users input file (one command per line)
            s = execute_input(args); // executing commands in the file
        }
    }
 
}

// function to read users input
char *read_input(){
    char* input = NULL;
    ssize_t size;

    if(getline(&input,&size,stdin) == -1) { // reading the input until end of file
        if(feof(stdin)) {
            exit(0);
        }else{
            write(STDERR_FILENO, std_error, strlen(std_error)); // if couldn't open the file or some other error occurs
            exit(1);
        }
    }

    return input;
}

// function to parse users input
char **parse_input(char *input){

    int bufs = 64, i = 0; // initialize buffersize and index
    char *token;
    char **list = malloc(bufs*sizeof(char*));
    char *delims = " \t\r\n\a"; // characters that divide the input

    if(!list){ // if could not allocare memory
        write(STDERR_FILENO, malloc_error, strlen(malloc_error));
        exit(1);
    }

    // getting the first token, do the token is not null
    token = strtok(input, delims);

    // getting the rest of the tokens
    while(token != NULL){

        // if user wants the output to some other output stream, finding '>' character
        if(strcmp(token, ">") == 0){
            token = strtok(NULL, delims);
            // Changing the output and error pipe
            FILE* fileOutput = fopen(token, "a+");
            if (fileOutput == NULL){ // if file couldn't be opened
                write(STDERR_FILENO, file_error, strlen(file_error));
                exit(1);
            }

            dup2(fileno(fileOutput), STDOUT_FILENO);
            dup2(fileno(fileOutput), STDERR_FILENO);
            // returning the list containing the command user wants to run
            list[i] = NULL; //add a NULL to the list for future termination
            return list;
        }

        // if the character is not '>', append tokens to list so we will have a full command user wants to execute
        else{
            list[i] = token;
            i++;
            // if the list grows too big, reallocate some more memory for it
            if(i >= bufs){
                bufs += 64;
                list = realloc(list, bufs*sizeof(char*));
                if(!list){ // if couldn't allocate memory
                    write(STDERR_FILENO, malloc_error, strlen(malloc_error));
                    exit(1);
                } 
            }
            token = strtok(NULL, delims);
        }
        
    }
    list[i] = NULL; //add a NULL to the list for future termination
    return list; // returning the list containing the command user wants to run

}

// function where to check if user has changed the standard path
int run_cmd(char **args){
    int status;
    int bufs = 1000;
    

    char *path = malloc(bufs*sizeof(char));

    // if user has not changed the path and it is default '/bin'
    if(strcmp(getenv("PATH"), "/bin") == 0){
        strcat(path,getenv("PATH"));
        strcat(path, "/");
        strcat(path, args[0]);
        if(runProcess(path, args) == 0){
            write(STDERR_FILENO, proc_error, strlen(proc_error));
        }
    }
    // if user has changed the path or appended multiple paths
    else{
        path = strtok(getenv("PATH"),":"); // different paths separeted with :
        while(path != NULL && status != 1){ // going paths trought and if one of them can execute the command, stop
            strcat(path, "/");
            strcat(path, args[0]);
            if((status = runProcess(path, args)) == 0){ // if non of the paths have the command, give error message
                write(STDERR_FILENO, proc_error, strlen(proc_error));
            }
            path = strtok(NULL, ":");
        }
    }

    return 1;
}

// function to actually execute the process
int runProcess(char *path, char **args){
    pid_t pid, wpid;
    int status;
    pid = fork(); // creating a child process
    if (pid == 0){
        if (execv(path, args) == -1){
            // error child process
            return 0;
        }
    }
    else if (pid < 0){
        // Error forking
        write(STDERR_FILENO, std_error, strlen(std_error));
    }
    else{
        do{ // waiting until the process is done
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
    return 1;
}

// checking if users input is one of the built-in functions
int execute_input(char **args){
    if (args[0] == NULL){
        return 1;
    }
    if(strcmp(args[0], "exit") == 0){
        return builtIn_exit(args); //run exit
    }
    if(strcmp(args[0], "cd") == 0){
        return builtIn_cd(args); //run cd
    }
    if(strcmp(args[0], "path") == 0){
        return builtIn_path(args); //run path
    }
    // if input was not any of the built-in functions, execute the command
    return run_cmd(args);
}

// BUILT-IN COMMANDS

int builtIn_exit(char **args){
    return 0; //exits shell when called
}

int builtIn_cd(char **args){
    if (args[1] == NULL){ // if no parameters were given to cd
        write(STDERR_FILENO, cd_error, strlen(cd_error));
    }
    else{
        if (chdir(args[1]) != 0){ // if couldn't open the directory
            write(STDERR_FILENO, chdir_error, strlen(chdir_error));
        }
    }

    return 1;
}

int builtIn_path(char **args){   
    char paths[1000] = "";
    int i = 1;
    if(args[1] == NULL){ // if no path were given
        setenv("PATH", "", 1);
    }
    else{ // appending all paths user gave
        while(args[i] != NULL){
            strcat(paths,args[i]);
            if(args[i+1] != NULL){
                strcat(paths,":");
            }
            i++;
        }
        // Appending them to environmental variable path
        setenv("PATH", paths, 1);
    
    }
    return 1;
}


