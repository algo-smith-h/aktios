//Дано название файла и директория, вывести размер файла в байтах
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

int main(int argc, char* argv[]){

    char *dir = argv[1];
    char *fil = argv[2];

    char full_path[strlen(dir) + strlen(fil) + 2];

    sprintf(full_path, "%s/%s", dir, fil);

    struct stat file_stat;
    stat(full_path, &file_stat);

    printf("%ld\n", file_stat.st_size);

    return 0;
}
