//process
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

// Разбиение строки на аргументы
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

// Освобождение памяти аргументов
void free_args(char** args, int count) {
    for (int i = 0; i < count; i++) {
        free(args[i]);
    }
    free(args);
}

// Встроенные команды shell
int execute_builtin(char** args, int arg_count) {
    if (arg_count == 0) return 0;
    
    if (strcmp(args[0], "exit") == 0) {
        printf("Выход из интерпретатора команд\n");
        exit(0);
    }
    else if (strcmp(args[0], "cd") == 0) {
        if (arg_count < 2) {
            fprintf(stderr, "cd: ожидается аргумент\n");
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
        printf("=== Интерпретатор команд ===\n");
        printf("Встроенные команды:\n");
        printf("  cd <dir>    - сменить директорию\n");
        printf("  pwd         - текущая директория\n");
        printf("  help        - эта справка\n");
        printf("  exit        - выход\n");
        printf("Любая другая команда будет выполнена через exec()\n");
        printf("Примеры: ls -la, ps aux, echo 'Hello'\n");
        return 1;
    }
    
    return 0; // Не встроенная команда
}

// Выполнение внешней команды
void execute_external(char** args, int arg_count) {
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("Ошибка при создании процесса");
        return;
    }
    
    if (pid == 0) { // Дочерний процесс
        // Пробуем выполнить команду
        execvp(args[0], args);
        
        // Если execvp вернул управление - ошибка
        perror(args[0]);
        exit(EXIT_FAILURE);
    } 
    else { // Родительский процесс
        int status;
        waitpid(pid, &status, 0);
        
        if (WIFEXITED(status)) {
            printf("Команда завершилась с кодом %d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("Команда завершена сигналом %d\n", WTERMSIG(status));
        }
    }
}

int main() {
    printf("=== Интерпретатор команд (версия с процессами) ===\n");
    printf("Введите 'help' для справки, 'exit' для выхода\n");
    
    char* line;
    
    // Настройка readline
    using_history();
    stifle_history(100); // Ограничиваем историю
    
    while (1) {
        // Чтение строки с поддержкой истории
        line = readline("myshell> ");
        
        if (!line) {
            printf("\n");
            break;
        }
        
        // Пропускаем пустые строки
        if (strlen(line) == 0) {
            free(line);
            continue;
        }
        
        // Добавляем в историю
        add_history(line);
        
        // Разбор команды
        int arg_count;
        char** args = parse_command(line, &arg_count);
        
        if (arg_count > 0) {
            // Проверяем встроенные команды
            if (!execute_builtin(args, arg_count)) {
                // Выполняем внешнюю команду
                execute_external(args, arg_count);
            }
        }
        
        // Очистка
        free_args(args, arg_count);
        free(line);
    }
    
    return 0;
}
