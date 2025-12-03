#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>

struct Statistics {
    unsigned long dir_count;
    unsigned long file_count;
    int found_count;
};

void print_time(time_t t) {
    struct tm *tm_info = localtime(&t);
    char buffer[26];
    strftime(buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    printf("%s", buffer);
}

void print_permissions(mode_t mode) {
    printf("%c%c%c%c%c%c%c%c%c",
           (mode & S_IRUSR) ? 'r' : '-',
           (mode & S_IWUSR) ? 'w' : '-',
           (mode & S_IXUSR) ? 'x' : '-',
           (mode & S_IRGRP) ? 'r' : '-',
           (mode & S_IWGRP) ? 'w' : '-',
           (mode & S_IXGRP) ? 'x' : '-',
           (mode & S_IROTH) ? 'r' : '-',
           (mode & S_IWOTH) ? 'w' : '-',
           (mode & S_IXOTH) ? 'x' : '-');
}

void find_file(const char *dir_path, const char *target_file, struct Statistics *stats) {
    DIR *dir;
    struct dirent *entry;
    struct stat stat_buf;
    char full_path[1024];

    dir = opendir(dir_path);
    stats->dir_count++;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
        lstat(full_path, &stat_buf);

        if (S_ISDIR(stat_buf.st_mode)) {
            find_file(full_path, target_file, stats);
        } else if (S_ISREG(stat_buf.st_mode) || S_ISLNK(stat_buf.st_mode)) {
            stats->file_count++;
            
            if (strcmp(entry->d_name, target_file) == 0) {
                stats->found_count++;
                
                printf("\n========================================\n");
                printf("Найден файл: %s\n", entry->d_name);
                printf("Полный путь: %s\n", full_path);
                printf("Размер: %ld байт\n", stat_buf.st_size);
                printf("Дата создания: ");
                print_time(stat_buf.st_ctime);
                printf("\n");
                printf("Права доступа: ");
                print_permissions(stat_buf.st_mode);
                printf(" (%04o)\n", stat_buf.st_mode & 0777);
                printf("Номер индексного дескриптора: %ld\n", stat_buf.st_ino);
                printf("Тип файла: ");
                if (S_ISREG(stat_buf.st_mode)) printf("Обычный файл\n");
                if (S_ISLNK(stat_buf.st_mode)) printf("Символическая ссылка\n");
                printf("Владелец: UID=%d, GID=%d\n", stat_buf.st_uid, stat_buf.st_gid);
                printf("Дата последнего доступа: ");
                print_time(stat_buf.st_atime);
                printf("\n");
                printf("Дата последней модификации: ");
                print_time(stat_buf.st_mtime);
                printf("\n");
                printf("Количество жестких ссылок: %ld\n", stat_buf.st_nlink);
                printf("========================================\n");
            }
        }
    }

    closedir(dir);
}

int main(int argc, char *argv[]) {
    struct Statistics stats = {0, 0, 0};
    
    printf("Поиск файла '%s' в каталоге '%s'...\n", argv[2], argv[1]);
    printf("================================================================\n");

    find_file(argv[1], argv[2], &stats);

    printf("\n\nИТОГИ ПОИСКА:\n");
    printf("================================================================\n");
    printf("Найдено файлов с именем '%s': %d\n", argv[2], stats.found_count);
    printf("Просмотрено каталогов: %lu\n", stats.dir_count);
    printf("Просмотрено файлов: %lu\n", stats.file_count);
    printf("================================================================\n");

    return 0;
}
