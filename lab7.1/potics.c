//potics
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <readline/readline.h>
#include <readline/history.h>

#define MAX_ARGS 64
#define MAX_PATH_LEN 1024
#define MAX_THREADS 10

// Структура для передачи данных в поток
typedef struct {
    char** args;
    int arg_count;
    int thread_id;
} ThreadData;

// Мьютекс для синхронизации вывода
pthread_mutex_t output_mutex = PTHREAD_MUTEX_INITIALIZER;

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

// Функция, выполняемая в потоке
void* execute_in_thread(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    pid_t pid;
    int status;
    
    pthread_mutex_lock(&output_mutex);
    printf("[Поток %d] Запуск команды: ", data->thread_id);
    for (int i = 0; i < data->arg_count; i++) {
        printf("%s ", data->args[i]);
    }
    printf("\n");
    pthread_mutex_unlock(&output_mutex);
    
    // Создаем процесс для выполнения команды
    pid = fork();
    
    if (pid < 0) {
        pthread_mutex_lock(&output_mutex);
        perror("[Поток] Ошибка fork");
        pthread_mutex_unlock(&output_mutex);
        pthread_exit(NULL);
    }
    
    if (pid == 0) { // Дочерний процесс
        execvp(data->args[0], data->args);
        
        pthread_mutex_lock(&output_mutex);
        perror(data->args[0]);
        pthread_mutex_unlock(&output_mutex);
        exit(EXIT_FAILURE);
    } 
    else { // Родительский процесс в потоке
        waitpid(pid, &status, 0);
        
        pthread_mutex_lock(&output_mutex);
        if (WIFEXITED(status)) {
            printf("[Поток %d] Команда завершилась с кодом %d\n", 
                   data->thread_id, WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("[Поток %d] Команда завершена сигналом %d\n", 
                   data->thread_id, WTERMSIG(status));
        }
        pthread_mutex_unlock(&output_mutex);
    }
    
    pthread_exit(NULL);
}

// Встроенные команды (выполняются в основном потоке)
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
        printf("=== Интерпретатор команд (потоковая версия) ===\n");
        printf("Встроенные команды:\n");
        printf("  cd <dir>    - сменить директорию\n");
        printf("  pwd         - текущая директория\n");
        printf("  help        - эта справка\n");
        printf("  exit        - выход\n");
        printf("  threads     - показать активные потоки\n");
        printf("Любая другая команда выполняется в отдельном потоке\n");
        return 1;
    }
    else if (strcmp(args[0], "threads") == 0) {
        printf("[Основной поток] ID: %lu\n", pthread_self());
        printf("Активные потоки можно посмотреть в отдельном терминале:\n");
        printf("  ps -T -p %d\n", getpid());
        return 1;
    }
    
    return 0;
}

int main() {
    printf("=== Интерпретатор команд (версия с потоками) ===\n");
    printf("Введите 'help' для справки, 'exit' для выхода\n");
    
    char* line;
    pthread_t threads[MAX_THREADS];
    ThreadData* thread_data[MAX_THREADS];
    int thread_count = 0;
    
    // Настройка readline
    using_history();
    stifle_history(100);
    
    while (1) {
        line = readline("myshell-thread> ");
        
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
                // Создаем поток для выполнения команды
                if (thread_count < MAX_THREADS) {
                    // Подготавливаем данные для потока
                    thread_data[thread_count] = malloc(sizeof(ThreadData));
                    thread_data[thread_count]->args = args;
                    thread_data[thread_count]->arg_count = arg_count;
                    thread_data[thread_count]->thread_id = thread_count + 1;
                    
                    // Создаем поток
                    if (pthread_create(&threads[thread_count], NULL, 
                                       execute_in_thread, thread_data[thread_count]) != 0) {
                        perror("Ошибка создания потока");
                        free(thread_data[thread_count]);
                        free_args(args, arg_count);
                    } else {
                        printf("[Основной] Команда отправлена в поток %d\n", 
                               thread_count + 1);
                        
                        // Отсоединяем поток (не ждем завершения)
                        pthread_detach(threads[thread_count]);
                        thread_count++;
                        
                        // Не освобождаем args здесь - они будут освобождены в потоке
                        continue; // Пропускаем освобождение args
                    }
                } else {
                    printf("[Основной] Достигнут лимит потоков (%d)\n", MAX_THREADS);
                }
            }
        }
        
        free_args(args, arg_count);
        free(line);
    }
    
    // Освобождаем оставшиеся данные потоков
    for (int i = 0; i < thread_count; i++) {
        free(thread_data[i]);
    }
    
    pthread_mutex_destroy(&output_mutex);
    
    return 0;
}
