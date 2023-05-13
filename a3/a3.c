#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <stdint.h>
#include <dirent.h>

#define REQ_PIPE_NAME "REQ PIPE 88528"
#define RESP_PIPE_NAME "RESP PIPE 88528"
#define SHM_NAME "/wdrCDJS1"

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

void parse_sf_file(const char* file_path, void* shm_addr, size_t shm_size) {
    // Open the file
    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        perror("ERROR\nCannot open the SF file");
        return;
    }

    // Get the size of the file
    struct stat st;
    if (fstat(fd, &st) == -1) {
        perror("ERROR\nCannot get the size of the SF file");
        close(fd);
        return;
    }

    if (st.st_size > shm_size) {
        printf("ERROR\nThe SF file is larger than the shared memory size\n");
        close(fd);
        return;
    }

    // Read the file into the shared memory
    ssize_t bytesRead = read(fd, shm_addr, st.st_size);
    if (bytesRead == -1) {
        perror("ERROR\nFailed to read the SF file");
    } else if (bytesRead < st.st_size) {
        printf("WARNING\nIncomplete read of the SF file\n");
    }

    close(fd);
}

int main() {
    int req_pipe, resp_pipe;

    if (mkfifo(RESP_PIPE_NAME, 0666) == -1) {
        perror("ERROR\nCannot create the response pipe");
        return 1;
    }

    if ((req_pipe = open(REQ_PIPE_NAME, O_RDONLY)) == -1) {
        perror("ERROR\nCannot open the request pipe");
        return 1;
    }

    if ((resp_pipe = open(RESP_PIPE_NAME, O_WRONLY)) == -1) {
        perror("ERROR\nCannot open the response pipe");
        return 1;
    }

    const char start_msg[] = "START!";
    ssize_t bytesWritten = write(resp_pipe, start_msg, strlen(start_msg));
    if (bytesWritten == -1) {
        perror("ERROR\nCannot write to the response pipe");
        return 1;
    } else if (bytesWritten < strlen(start_msg)) {
        printf("WARNING\nIncomplete write to the response pipe\n");
    }

    printf("SUCCESS\n");

    while (1) {
        char req_name[251];
        unsigned int size;

        ssize_t bytesRead;
        int i = 0;
        do {
            bytesRead = read(req_pipe, &req_name[i], 1);
            if (bytesRead == -1) {
                perror("ERROR\nFailed to read from request pipe");
                break;
            }
        } while (req_name[i++] != '!' && bytesRead > 0);

        if (bytesRead <= 0) {
            if (bytesRead == -1) {
                perror("ERROR\nFailed to read from request pipe");
            }
            break;
        }

        req_name[i - 1] = '\0';

        if (strcmp(req_name, "VARIANT") == 0) {
            char resp_msg[] = "VARIANT!VALUE!88528!";
            ssize_t bytesWritten = write(resp_pipe, resp_msg, strlen(resp_msg));
            if (bytesWritten == -1) {
                perror("ERROR\nFailed to write to response pipe");
                break;
            } else if (bytesWritten < strlen(resp_msg)) {
                printf("WARNING\nIncomplete write to the response pipe\n");
                break;
            }
        } else if (strcmp(req_name, "CREATE_SHM") == 0) {
            ssize_t bytesRead = read(req_pipe, &size, sizeof(unsigned int));
            if (bytesRead != sizeof(unsigned int)) {
                perror("ERROR\nFailed to read from request pipe");
                break;
            }

            int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0664);
            if (shm_fd == -1) {
                char err_msg[] = "CREATE_SHM!ERROR!";
                ssize_t bytesWritten = write(resp_pipe, err_msg, strlen(err_msg));
                if (bytesWritten == -1) {
                    perror("ERROR\nFailed to write to response pipe");
                }
                break;
            }

            if (ftruncate(shm_fd, size) == -1) {
                char err_msg[] = "CREATE_SHM!ERROR!";
                ssize_t bytesWritten = write(resp_pipe, err_msg, strlen(err_msg));
                if (bytesWritten == -1) {
                    perror("ERROR\nFailed to write to response pipe");
                }
                close(shm_fd);
                shm_unlink(SHM_NAME);
                break;
            }

            void *addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
            if (addr == MAP_FAILED) {
                char err_msg[] = "CREATE_SHM!ERROR!";
                ssize_t bytesWritten = write(resp_pipe, err_msg, strlen(err_msg));
                if (bytesWritten == -1) {
                    perror("ERROR\nFailed to write to response pipe");
                }
                close(shm_fd);
                shm_unlink(SHM_NAME);
                break;
            }

            char success_msg[] = "CREATE_SHM!SUCCESS!";
            ssize_t bytesWritten = write(resp_pipe, success_msg, strlen(success_msg));
            if (bytesWritten == -1) {
                perror("ERROR\nFailed to write to response pipe");
                break;
            } else if (bytesWritten < strlen(success_msg)) {
                printf("WARNING\nIncomplete write to the response pipe\n");
                break;
            }
        } else if (strcmp(req_name, "PARSE_SF") == 0) {
            // Handle PARSE_SF request
            // First, read the file path from the pipe
            char file_path[251];
            ssize_t bytesRead;
            int i = 0;
            do {
                bytesRead = read(req_pipe, &file_path[i], 1);
                if (bytesRead == -1) {
                    perror("ERROR\nFailed to read from request pipe");
                    break;
                }
            } while (file_path[i++] != '!' && bytesRead > 0);

            if (bytesRead <= 0) {
                if (bytesRead == -1) {
                    perror("ERROR\nFailed to read from request pipe");
                }
                break;
            }

            file_path[i - 1] = '\0';  // Replace '!' with '\0' to make it a proper C string

            // Now parse the SF file
            parse_sf_file(file_path, readdir, size);

            char success_msg[] = "PARSE_SF!SUCCESS!";
            ssize_t bytesWritten = write(resp_pipe, success_msg, strlen(success_msg));
            if (bytesWritten == -1) {
                perror("ERROR\nFailed to write to response pipe");
                break;
            } else if (bytesWritten < strlen(success_msg)) {
                printf("WARNING\nIncomplete write to the response pipe\n");
                break;
            }
        }
    }

    close(req_pipe);
    close(resp_pipe);
    unlink(RESP_PIPE_NAME);

    return 0;
}
