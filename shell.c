#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <fcntl.h>




void parser(char *input, char *argv[]){ //Aquí está la función para parsear el input del usuario
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
                j++;
                word = false;
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

    char *user = getenv("USER");
    char *cwd = getcwd(NULL, 0);
    char host[HOST_NAME_MAX + 1];
    gethostname(host, sizeof(host));
    printf("\x1B[1;36m%s@%s\x1B[0m:%s$ ", user, host, cwd);
    free(cwd);
}


int main(int argc, char **argv)
{
    char input[255];
  
    puts("You are now using \x1B[1;36mhf-shell\x1B[0m made by \x1B[1;36mHFMaker\x1B[0m");
    

    while (1) //El bucle principal de la Shell
    {
        printPrompt();
        fflush(stdout);
        if (!fgets(input, MAX_INPUT, stdin)) break;
        
    input[strcspn(input, "\n")] = 0;

    if (strlen(input) == 0) continue;

    char *argv[MAX_INPUT]; //El buffer qe guarda el input del usuario

    parser(input, argv); //Aquí se parsea el input del usuario

    //Debajo tenemos algunos comandos butilt-in y una función que abre una terminal nueva
     
    if (strcmp(argv[0], "cd") == 0){ 
         if (!argv[1]){
            chdir(getenv("HOME"));
         } else{
             if (chdir(argv[1]) != 0){
                perror("cd");
            }
         }
        continue;
        
    }

    
    if (strcmp(argv[0], "exit") == 0){
        exit(0);
    }

    if (strcmp(argv[0], "openshell") == 0){ //el comando que abre la nueva terminal es este (creo que su nombre lo dice todo)
        pid_t pid = fork();

        if (pid == 0)
        {
            char *term = getenv("TERMINAL");

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
            execvp(argv[0], argv);
            perror("execvp");
            exit(1);
        }
    

        wait(NULL);
        continue;
        
    }
    bool pipe_on_input = false; //Aquí comprobamos si el input del usuario tiene alguna pipe
    int i;
    int pipe_index;
    for (i = 0; argv[i] != NULL; i++){
        if (strcmp(argv[i], "|") == 0){
            pipe_on_input = true;
            pipe_index = i; 
            argv[i] = NULL;
        }

    

    }

    if (pipe_on_input == true){ //Y aquí ejecutamos el comando del usuario si este tiene una pipe
            int fd[2];
            if (pipe(fd) == -1){
                printf("Error while doing the pipe");
                return 0;
            }
            pid_t p1 = fork(); //Primero tenemos el primero proceso (el que se encuentra a la izquierda de la pipe)

            if (p1 == 0){

                dup2(fd[1], STDOUT_FILENO);
                close(fd[0]);
                close(fd[1]);

                execvp(argv[0], argv);
                perror("execvp left");
                exit(1);
            }

            pid_t p2 = fork(); //Y luego tenemos el segundo proceso, que está al derecha de la pipe

            if (p2 == 0){
                dup2(fd[0], STDIN_FILENO);
                close(fd[0]);
                close(fd[1]);
                execvp(argv[pipe_index + 1], argv + pipe_index + 1);
                perror("execvp right");
                exit(1);

            }

            close(fd[0]);
            close(fd[1]);
            wait(NULL);
            wait(NULL);
            continue;
        }

    int redirection_type = 0; //En esta parte, comprobamos si el input del user tiene alguna redirección
    int j;
    int redirection_index;
    for (j = 0; argv[j] != NULL; j++){
         if (strcmp(argv[j], ">") == 0){
            redirection_type = 1;
            redirection_index = j;
            argv[j] = NULL;
         }

         else if (strcmp(argv[j], ">>") == 0){
            redirection_type = 2;
            redirection_index = j;
            argv[j] = NULL;
         }

         else if (strcmp(argv[j], "<") == 0){
            redirection_type = 3;
            redirection_index = j;
            argv[j] = NULL;
         }
    }

    if (redirection_type != 0){// Y aquí ejecutamos el comando del usuario si este tiene alguna redirección
        pid_t pid = fork();
        if (pid == 0){
            int fd_archivo;
            if (redirection_type == 1){
                fd_archivo = open(argv[redirection_index + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            }

            else if (redirection_type == 2){
                fd_archivo = open(argv[redirection_index + 1], O_WRONLY | O_CREAT | O_APPEND, 0644);
            }
            
            else if (redirection_type == 3){
                fd_archivo = open(argv[redirection_index + 1], O_RDONLY, 0644);
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
            execvp(argv[0], argv);
        }
        
        wait(NULL);
        continue;
    }

    
    pid_t pid = fork();// Si el input del usuario no tiene ninguna pipe o redirección, entonces se ejecuta de manera normal

    if (pid == 0){
        execvp(argv[0], argv);
        printf("hf-shell: %s: command not found\n", argv[0]);
        exit(1);
    }
    else wait(NULL);
    
    }
return 0; 
}



