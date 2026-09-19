#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <fcntl.h>

#define PIPELINE_FLAG (1 << 0) // -> 0001
#define REDIRECTION_FLAG (1 << 1) // -> 0010



unsigned int flags = ~(PIPELINE_FLAG | REDIRECTION_FLAG);
// 0000 if all flags are deactivated, 0011 if all flags are activated

int pipeIndex;


void parser(char *input, char *argv[]){ //Parse the user's input
        int i;
        int j = 0;
        bool word = false;
        for (i = 0; input[i] != '\0'; i++){
            if (input[i] == ' '){
                input[i] = '\0';
                word = false;
                continue;
            }

            if (input[i] == '|'){
                argv[j] = "|";
                word = false;
		flags |= PIPELINE_FLAG;
                flags |= PIPELINE_FLAG;
                pipeIndex = j;
                j++;
                continue;

            }

            if (input[i] == '>' && input[i+1] == '>'){
                argv[j] = ">>";
                j++;
                word = false;
                i++; 
                continue;
            }

            if (input[i] == '>'){
                argv[j] = ">";
                j++;
                word = false;
                continue;
            }

            if (input[i] == '<'){
                argv[j] = "<";
                j++;
                word = false;
                continue;
            }

            if (input[i] != ' ' && word == false){
                    argv[j] = &input[i];
                    j++;

            if (input[i] != ' ')
                word = true;
            }
        }
    argv[j] = NULL;
}

void printPrompt(void){
	
    char buffer[256];
    char *user = getenv("USER");
    snprintf(buffer, sizeof(buffer), "/home/%s", user);
    char *cwd = getcwd(NULL, 0);
    char host[HOST_NAME_MAX + 1];
    gethostname(host, sizeof(host));
    strcmp(cwd, buffer) == 0 ? printf("\x1B[1;36m%s@%s\x1B[0m:~$ ", user, host)
    : printf("\x1B[1;36m%s@%s\x1B[0m:%s$ ", user, host, cwd);
    free(cwd);
}


int main(int argc, char **argv){

    char input[255];
  
    puts("You are now using \x1B[1;36mhf-shell\x1B[0m made by \x1B[1;36mHFMaker\x1B[0m");

        

    while (1) //El bucle principal de la Shell
    {
	if (flags & PIPELINE_FLAG) puts("Pipeline flag is ON");
        printPrompt();
        fflush(stdout);
        if (!fgets(input, MAX_INPUT, stdin)) break;
    while (1){ //Main loop of the program
    
    printPrompt();
    fflush(stdout);
    if (!fgets(input, MAX_INPUT, stdin)) break;
        
    input[strcspn(input, "\n")] = 0;

    if (strlen(input) == 0) continue;

    char *args[MAX_INPUT]; //User's input is stored in this buffer

    parser(input, args);

    //We can find some built-in commands and a command that opens a new shell down below
     
    if (strcmp(args[0], "cd") == 0){ 
         if (!args[1]){
            chdir(getenv("HOME")); //Get the user's home directory
         } else{
             if (chdir(args[1]) != 0){
                perror("cd");
            }
         }
        continue;
        
    }

    
    if (strcmp(args[0], "exit") == 0){
        exit(0);
    }

    if (strcmp(args[0], "openshell") == 0){ 
        pid_t pid = fork();

        if (pid == 0)
        {
            char *term = getenv("TERMINAL"); //Get the user's default terminal

            if (term){
                execlp(term, term, "-e", "./hf-shell", NULL);
                execlp(term, term, "--", "./hf-shell", NULL);
            }

            execlp("x-terminal-emulator", "x-terminal-emulator", "-e", "./hf-shell", NULL);
            execlp("kitty", "kitty", "-e", "./hf-shell", NULL);
            execlp("konsole", "konsole", "-e", "./shell", NULL);
            execlp("gnome-terminal", "gnome-terminal", "--", "./hf-shell", NULL);
            execlp("xfce4-terminal", "xfce4-terminal", "-e", "./hf-shell", NULL);
            execlp("alacritty", "alacritty", "-e", "./hf-shell", NULL);
            execlp("mate-terminal", "mate-terminal", "-e", "./hf-shell", NULL);
            execlp("xterm", "xterm", "-e", "./hf-shell", NULL);
            perror("Compatible terminal not found");
            exit(1);
            execvp(args[0], args);
            perror("execvp");
            exit(1);
        }
    

        wait(NULL);
        continue;
        
    }
   
    if (flags & PIPELINE_FLAG){ //Y aquí ejecutamos el comando del usuario si este tiene una pipe
            int fd[2];
            if (pipe(fd) == -1){
                printf("Error while doing the pipe");
                return 0;
            }

            args[pipeIndex] = NULL;

            pid_t p1 = fork(); //first fork for the left child

            if (p1 == 0){

                dup2(fd[1], STDOUT_FILENO);
                close(fd[0]);
                close(fd[1]);

                execvp(args[0], args);
                perror("execvp");
                exit(1);
            }

            pid_t p2 = fork(); //second fork for the right child

            if (p2 == 0){
                dup2(fd[0], STDIN_FILENO);
                close(fd[0]);
                close(fd[1]);
                execvp(args[pipeIndex + 1], args + pipeIndex + 1);
                perror("execvp");
                exit(1);

            }

            close(fd[0]);
            close(fd[1]);
            wait(NULL);
            wait(NULL);
            flags &= ~PIPELINE_FLAG;
            continue;
        }

    int redirection_type = 0; //Check if the user's input has any redirection
    int j;
    int redirection_index;
    for (j = 0; args[j] != NULL; j++){
         if (strcmp(args[j], ">") == 0){
            redirection_type = 1;
            redirection_index = j;
            args[j] = NULL;
         }

         else if (strcmp(args[j], ">>") == 0){
            redirection_type = 2;
            redirection_index = j;
            args[j] = NULL;
         }

         else if (strcmp(args[j], "<") == 0){
            redirection_type = 3;
            redirection_index = j;
            args[j] = NULL;
         }
    }

    if (redirection_type != 0){//Execute the user's input of the code has any redirection
        pid_t pid = fork();
        if (pid == 0){
            int fd_archivo;
            if (redirection_type == 1){
                fd_archivo = open(args[redirection_index + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            }

            else if (redirection_type == 2){
                fd_archivo = open(args[redirection_index + 1], O_WRONLY | O_CREAT | O_APPEND, 0644);
            }
            
            else if (redirection_type == 3){
                fd_archivo = open(args[redirection_index + 1], O_RDONLY, 0644);
            }
            
            
            if (fd_archivo == -1){
                perror("open");
                exit(1);
            }
            if (redirection_type == 1 || redirection_type == 2){
                dup2(fd_archivo, STDOUT_FILENO);
            }
            else if (redirection_type == 3){
                dup2(fd_archivo, STDIN_FILENO);
            }
            if (close(fd_archivo) == -1){   
                perror("close");
                exit(1);
            }
            execvp(args[0], args);
        }
        
        wait(NULL);
        continue;
    }

    
    pid_t pid = fork();//Execute the user's input if there's no pipeline or redirection

    if (pid == 0){
        execvp(args[0], args);
        printf("hf-shell: %s: command not found\n", args[0]);
        exit(1);
    }
    else wait(NULL);
    
    }
return 0; 
}
}


