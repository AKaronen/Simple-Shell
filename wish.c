#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void shell();
char *read();
char **parse(char *input);


int main(int argc, char* argv[]){
 

    shell();




    return 0;
}






void shell(){
    char* input = "input";
    while(strcmp(input, "exit") == 0){
        printf("wish> ");
        scanf("%s",input);
    }

}




char *read(){

}



char **parse(char *input){

}




void exec(char **args){

}