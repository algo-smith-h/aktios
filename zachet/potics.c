//Создать поток, который создаёт ещё один поток,
//который выводит на консоль hello world
#include <stdio.h>
#include <pthread.h>

void* print_hello(void* arg){
    printf("Hello world\n");
    return NULL;
}

void* create_printer(void* arg){
    pthread_t pth_2;
    pthread_create(&pth_2, NULL, print_hello, NULL);
    pthread_join(pth_2, NULL);
    return NULL;
}

int main(){

    pthread_t pth_1;

    pthread_create(&pth_1, NULL, create_printer, NULL);
    pthread_join(pth_1, NULL);

    return 0;
}
