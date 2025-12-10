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

typedef struct {
    char** args;
    int arg_count;
    int thread_id;
} ThreadData;

pthread_mutex_t output_mutex = PTHREAD_MUTEX_INITIALIZER;

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

void* execute_in_thread(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    pid_t pid;
    int status;
    
    pthread_mutex_lock(&output_mutex);
    printf("[Thread %d] Start command: ", data->thread_id);
    for (int i = 0; i < data->arg_count; i++) {
        printf("%s ", data->args[i]);
    }
    printf("\n");
    pthread_mutex_unlock(&output_mutex);

    pid = fork();
    
    if (pid == 0) { 
        execvp(data->args[0], data->args);
        
        pthread_mutex_lock(&output_mutex);
        perror(data->args[0]);
        pthread_mutex_unlock(&output_mutex);
        exit(EXIT_FAILURE);
    } 
    else { 
        waitpid(pid, &status, 0);
        
        pthread_mutex_lock(&output_mutex);
        if (WIFEXITED(status)) {
            printf("[Thread %d] Command code: %d\n", 
                   data->thread_id, WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("[Thread %d] Command signal: %d\n", 
                   data->thread_id, WTERMSIG(status));
        }
        pthread_mutex_unlock(&output_mutex);
    }
    
    pthread_exit(NULL);
}

int execute_builtin(char** args, int arg_count) {
    if (arg_count == 0) return 0;
    
    if (strcmp(args[0], "exit") == 0) {
        printf("END.\n");
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
        printf("  threads     - show active threads\n");
        return 1;
    }
    else if (strcmp(args[0], "threads") == 0) {
        printf("[Main thread] ID: %lu\n", pthread_self());
        printf("To show active threads type in other termenal:\n");
        printf("  ps -T -p %d\n", getpid());
        return 1;
    }
    
    return 0;
}

int main() {
    printf("=== Interpreter (version with threads) ===\n");
    printf("Type 'exit' - to end program or 'help' - to get help notes\n");
    
    char* line;
    pthread_t threads[MAX_THREADS];
    ThreadData* thread_data[MAX_THREADS];
    int thread_count = 0;
    
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
                if (thread_count < MAX_THREADS) {
                    thread_data[thread_count] = malloc(sizeof(ThreadData));
                    thread_data[thread_count]->args = args;
                    thread_data[thread_count]->arg_count = arg_count;
                    thread_data[thread_count]->thread_id = thread_count + 1;
  
                    pthread_create(&threads[thread_count], NULL, 
                                       execute_in_thread, thread_data[thread_count]); 
                    printf("[Main] command in thread %d\n", 
                            thread_count + 1);
                    pthread_detach(threads[thread_count]);
                    thread_count++;
                    
                    continue; 
                } else {
                    printf("[Main] Limit of thread (%d)\n", MAX_THREADS);
                }
            }
        }
        
        free_args(args, arg_count);
        free(line);
    }
    
    for (int i = 0; i < thread_count; i++) {
        free(thread_data[i]);
    }
    
    pthread_mutex_destroy(&output_mutex);
    
    return 0;
}
