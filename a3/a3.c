#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>
#include <malloc.h>

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

void* handle_request_new(void* arg) {
    Pipes* pipes = (Pipes*) arg;
    int request_pipe = pipes->request_pipe;
    int response_pipe = pipes->response_pipe;
    char buffer[MAX_SIZE];
    char response[MAX_SIZE];

    while (1) {
        memset(buffer, 0, MAX_SIZE);
        ssize_t readSize = read(request_pipe, buffer, MAX_SIZE);
        if (readSize < 0) {
            perror("Error reading from the request pipe");
            close(request_pipe);
            close(response_pipe);
            break;
        } else if (readSize == 0) {
            break;
        }

        // If the command is "ping", respond with "ping" and the variant number
        if (strcmp(buffer, "ping") == 0) {
            snprintf(response, MAX_SIZE, "ping %d", VARIANT_VALUE);
        } else {
            // for now, just echo the request back as the response
            snprintf(response, MAX_SIZE, "%s", buffer);
        }

        if (write(response_pipe, response, strlen(response)) == -1) {
            perror("Error writing to the response pipe");
            close(request_pipe);
            close(response_pipe);
            break;
        }
    }

    return NULL;
}

void parse(const char *file_path) {
    int fd = -1;
    fd = open(file_path, O_RDONLY);

    SFHeader sfHeader;

    ssize_t num_bytes_read = read(fd, &sfHeader, sizeof(SFHeader));

    if (num_bytes_read != sizeof(SFHeader)) {
        printf("ERROR: Failed to read header");
        close(fd);
        return;
    }

    lseek(fd, 0, SEEK_SET);

    read(fd, &(sfHeader.magic), 1);
    if (sfHeader.magic != SF_MAGIC) {
        printf("ERROR\nwrong magic\n");
        close(fd);
        return;
    }

    read(fd, &(sfHeader.header_size), sizeof(sfHeader.header_size));
    read(fd, &(sfHeader.version), sizeof(sfHeader.version));
    read(fd, &(sfHeader.no_of_sections), sizeof(sfHeader.no_of_sections));

    if (sfHeader.version < SF_MIN_VERSION || sfHeader.version > SF_MAX_VERSION) {
        printf("ERROR\nwrong version\n");
        close(fd);
        return;
    }

    if (sfHeader.no_of_sections < SF_MIN_SECT_NR || sfHeader.no_of_sections > SF_MAX_SECT_NR) {
        printf("ERROR\nwrong sect_nr\n");
        close(fd);
        return;
    }

    SectionHeader *section_headers = malloc(sfHeader.no_of_sections * sizeof(SectionHeader));
    if (!section_headers) {
        perror("ERROR: Failed to allocate memory for section headers");
        close(fd);
        return;
    }

    for (int i = 0; i < sfHeader.no_of_sections; i++) {
        read(fd, section_headers[i].sect_name, sizeof(section_headers[i].sect_name) - 1);
        section_headers[i].sect_name[sizeof(section_headers[i].sect_name) - 1] = '\0';

        read(fd, &(section_headers[i].sect_type), sizeof(section_headers[i].sect_type));
        if (section_headers[i].sect_type != SF_SECT_TYPE_TEXT &&
            section_headers[i].sect_type != SF_SECT_TYPE_DATA) {
            printf("ERROR\nwrong sect_types\n");
            printf("sect_types: %d\n", section_headers[i].sect_type);
            close(fd);
            free(section_headers);
            return;
        }

        read(fd, &(section_headers[i].sect_offset), sizeof(section_headers[i].sect_offset));
        read(fd, &(section_headers[i].sect_size), sizeof(section_headers[i].sect_size));
    }

    // print_sf_file(&sfHeader, section_headers);

    free(section_headers);
    close(fd);
}



int main() {
    int request_pipe;

    if (mkfifo(RESP_PIPE_NAME, 0666) == -1) {
        perror("ERROR\nCannot create the response pipe");
        return 1;
    }

    request_pipe = open(REQ_PIPE_NAME, O_RDONLY);
    if (request_pipe == -1) {
        perror("ERROR\ncannot open the request pipe");
        unlink(RESP_PIPE_NAME);
        return 1;
    }

    int response_pipe = open(RESP_PIPE_NAME, O_WRONLY);
    if (response_pipe == -1) {
        perror("ERROR\ncannot open the response pipe");
        close(request_pipe);
        unlink(RESP_PIPE_NAME);
        return 1;
    }

    // write "START" to response pipe
    char *message = "START!";
    if (write(response_pipe, message, strlen(message)) == -1) {
        perror("Error writing to the response pipe");
        close(request_pipe);
        close(response_pipe);
        unlink(RESP_PIPE_NAME);
        return 1;
    }

    printf("SUCCESS\n");

    pthread_t thread_ids[NUM_THREADS];
    Pipes pipes = {request_pipe, response_pipe};

    for (int i = 0; i < NUM_THREADS; i++) {
        if (pthread_create(&thread_ids[i], NULL, handle_request_new, &pipes) != 0) {
            perror("Error creating thread");
            close(request_pipe);
            close(response_pipe);
            unlink(RESP_PIPE_NAME);
            return 1;
        }
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        if (pthread_join(thread_ids[i], NULL) != 0) {
            perror("Error joining thread");
            return 1;
        }
    }

    // close the pipes and unlink the response pipe after all threads have finished
    close(request_pipe);
    close(response_pipe);
    if (unlink(RESP_PIPE_NAME) == -1) {
        perror("Error unlinking the response pipe");
        return 1;
    }

    return 0;
}
