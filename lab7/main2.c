//potics
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_ARGS 32
#define MAX_LINE 256

// Структура для передачи данных в поток
typedef struct {
    char** args;
} ThreadData;

// Разбор строки на аргументы
char** parse_args(char* line) {
    static char* args[MAX_ARGS];
    int i = 0;
    char* token = strtok(line, " \t\n");
    
    while (token && i < MAX_ARGS-1) {
        args[i++] = strdup(token); // Копируем строку для потока
        token = strtok(NULL, " \t\n");
    }
    args[i] = NULL;
    return args;
}

// Встроенные команды
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

// Функция, выполняемая в потоке
void* thread_execute(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    char** args = data->args;
    
    printf("[Thread %lu] Executing: ", pthread_self());
    for (int i = 0; args[i]; i++) 
        printf("%s ", args[i]);
    printf("\n");
    
    // В потоке мы НЕ можем использовать exec() напрямую,
    // поэтому создаем отдельный процесс
    pid_t pid = fork();
    
    if (pid == 0) { // Дочерний процесс
        execvp(args[0], args);
        fprintf(stderr, "Error: command '%s' not found\n", args[0]);
        exit(EXIT_FAILURE);
    } 
    else if (pid > 0) { // Родительский процесс
        wait(NULL);
    }
    
    // Освобождаем память
    for (int i = 0; args[i]; i++) 
        free(args[i]);
    free(data);
    
    return NULL;
}

// Основной цикл интерпретатора с потоками
void shell_loop_threaded() {
    char line[MAX_LINE];
    
    printf("Threaded Shell (limited - uses fork in threads)\n");
    printf("===============================================\n");
    
    while (1) {
        printf("tsh> ");
        fflush(stdout);
        
        if (!fgets(line, sizeof(line), stdin)) 
            break;
            
        if (strlen(line) <= 1) 
            continue;
            
        line[strcspn(line, "\n")] = 0;
        
        char** args = parse_args(line);
        
        if (builtin_cmd(args)) {
            // Освобождаем память для встроенных команд
            for (int i = 0; args[i]; i++) 
                free(args[i]);
            continue;
        }
        
        // Создаем поток для выполнения команды
        ThreadData* data = malloc(sizeof(ThreadData));
        data->args = args;
        
        pthread_t thread;
        pthread_create(&thread, NULL, thread_execute, data);
        pthread_detach(thread); // Отсоединяем поток
    }
}

int main() {
    shell_loop_threaded();
    return 0;
}
