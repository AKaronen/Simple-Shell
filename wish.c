#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define std_error "An error has occurred\n"
#define file_error "Cannot open file\n"
#define proc_error "Process could not be executed\n"
#define malloc_error "Error allocating memory\n"
#define usage "usage: ./wish [batch-file]\n"
#define cd_error "usage: cd 'path'\n"
#define chdir_error "Couldn't change directory\n"

void shell();
char *read_input();
char **parse_input(char *input);
int execute_input(char **args);
int builtIn_exit(char **args);
int builtIn_cd(char **args);
int builtIn_path(char **args);
int runProcess(char *path,char **args);
int run_cmd(char **args);

int main(int argc, char *argv[])
{
    // standard PATH
    putenv("PATH=/bin");
    // Start the shell
    if(argc == 1){
        shell(0);
    }
    else if(argc == 2){
            // Changing the input pipe
            FILE* fileInput = fopen(argv[1], "r");
            if (fileInput == NULL)
            {
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

void shell(int mode)
{
    // running the shell
    int s = 1;
    // interactive mode
    if(mode == 0){
        while (s){
            printf("wish> ");
            char *input = read_input();
            int save = dup(1);
            char **args = parse_input(input);
            s = execute_input(args);
            dup2(save,1);
            close(save);
        }
    
    }// Batch-mode
    if(mode == 1){
        while (s){
            char *input = read_input();
            char **args = parse_input(input);
            s = execute_input(args);
        }
    }
 
}

char *read_input(){
    char* input = NULL;
    ssize_t size;

    if(getline(&input,&size,stdin) == -1) {
        if(feof(stdin)) {
            exit(0);
        }else{
            write(STDERR_FILENO, std_error, strlen(std_error));
            exit(1);
        }
    }

    return input;
}

char **parse_input(char *input){

    int bufs = 64, i = 0; // initialize buffersize and index
    char *token;
    char **list = malloc(bufs*sizeof(char*));
    char *delims = " \t\r\n\a";

    if(!list){
        write(STDERR_FILENO, malloc_error, strlen(malloc_error));
        exit(1);
    }

    // getting the first token

    token = strtok(input, delims);

    // getting the rest of the tokens

    while(token != NULL){

        if(strcmp(token, ">") == 0){
            token = strtok(NULL, delims);
            // Changing the output and error pipe
            FILE* fileOutput = fopen(token, "a+");
            if (fileOutput == NULL)
            {
                write(STDERR_FILENO, file_error, strlen(file_error));
                exit(1);
            }

            dup2(fileno(fileOutput), STDOUT_FILENO);
            dup2(fileno(fileOutput), STDERR_FILENO);
            list[i] = NULL;
            return list;
        }

        
        else{
            list[i] = token;
            i++;

            // if the list grows too big, reallocate some more memory for it
            if(i >= bufs){
                bufs += 64;
                list = realloc(list, bufs*sizeof(char*));
                if(!list){
                    write(STDERR_FILENO, malloc_error, strlen(malloc_error));
                    exit(1);
                } 
            }
            token = strtok(NULL, delims);
        }
        
    }
    list[i] = NULL; //add a NULL to the list for future termination
    return list;

}

int run_cmd(char **args)
{
    int status;
    int bufs = 1000;
    

    char *path = malloc(bufs*sizeof(char));

    if(strcmp(getenv("PATH"), "/bin") == 0){
        strcat(path,getenv("PATH"));
        strcat(path, "/");
        strcat(path, args[0]);
        if(runProcess(path, args) == 0){
            write(STDERR_FILENO, proc_error, strlen(proc_error));
        }

    }else{
        path = strtok(getenv("PATH"),":");
        while(path != NULL && status != 1){
            strcat(path, "/");
            strcat(path, args[0]);
            if((status = runProcess(path, args)) == 0){
                write(STDERR_FILENO, proc_error, strlen(proc_error));
            }
            path = strtok(NULL, ":");
        }
    }

    return 1;
}



int runProcess(char *path, char **args){
    pid_t pid, wpid;
    int status;
    pid = fork();
    if (pid == 0)
    {
        if (execv(path, args) == -1)
        {
            // error child process
            return 0;
        }
    }
    else if (pid < 0)
    {
        // Error forking
        write(STDERR_FILENO, std_error, strlen(std_error));
    }
    else
    {
        do
        {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
    return 1;
}



int execute_input(char **args)
{


    if (args[0] == NULL)
    {
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

    return run_cmd(args);
}

// Built ins

int builtIn_exit(char **args)
{
    return 0; //exits shell when called
}

int builtIn_cd(char **args)
{

    if (args[1] == NULL)
    {
        write(STDERR_FILENO, cd_error, strlen(cd_error));
    }
    else
    {
        if (chdir(args[1]) != 0)
        {
            write(STDERR_FILENO, chdir_error, strlen(chdir_error));
        }
    }

    return 1;
}



int builtIn_path(char **args)
{   
    char paths[1000] = "";
    int i = 1;
    if(args[1] == NULL){
        setenv("PATH", "", 1);
    }else{
        while(args[i] != NULL){
            strcat(paths,args[i]);
            if(args[i+1] != NULL){
                strcat(paths,":");
            }
            i++;
        }
        setenv("PATH", paths, 1);
    
    }
    return 1;
}


