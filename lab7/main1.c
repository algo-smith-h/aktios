//processes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_ARGS 32
#define MAX_LINE 256

// Разбор строки на аргументы
char** parse_args(char* line) {
    static char* args[MAX_ARGS];
    int i = 0;
    char* token = strtok(line, " \t\n");
    
    while (token && i < MAX_ARGS-1) {
        args[i++] = token;
        token = strtok(NULL, " \t\n");
    }
    args[i] = NULL;
    return args;
}

// Проверка и выполнение встроенных команд
int builtin_cmd(char** args) {
    if (!args[0]) return 0;
    
    if (strcmp(args[0], "cd") == 0) {
        chdir(args[1] ? args[1] : getenv("HOME"));
        return 1;
    }
    
    if (strcmp(args[0], "exit") == 0) {
        exit(args[1] ? atoi(args[1]) : 0);
    }
    
    if (strcmp(args[0], "echo") == 0) {
        for (int i = 1; args[i]; i++) 
            printf("%s ", args[i]);
        printf("\n");
        return 1;
    }
    
    return 0;
}

// Выполнение внешней команды в процессе
void exec_external(char** args) {
    pid_t pid = fork();
    
    if (pid == -1) {
        perror("fork");
        return;
    }
    
    if (pid == 0) { // Дочерний процесс
        execvp(args[0], args);
        fprintf(stderr, "Error: command '%s' not found\n", args[0]);
        exit(EXIT_FAILURE);
    } 
    else { // Родительский процесс
        wait(NULL);
    }
}

// Основной цикл интерпретатора
void shell_loop() {
    char line[MAX_LINE];
    
    printf("Process Shell (type 'exit' to quit)\n");
    printf("==================================\n");
    
    while (1) {
        printf("psh> ");
        fflush(stdout);
        
        if (!fgets(line, sizeof(line), stdin)) 
            break;
            
        if (strlen(line) <= 1) 
            continue;
            
        line[strcspn(line, "\n")] = 0;
        
        char** args = parse_args(line);
        
        if (!builtin_cmd(args)) {
            exec_external(args);
        }
    }
}

int main() {
    shell_loop();
    return 0;
}
