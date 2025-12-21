//Вывести имя файла (например, file в каталоге test)
//(char* name = basename(full)) include libgen
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    
    const char *directory = argv[1];
    DIR *dir = opendir(directory);
   
    struct dirent *entry;
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || 
            strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        printf("%s\n", entry->d_name);
        break;  
    }
    
    closedir(dir);
    
    return 0;
}
