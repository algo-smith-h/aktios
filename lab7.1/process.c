//process || gcc process.c -0 proces -lreadline
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>

#define MAX_ARGS 64
#define MAX_PATH_LEN 1024

char** parse_command(char* line, int* arg_count) {
    char** args = malloc(MAX_ARGS * sizeof(char*));
    if (!args) return NULL;
    
    *arg_count = 0;
    char* token = strtok(line, " \t\n");
    
    while (token != NULL && *arg_count < MAX_ARGS - 1) {
        args[(*arg_count)++] = strdup(token);
        token = strtok(NULL, " \t\n");
    }
    args[*arg_count] = NULL;
    
    return args;
}

void free_args(char** args, int count) {
    for (int i = 0; i < count; i++) {
        free(args[i]);
    }
    free(args);
}

int execute_builtin(char** args, int arg_count) {
    if (arg_count == 0) return 0;
    
    if (strcmp(args[0], "exit") == 0) {
        printf("EXIT.\n");
        exit(0);
    }
    else if (strcmp(args[0], "cd") == 0) {
        if (arg_count < 2) {
            fprintf(stderr, "cd: need argument\n");
        } else if (chdir(args[1]) != 0) {
            perror("cd");
        }
        return 1;
    }
    else if (strcmp(args[0], "pwd") == 0) {
        char cwd[MAX_PATH_LEN];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s\n", cwd);
        } else {
            perror("pwd");
        }
        return 1;
    }
    else if (strcmp(args[0], "help") == 0) {
        printf("=== Interpreter ===\n");
        printf("Built-in commands:\n");
        printf("  cd <dir>    - change directory\n");
        printf("  pwd         - print working directory\n");
        printf("  help        - show this help\n");
        printf("  exit        - exit\n");
        return 1;
    }
    
    return 0; 
}

void execute_external(char** args, int arg_count) {
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("Error: can`t make process");
        return;
    }
    
    if (pid == 0) {
        execvp(args[0], args);
        perror(args[0]);
        exit(EXIT_FAILURE);
    } 
    else { 
        int status;
        waitpid(pid, &status, 0);
        
        if (WIFEXITED(status)) {
            printf("Command code: %d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("Command signal: %d\n", WTERMSIG(status));
        }
    }
}

int main() {
    printf("=== Interpreter (version with process) ===\n");
    printf("Type 'exit' - to end program or 'help' - to get help notes\n");
    
    char* line;
    
    using_history();
    stifle_history(100); 
    
    while (1) {
        line = readline("cs> ");
        
        if (!line) {
            printf("\n");
            break;
        }
        
        if (strlen(line) == 0) {
            free(line);
            continue;
        }
        
        add_history(line);
        
        int arg_count;
        char** args = parse_command(line, &arg_count);
        
        if (arg_count > 0) {
            if (!execute_builtin(args, arg_count)) {
                execute_external(args, arg_count);
            }
        }
        
        free_args(args, arg_count);
        free(line);
    }
    
    return 0;
}
