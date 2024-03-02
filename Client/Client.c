#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>

#define MAX_FILES 10
#define BUFFER_SIZE 1024

// Function to base64 encode data
char* base64_encode(const unsigned char *input, int length) {
    BIO *bmem, *b64;
    BUF_MEM *bptr;

    b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bmem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bmem);
    BIO_write(b64, input, length);
    BIO_flush(b64);
    BIO_get_mem_ptr(b64, &bptr);

    char *buff = (char *)malloc(bptr->length + 1);
    memcpy(buff, bptr->data, bptr->length);
    buff[bptr->length] = 0;

    BIO_free_all(b64);
    return buff;
}

// Function to base64 decode data
unsigned char* base64_decode(const char *input, int length, int *output_length) {
    BIO *b64, *bmem;

    unsigned char *buffer = (unsigned char *)malloc(length);
    if (!buffer) return NULL;
    memset(buffer, 0, length);

    b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bmem = BIO_new_mem_buf((void *)input, length);
    bmem = BIO_push(b64, bmem);

    *output_length = BIO_read(bmem, buffer, length);

    BIO_free_all(bmem);
    return buffer;
}

// Function to send request to server (with base64 encoding)
void send_request(int server_socket, const char *request) {
    // Encode the request using base64
    char *encoded_request = base64_encode((const unsigned char *)request, strlen(request));
    send(server_socket, encoded_request, strlen(encoded_request), 0);
    free(encoded_request);
}

// Function to receive data from server (with base64 decoding)
char *receive_data(int server_socket) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;
    while ((bytes_received = recv(server_socket, buffer, BUFFER_SIZE, 0)) > 0) {
        // Decode the received data using base64
        int decoded_length;
        unsigned char *decoded_data = base64_decode(buffer, bytes_received, &decoded_length);
        // Write the decoded data to STDOUT
        write(STDOUT_FILENO, decoded_data, decoded_length);
        char *data = (char *)decoded_data;
        return data;
        free(decoded_data);
    }
    return "";
}


void download_file(const char *server_address, int server_port, const char *file_name) {
    const char *download_folder = "./downloads";
    struct stat st;

    // Check if the downloads folder exists, if not create it
    if (stat(download_folder, &st) == -1) {
        mkdir(download_folder, 0700); // Create the downloads folder with read, write, and execute permissions for owner
    }

    char full_path[2048];
    snprintf(full_path, sizeof(full_path), "%s/%s", download_folder, file_name); // Construct the full file path

    // Open the file for writing
    int file_descriptor = open(full_path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (file_descriptor == -1) {
        perror("open");
        printf("Download failed");
        return;
    }

    // Create a socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Connect to the server
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    inet_pton(AF_INET, server_address, &server_addr.sin_addr);
    if (connect(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Send GET request for file
    char request[BUFFER_SIZE];
    snprintf(request, sizeof(request), "GET /%s HTTP/1.1\r\n\r\n", file_name);
    send_request(server_socket, request);
    char* response = receive_data(server_socket);
    printf("bytes recv - %ld ,  the data recv - %s", sizeof(response),response);
    if (strcmp(response,"200 OK \r\n\r\n") == 0)
    {
        send_request(server_socket,"YES");
    

    // Read data from server and write to file
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;
    while ((bytes_received = recv(server_socket, buffer, BUFFER_SIZE, 0)) > 0) {
        // Decode the received data using base64
        int decoded_length;
        unsigned char *decoded_data = base64_decode(buffer, bytes_received, &decoded_length);

        // Write the decoded data to STDOUT
        write(file_descriptor, decoded_data, decoded_length);
        printf("bytes recv - %ld ,  the data recv - %s\n", bytes_received,decoded_data);
        free(decoded_data);
    }

    // Close the file and socket
    close(file_descriptor);
    close(server_socket);

    }
    else{
        remove(full_path);
        // Close the file and socket
        close(file_descriptor);
        close(server_socket);  
    }
}



// Function to upload file to server (with base64 encoding)
void upload_file(const char *server_address, int server_port, const char *file_name) {
    // Open the file for reading
    int file_descriptor = open(file_name, O_RDONLY);
    if (file_descriptor == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    // Create a socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("socket");
        close(file_descriptor);
        exit(EXIT_FAILURE);
    }

    // Connect to the server
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    inet_pton(AF_INET, server_address, &server_addr.sin_addr);
    if (connect(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        close(server_socket);
        close(file_descriptor);
        exit(EXIT_FAILURE);
    }
    printf("in the function");
    // Send GET request for file
    char request[BUFFER_SIZE];
    snprintf(request, sizeof(request), "POST %s HTTP/1.1\r\n\r\n", file_name);
    send_request(server_socket, request);
    char* response = receive_data(server_socket);
    if (strcmp(response,"POST %s HTTP/1.1\r\n\r\n"))
    {
        
    


    // Read data from file and send to server
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    while ((bytes_read = read(file_descriptor, buffer, BUFFER_SIZE)) > 0) {
        write(STDOUT_FILENO, buffer, bytes_read);
    char *encoded_data = base64_encode((const unsigned char *)buffer, bytes_read);
    send(server_socket, encoded_data, strlen(encoded_data), 0);
    free(encoded_data);
    }
    // Close the file
    close(file_descriptor);

    // Close the socket
    close(server_socket);
          
}
    }




    // Function to download all files listed in list.txt

    void download_all_files(const char *server_address, int server_port){
    download_file(server_address,server_port,"list.txt");
            // Open list.txt file for reading
    char list_file_path[256];
    snprintf(list_file_path, sizeof(list_file_path), "./downloads/list.txt");
    FILE *list_file = fopen(list_file_path, "r");
    if (list_file == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    // Create a socket and connect to the server
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    inet_pton(AF_INET, server_address, &server_addr.sin_addr);
    if (connect(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Read file names line by line and send GET request for each file
    char file_name[1024];
    while (fgets(file_name, sizeof(file_name), list_file) != NULL) {
        // Remove trailing newline character, if present
        size_t len = strlen(file_name);
        if (len > 0 && file_name[len - 1] == '\n') {
            file_name[len - 1] = '\0';
        }

        // Download the file
        download_file(server_address, server_port, file_name);
    }

    // Close the list.txt file
    fclose(list_file);

    // Close the socket
    close(server_socket);
    remove(list_file_path);
}


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <server_address> <server_port> <command> [<file_name>]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *server_address = argv[1];
    int server_port = atoi(argv[2]);
    const char *command = argv[3];

    // Initialize pollfd array
    struct pollfd *fds = NULL;
    int nfds = 0;

    // Set up pollfd for server_socket
    fds = realloc(fds, sizeof(struct pollfd));
    if (fds == NULL) {
        perror("realloc");
        exit(EXIT_FAILURE);
    }
    fds[nfds].fd = -1; // Initially set to invalid fd
    fds[nfds].events = POLLIN;
    nfds++;

    if (strcmp(command, "POST") == 0 && argc == 5) {
        const char *file_name = argv[4];

        // Create a socket and connect
        int server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket == -1) {
            perror("socket");
            exit(EXIT_FAILURE);
        }

        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(server_port);
        inet_pton(AF_INET, server_address, &server_addr.sin_addr);
        if (connect(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
            perror("connect");
            close(server_socket);
            exit(EXIT_FAILURE);
        }

        // Send POST request for file
        char request[BUFFER_SIZE];
        snprintf(request, sizeof(request), "POST %s HTTP/1.1\r\n\r\n", file_name);
        send_request(server_socket, request);

        // Update pollfd to include server_socket
        fds = realloc(fds, (nfds + 1) * sizeof(struct pollfd));
        if (fds == NULL) {
            perror("realloc");
            close(server_socket);
            exit(EXIT_FAILURE);
        }
        fds[nfds].fd = server_socket;
        fds[nfds].events = POLLIN;
        nfds++;

        // Perform polling
        while (poll(fds, nfds, -1) > 0) {
            for (int i = 0; i < nfds; i++) {
                if (fds[i].revents & POLLIN) {
                    char* response = receive_data(fds[i].fd);
                    printf("bytes recv - %ld ,  the data recv - %s", sizeof(response),response);
                    if (strcmp(response,"POST \r\n\r\n") == 0) {
                        // Receive and print file data
                        upload_file(server_address,server_port,file_name);
                        close(server_socket);
                        exit(EXIT_SUCCESS);
                    } else {
                        close(server_socket);
                        exit(EXIT_FAILURE);
                    }
                }
            }
        }

        // Close the socket
        close(server_socket);
    } else if (strcmp(command, "GET") == 0 && argc == 5) {
        const char *file_name = argv[4];

        // Create a socket and connect
        int server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket == -1) {
            perror("socket");
            exit(EXIT_FAILURE);
        }

        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(server_port);
        inet_pton(AF_INET, server_address, &server_addr.sin_addr);
        if (connect(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
            perror("connect");
            close(server_socket);
            exit(EXIT_FAILURE);
        }

        // Send GET request for file
        char request[BUFFER_SIZE];
        snprintf(request, sizeof(request), "GET %s HTTP/1.1\r\n\r\n", file_name);
        send_request(server_socket, request);

        // Update pollfd to include server_socket
        fds = realloc(fds, (nfds + 1) * sizeof(struct pollfd));
        if (fds == NULL) {
            perror("realloc");
            close(server_socket);
            exit(EXIT_FAILURE);
        }
        fds[nfds].fd = server_socket;
        fds[nfds].events = POLLIN;
        nfds++;

        // Perform polling
        while (poll(fds, nfds, -1) > 0) {
            for (int i = 0; i < nfds; i++) {
                if (fds[i].revents & POLLIN) {
                    char* response = receive_data(fds[i].fd);
                    printf("bytes recv - %ld ,  the data recv - %s", sizeof(response),response);
                    if (strcmp(response,"200 OK \r\n\r\n") == 0) {
                        // Receive and print file data
                        download_file(server_address,server_port,file_name);
                        close(server_socket);
                        exit(EXIT_SUCCESS);
                    } else {
                        close(server_socket);
                        exit(EXIT_FAILURE);
                    }
                }
            }
        }

        // Close the socket
        close(server_socket);

    } else if (strcmp(command, "LIST") == 0 && argc == 4) {

        // Create a socket and connect
        int server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket == -1) {
            perror("socket");
            exit(EXIT_FAILURE);
        }

        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(server_port);
        inet_pton(AF_INET, server_address, &server_addr.sin_addr);
        if (connect(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
            perror("connect");
            close(server_socket);
            exit(EXIT_FAILURE);
        }

        // Send GET request for file
        char request[BUFFER_SIZE];
        snprintf(request, sizeof(request), "LIST HTTP/1.1\r\n\r\n");
        send_request(server_socket, request);

        // Update pollfd to include server_socket
        fds = realloc(fds, (nfds + 1) * sizeof(struct pollfd));
        if (fds == NULL) {
            perror("realloc");
            close(server_socket);
            exit(EXIT_FAILURE);
        }
        fds[nfds].fd = server_socket;
        fds[nfds].events = POLLIN;
        nfds++;

        // Perform polling
        while (poll(fds, nfds, -1) > 0) {
            for (int i = 0; i < nfds; i++) {
                if (fds[i].revents & POLLIN) {
                    // Receive and print file data
                    download_all_files(server_address,server_port);
                    close(server_socket);
                    exit(EXIT_SUCCESS);
                }
            }
        }

        // Close the socket
        close(server_socket);
    } else {
        fprintf(stderr, "Invalid command or missing file name.\n");
        exit(EXIT_FAILURE);
    }

    return 0;
}
