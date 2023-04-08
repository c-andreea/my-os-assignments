#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>

typedef struct {
    char magic;
    uint16_t header_size;
    uint8_t version;
    uint8_t no_of_sections;
} SFHeader;


typedef struct {
    char sect_name[7];
    uint8_t sect_type;
    uint32_t sect_offset;
    uint32_t sect_size;
} SectionHeader;

typedef struct {
    SFHeader header;
    SectionHeader *section_headers;
} SFFile;

bool ends_with(const char *str, const char *suffix) {
    if (!str || !suffix) {
        return false;
    }
    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);
    if (suffix_len > str_len) {
        return false;
    }
    return strcasecmp(str + str_len - suffix_len, suffix) == 0;
}

bool size_greater_than(off_t size, off_t threshold) {
    return size > threshold;
}

void list_directory(const char *dir_path, int recursive, const char *name_ends_with, off_t size_greater, const char *size_greater_suffix) {
    DIR *dir = opendir(dir_path);
    if (!dir) {
        perror("Error opening directory");
        return;
    }

    struct dirent *entry;
    struct stat st;
    char full_path[1024];
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

            if (lstat(full_path, &st) == 0) {
                if (S_ISDIR(st.st_mode)) {


                    if (recursive) {
                        list_directory(full_path, recursive, name_ends_with, size_greater, size_greater_suffix);
                    }

                    if(size_greater==-1){


                        if (name_ends_with != NULL && !ends_with(full_path, name_ends_with)) {
                            continue;
                        }

                        if (size_greater_suffix && !ends_with(full_path, size_greater_suffix)) {
                            continue;
                        }
                        
                        printf("%s\n",full_path);
                    }
                } else if (S_ISREG(st.st_mode)) {
                    if (name_ends_with != NULL && !ends_with(full_path, name_ends_with)) {
                        continue;
                    }

                    if (size_greater_suffix && !ends_with(full_path, size_greater_suffix)) {
                        continue;
                    }

                    if (size_greater != -1 && !size_greater_than(st.st_size, size_greater)) {
                        continue;
                    }

                    printf("%s\n", full_path);
                }
            }
        }
    }

    closedir(dir);
}


int main(int argc, char **argv) {
    if (argc >= 2) {
        if (strcmp(argv[1], "variant") == 0) {
            printf("88528\n");
        } else if (strcmp(argv[1], "parse") == 0) {

            for (int i = 1; i < argc; i++) {
                if (strncmp(argv[i], "path=", 5) == 0) {

                }
            }
        }}
    int recursive = 0;
    const char *dir_path = NULL;
    const char *name_ends_with = NULL;
    off_t size_greater = -1;
    const char *size_greater_suffix = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "recursive") == 0) {
            recursive = 1;
        } else if (strncmp(argv[i], "path=", 5) == 0) {
            dir_path = argv[i] + 5;
        } else if (strncmp(argv[i], "name_ends_with=", 15) == 0) {
            name_ends_with= argv[i] + 15;
        } else if (strncmp(argv[i], "size_greater=", 13) == 0) {
            size_greater = strtoll(argv[i] + 13, NULL, 10);
        } else if (strncmp(argv[i], "size_greater_", 14) == 0) {
            size_greater_suffix = argv[i] + 14;
        }
    }

    if (!dir_path) {
        fprintf(stderr, "ERROR\nmissing directory path\n");
        return 1;
    }


    printf("SUCCESS\n");
    list_directory(dir_path, recursive, name_ends_with, size_greater, size_greater_suffix);

    return 0;
}
