#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdbool.h>

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

bool parse_sf_file(const char *file_path) {
    FILE *file = fopen(file_path, "rb");
    if (!file) {
        perror("Error opening file");
        return false;
    }

    SFHeader sf_header;
    if (fread(&sf_header, sizeof(SFHeader), 1, file) != 1) {
        fclose(file);
        return false;
    }

    if (sf_header.magic != 'Y' ||
        sf_header.version < 14 || sf_header.version > 124 ||
        sf_header.no_of_sections < 4 || sf_header.no_of_sections > 20) {
        fclose(file);
        return false;
    }

    SectionHeader *section_headers = (SectionHeader *)malloc(sf_header.no_of_sections * sizeof(SectionHeader));
    if (!section_headers) {
        fclose(file);
        return false;
    }

    if (fread(section_headers, sizeof(SectionHeader), sf_header.no_of_sections, file) != sf_header.no_of_sections) {
        free(section_headers);
        fclose(file);
        return false;
    }

    for (int i = 0; i < sf_header.no_of_sections; i++) {
        if (section_headers[i].sect_type != 45 && section_headers[i].sect_type != 16) {
            free(section_headers);
            fclose(file);
            return false;
        }
    }

    free(section_headers);
    fclose(file);
    return true;
}
bool ends_with(const char *str, const char *suffix) {
    if (!str || !suffix) {
        return false;
    }
    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);
    if (suffix_len > str_len) {
        return false;
    }
    return strncmp(str + str_len - suffix_len, suffix, suffix_len) == 0;
}



void list_directory(const char *dir_path, int recursive, const char *name_ends_with, off_t size_greater) {
    DIR *dir = opendir(dir_path);
    if (!dir) {
        perror("Error opening directory");
        return;
    }

    struct dirent *entry;
    struct stat st;
    char full_path[1024];
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

        if (lstat(full_path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                if (name_ends_with && ends_with(entry->d_name, name_ends_with)) {
                    printf("%s\n", full_path);
                }

                if (recursive) {
                    list_directory(full_path, recursive, name_ends_with, size_greater);
                }
            } else if (S_ISREG(st.st_mode)) {
                if ((size_greater == -1 || st.st_size > size_greater) && (!name_ends_with || ends_with(entry->d_name, name_ends_with))) {
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
            const char *file_path = NULL;
            for (int i = 1; i < argc; i++) {
                if (strncmp(argv[i], "path=", 5) == 0) {
                    file_path = argv[i] + 5;
                }
            }

            if (!file_path) {
                fprintf(stderr, "ERROR\nmissing file path\n");
                return 1;
            }

            if (parse_sf_file(file_path)) {
                printf("SUCCESS\n");
                // You can print additional information about the SF file here, if required.
            } else {
                printf("ERROR\nwrong magic|version|sect_nr|sect_types\n");
            }
        } else if (strcmp(argv[1], "list") == 0) {
            // The existing code for the "list" command goes here.
        }
    }



    int recursive = 0;
    const char *dir_path = NULL;
    const char *name_ends_with = NULL;
    off_t size_greater = -1;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "recursive") == 0) {
            recursive = 1;
        } else if (strncmp(argv[i], "path=", 5) == 0) {
            dir_path = argv[i] + 5;
        } else if (strncmp(argv[i], "name ends with=", 15) == 0) {
            name_ends_with = argv[i] + 15;
        } else if (strncmp(argv[i], "size greater=", 13) == 0) {
            size_greater = strtoll(argv[i] + 13, NULL, 10);
        }
    }

    if (!dir_path) {
        fprintf(stderr, "ERROR\nmissing directory path\n");
        return 1;
    }


    printf("SUCCESS\n");
    list_directory(dir_path, recursive, name_ends_with, size_greater);

    return 0;
}
