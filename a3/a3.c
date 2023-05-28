#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>

#define REQ_PIPE_NAME "REQ_PIPE_88528"
#define RESP_PIPE_NAME "RESP_PIPE_88528"
#define MAX_SIZE 256
#define VARIANT_VALUE 88528
#define NUM_THREADS 4
#define QUIT "QUIT"
#define VARIANT "VARIANT"

#define SF_MAGIC 'Y'
#define SF_MIN_VERSION 14
#define SF_MAX_VERSION 124
#define SF_MIN_SECT_NR 4
#define SF_MAX_SECT_NR 20
#define SF_SECT_TYPE_TEXT 45
#define SF_SECT_TYPE_DATA 16

#define SHM_NAME "/wdrCDJS1"
#define SHM_PERMISSIONS 0664
#define CREATE_SHM "CREATE_SHM"
#define SUCCESS "SUCCESS"
#define ERROR "ERROR"
#define SECTION_SIZE 256

char buffer[SECTION_SIZE];


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

typedef struct {
    int request_pipe;
    int response_pipe;
} Pipes;

int shm_fd;
void* shm_ptr;
pthread_t thread_ids[NUM_THREADS];
void* file_mapped;


void parse(void* file_mapped, off_t size) {
    SFHeader* sfHeader = (SFHeader*)file_mapped;

    if (sfHeader->magic != SF_MAGIC) {
        printf("ERROR: Wrong magic\n");
        return;
    }

    if (sfHeader->version < SF_MIN_VERSION || sfHeader->version > SF_MAX_VERSION) {
        printf("ERROR: Wrong version\n");
        return;
    }

    if (sfHeader->no_of_sections < SF_MIN_SECT_NR || sfHeader->no_of_sections > SF_MAX_SECT_NR) {
        printf("ERROR: Wrong number of sections\n");
        return;
    }

    SectionHeader* section_headers = (SectionHeader*)(file_mapped + sizeof(SFHeader));

    for (int i = 0; i < sfHeader->no_of_sections; i++) {
        SectionHeader* sh = &section_headers[i];

        sh->sect_name[sizeof(sh->sect_name) - 1] = '\0';

        if (sh->sect_type != SF_SECT_TYPE_TEXT &&
            sh->sect_type != SF_SECT_TYPE_DATA) {
            printf("ERROR: Wrong section types\n");
            printf("section types: %d\n", sh->sect_type);
            return;
        }
    }
}

int map_file(const char* filename) {
  
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file for reading");
        return -1;
    }


    struct stat st;
    fstat(fd, &st);
    off_t size = st.st_size;


    void* file_mapped = mmap(0, size, PROT_READ, MAP_SHARED, fd, 0);
    if (file_mapped == MAP_FAILED) {
        close(fd);
        perror("Error mmapping the file");
        return -1;
    }

    close(fd);


    parse(file_mapped, size);

   
    munmap(file_mapped, size);

    return 0;
}


int read_from_file_offset(off_t offset, size_t size) {
    if (file_mapped == NULL) {
        printf("File is not mapped yet\n");
        return -1;
    }

   
    if (size > SECTION_SIZE) {
        printf("Size is greater than buffer size\n");
        return -1;
    }


    memcpy(buffer, file_mapped + offset, size);
    return 0;
}

int read_from_file_section(int section_no, off_t offset, size_t size) {

    off_t section_offset = section_no * SECTION_SIZE;

    return read_from_file_offset(section_offset + offset, size);
}

int read_from_logical_space_offset(off_t logical_offset, size_t size) {

    return read_from_file_offset(logical_offset, size);
}

void* handle_request_new(void* arg) {
    Pipes* pipes = (Pipes*) arg;
    int request_pipe = pipes->request_pipe;
    int response_pipe = pipes->response_pipe;
    char buffer[MAX_SIZE];
    char response[MAX_SIZE];
    shm_ptr = NULL;
    shm_fd = -1;
    while (1) {
        memset(buffer, 0, MAX_SIZE);
        ssize_t readSize = read(request_pipe, buffer, MAX_SIZE);
        if (readSize < 0) {
            perror("Error reading from the request pipe");
            break;
        } else if (readSize == 0) {
            break;
        }

        if (strcmp(buffer, "ping") == 0) {
            snprintf(response, MAX_SIZE, "ping %d", VARIANT_VALUE);
        } else if (strcmp(buffer, VARIANT) == 0) {

            snprintf(response, MAX_SIZE, "%s %s %d", VARIANT, "VALUE", VARIANT_VALUE);
        } else {
          
            snprintf(response, MAX_SIZE, "%s", buffer);
        }

        if (write(response_pipe, response, strlen(response)) == -1) {
            perror("Error writing to the response pipe");
            break;
        }

        if (strncmp(buffer, CREATE_SHM, strlen(CREATE_SHM)) == 0) {
      
            int size = atoi(buffer + strlen(CREATE_SHM));
            shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, SHM_PERMISSIONS);
            if (write(response_pipe, response, strlen(response)) == -1) {
                perror("Error writing to the response pipe");
                break;
            }
            if (shm_fd == -1) {
                snprintf(response, MAX_SIZE, "%s %s", CREATE_SHM, ERROR);
            } else {
                if (ftruncate(shm_fd, size) == -1) {
                    snprintf(response, MAX_SIZE, "%s %s", CREATE_SHM, ERROR);
                } else {
                    shm_ptr = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
                    if (shm_ptr == MAP_FAILED) {
                        snprintf(response, MAX_SIZE, "%s %s", CREATE_SHM, ERROR);
                    } else {
                        snprintf(response, MAX_SIZE, "%s %s", CREATE_SHM, SUCCESS);
                        break; 
                    }

                    if (strncmp(buffer, "MAP_FILE", strlen("MAP_FILE")) == 0) {
                        char* filename = buffer + strlen("MAP_FILE") + 1;
                        int result = map_file(filename);
                        snprintf(response, MAX_SIZE, "MAP_FILE %s", result ? "SUCCESS" : "ERROR");
                    } else if (strncmp(buffer, "READ_FROM_FILE_OFFSET", strlen("READ_FROM_FILE_OFFSET")) == 0) {
                        off_t offset;
                        size_t size;
                        sscanf(buffer, "READ_FROM_FILE_OFFSET %ld %zu", &offset, &size);
                        int result = read_from_file_offset(offset, size);
                        snprintf(response, MAX_SIZE, "READ_FROM_FILE_OFFSET %s", result ? "SUCCESS" : "ERROR");
                    } else if (strncmp(buffer, "READ_FROM_FILE_SECTION", strlen("READ_FROM_FILE_SECTION")) == 0) {
                        int section_no;
                        off_t offset;
                        size_t size;
                        sscanf(buffer, "READ_FROM_FILE_SECTION %d %ld %zu", &section_no, &offset, &size);
                        int result = read_from_file_section(section_no, offset, size);
                        snprintf(response, MAX_SIZE, "READ_FROM_FILE_SECTION %s", result ? "SUCCESS" : "ERROR");
                    } else if (strncmp(buffer, "READ_FROM_LOGICAL_SPACE_OFFSET", strlen("READ_FROM_LOGICAL_SPACE_OFFSET")) == 0) {
                        off_t logical_offset;
                        size_t size;
                        sscanf(buffer, "READ_FROM_LOGICAL_SPACE_OFFSET %ld %zu", &logical_offset, &size);
                        int result = read_from_logical_space_offset(logical_offset, size);
                        snprintf(response, MAX_SIZE, "READ_FROM_LOGICAL_SPACE_OFFSET %s", result ? "SUCCESS" : "ERROR");
                    } else if (strcmp(buffer, "EXIT") == 0) {
                        break;
                    }


                }
            }
        }
    }

    return NULL;
}


int main() {
    int request_pipe;

    if (mkfifo(RESP_PIPE_NAME, 0666) == -1) {
        perror("ERROR\nCannot create the response pipe");
        printf("ERROR\ncannot create the response pipe\n");
        return 1;
    }

    request_pipe = open(REQ_PIPE_NAME, O_RDONLY);
    if (request_pipe == -1) {
        perror("ERROR\ncannot open the request pipe");
        printf("ERROR\ncannot open the request pipe\n");
        unlink(RESP_PIPE_NAME);
        return 1;
    }

    int response_pipe = open(RESP_PIPE_NAME, O_WRONLY);
    if (response_pipe == -1) {
        perror("ERROR\ncannot open the response pipe");
        printf("ERROR\ncannot open the response pipe\n");
        close(request_pipe);
        unlink(RESP_PIPE_NAME);
        return 1;
    }

    char* message = "START!";
    if (write(response_pipe, message, strlen(message)) == -1) {
        perror("Error writing to the response pipe");
        printf("ERROR\n");
        close(request_pipe);
        close(response_pipe);
        unlink(RESP_PIPE_NAME);
        return 1;
    }

    printf("SUCCESS\n");

    for (int i = 0; i < NUM_THREADS; i++) {
        if (pthread_join(thread_ids[i], NULL) != 0) {
            perror("Error joining thread");
            return 1;
        }
    }

    if (unlink(RESP_PIPE_NAME) == -1) {
        perror("Error unlinking the response pipe");
        return 1;
    }

    if (shm_ptr != NULL) {
        munmap(shm_ptr, 0);
    }

    if (shm_fd != -1) {
        close(shm_fd);
    }
    close(request_pipe);
    close(response_pipe);

    if (file_mapped != NULL) {
        munmap(file_mapped, 0);
    }

    if (shm_fd != -1) {
        close(shm_fd);
    }
    close(request_pipe);
    close(response_pipe);

    return 0;
}
