#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>


#define SF_MAGIC 'Y'
#define SF_MIN_VERSION 14
#define SF_MAX_VERSION 124
#define SF_MIN_SECT_NR 4
#define SF_MAX_SECT_NR 20
#define SF_SECT_TYPE_TEXT 4
#define SF_SECT_TYPE_DATA 16


typedef struct {
    char magic;
    uint16_t header_size;
    uint8_t version;
    uint8_t no_of_sections;
} SFHeader;


typedef struct {
    char sect_name[8];
    uint8_t sect_type;
    uint32_t sect_offset;
    uint32_t sect_size;
} SectionHeader;



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



void print_invalid_sf_file(const char* reason) {
    printf("ERROR\n%s\n", reason);
}

void print_sf_file(SFHeader* header, SectionHeader* sections) {
    printf("SUCCESS\nversion=%d\nnr_sections=%d\n", header->version, header->no_of_sections);
    for (int i = 0; i < header->no_of_sections; i++) {
        printf("section%d: %s %d %d\n", i+1, sections[i].sect_name, sections[i].sect_type, sections[i].sect_size);
    }
}
void parse(const char *file_path) {
    int fd = -1;
    fd = open(file_path, O_RDONLY);

    if (fd == -1) {
        perror("ERROR");
        return;
    }
    SFHeader header;
    ssize_t num_bytes_read = read(fd, &header, sizeof(header));
    if (num_bytes_read != sizeof(header)) {
        print_invalid_sf_file("failed to read SF header");
        close(fd);
        return;
    }
    lseek(fd, 0, SEEK_SET);
     read(fd, &header.magic, sizeof(header.magic));
     read(fd, &header.header_size, sizeof(header.header_size));
     read(fd, &header.version, sizeof(header.version));
    read(fd, &header.no_of_sections, sizeof(header.no_of_sections));

//    printf("%c\n",header.magic);
//    printf("%d\n",header.version);
//    printf("%d\n",header.no_of_sections);
//    printf("%d\n",header.header_size);



    if (header.magic != SF_MAGIC) {
        print_invalid_sf_file("wrong magic");
        close(fd);
        return;
    }

    if (header.version < SF_MIN_VERSION || header.version > SF_MAX_VERSION) {
        //printf("pricina: %d\n", header.version);
        print_invalid_sf_file("wrong version");
        close(fd);
        return;
    }

    if (header.no_of_sections < SF_MIN_SECT_NR || header.no_of_sections > SF_MAX_SECT_NR) {
        print_invalid_sf_file("wrong sect_nr");
        close(fd);
        return;
    }

    SectionHeader *section_headers = malloc(header.no_of_sections * sizeof(SectionHeader));
    if (!section_headers) {
        perror("Failed to allocate memory for section headers");
        close(fd);
        return;
    }
    //int var = sizeof(section_headers->sect_name) + sizeof (section_headers->sect_type) + sizeof(section_headers->sect_offset) + sizeof(section_headers->sect_size);
    num_bytes_read = read(fd, section_headers, header.no_of_sections * 16);
    if (num_bytes_read != header.no_of_sections * 16) {
        print_invalid_sf_file("failed to read section headers");
        free(section_headers);
        close(fd);
        return;
    }
    lseek(fd,-num_bytes_read, SEEK_CUR);
    for (int i = 0; i < header.no_of_sections; i++) {
        //vii cu citire in sect type

        ssize_t num_bytes_read = read(fd, section_headers[i].sect_name, 7/*sizeof(section_headers[i].sect_name)*/);
        section_headers[i].sect_name[7] = 0;
        if (num_bytes_read != 7) {
            print_invalid_sf_file("failed to read SECT_NAME field");
            free(section_headers);
            close(fd);
            return;
        }
        printf("NAME: %s \n",section_headers[i].sect_name);

        num_bytes_read = read(fd, &section_headers[i].sect_type, sizeof(section_headers[i].sect_type));
        if (num_bytes_read != sizeof(section_headers[i].sect_type)) {
            print_invalid_sf_file("failed to read SECT_TYPE field");
            free(section_headers);
            close(fd);
            return;
        }
        printf("TYPE: %d \n",section_headers[i].sect_type);


        if (lseek(fd, sizeof(section_headers[i].sect_offset), SEEK_CUR) == -1) {
            print_invalid_sf_file("failed to skip SECT_OFFSET field");
            free(section_headers);
            close(fd);
            return;
        }

        num_bytes_read = read(fd, &section_headers[i].sect_size, sizeof(section_headers[i].sect_size));
        if (num_bytes_read != sizeof(section_headers[i].sect_size)) {
            print_invalid_sf_file("failed to read SECT_SIZE field");
            free(section_headers);
            close(fd);
            return;
        }
        printf("%d \n",section_headers[i].sect_size);

        if (section_headers[i].sect_type != SF_SECT_TYPE_TEXT && section_headers[i].sect_type != SF_SECT_TYPE_DATA) {
            print_invalid_sf_file("wrong sect_type");
            free(section_headers);
            close(fd);
            return;
        }

        if (section_headers[i].sect_type != SF_SECT_TYPE_TEXT && section_headers[i].sect_type != SF_SECT_TYPE_DATA) {
            print_invalid_sf_file("wrong sect_type");
            free(section_headers);
            close(fd);
            return;
        }

    }
    //for

    //print_sf_file(&header, section_headers);

    free(section_headers);
    close(fd);
}




int main(int argc, char **argv) {

    char *dir_path1 = NULL;
    int recursive = 0;
    const char *dir_path = NULL;
    const char *name_ends_with = NULL;
    off_t size_greater = -1;
    const char *size_greater_suffix = NULL;


    if (argc >= 2) {
        if (strcmp(argv[1], "variant") == 0) {
            printf("88528\n");

        } else if (strcmp(argv[1], "parse") == 0) {
            for (int i = 2; i < argc; i++) {

                if (strncmp(argv[i], "path=", 5) == 0) {
                  //  printf("OK\n");
                    dir_path1 = argv[i] + 5;
                    dir_path1[strlen(argv[i])-5] = 0;
                   // printf("%s\n",argv[i]);
                }
                parse(dir_path1);


            }

        }else if (strcmp(argv[1], "list") == 0) {
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



        }

    }

    return 0;
}
