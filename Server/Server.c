#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>

#define BUFFER_SIZE 1024

// Function to send response to client (encoded)


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
void send_response(int client_socket, const char *response) {
    char *encoded_response = base64_encode((const unsigned char *)response, strlen(response));
    send(client_socket, encoded_response, strlen(encoded_response), 0);
    free(encoded_response);
}
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

void list_files(const char *root_directory) {
    char list_path[1024];
    snprintf(list_path, sizeof(list_path), "%s/list.txt", root_directory);

    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(root_directory)) != NULL) {
        FILE *file = fopen(list_path, "w");
        if (file != NULL) {
            while ((ent = readdir(dir)) != NULL) {
                if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0 && strcmp(ent->d_name, "list.txt") != 0) {
                    fprintf(file, "%s\n", ent->d_name);
                }
            }
            fclose(file);
        }
        closedir(dir);
    } else {
        perror("opendir");
    }
}

// Function to handle GET request
void handle_get_request(int client_socket, const char *root_directory, const char *path) {
    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "%s/%s", root_directory, path);

    // Open the file for reading
    int file_descriptor = open(full_path, O_RDONLY);
    if (file_descriptor == -1) {
        if (errno == ENOENT) {
            // File not found
            send_response(client_socket, "404 FILE NOT FOUND\r\n\r\n");
        } else {
            // Internal server error
            send_response(client_socket, "500 INTERNAL ERROR\r\n\r\n");
        }
        return;
    }
    send_response(client_socket,"200 OK \r\n\r\n");
    char* isReacive = receive_data(client_socket);
        printf("bytes recv - %ld ,  the data recveive - %s", sizeof(isReacive),isReacive);
    if (strcmp(isReacive,"YES") == 0)
    {
        // Lock the file
    struct flock lock;
    memset(&lock, 0, sizeof(lock));
    lock.l_type = F_RDLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    fcntl(file_descriptor, F_SETLKW, &lock);

    // Read file contents and send to client
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    while ((bytes_read = read(file_descriptor, buffer, BUFFER_SIZE)) > 0) {
        char *encoded_data = base64_encode((const unsigned char *)buffer, bytes_read);
        send(client_socket, encoded_data, strlen(encoded_data), 0);
        free(encoded_data);
    }

    // Unlock the file
    lock.l_type = F_UNLCK;
    fcntl(file_descriptor, F_SETLK, &lock);

    // Close the file
    close(file_descriptor);
    // Update list.txt
    list_files(root_directory);

    // Close connection to client
    close(client_socket);



    }
    else{

        // Close the file
        close(file_descriptor);

        // Close connection to client
        close(client_socket);
}
}

// Function to handle POST request
void handle_post_request(int client_socket, const char *root_directory, const char *path) {
    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "%s/%s", root_directory, path);

    // Open the file for writing
    int file_descriptor = open(full_path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (file_descriptor == -1) {
        // Internal server error
        send_response(client_socket, "500 INTERNAL ERROR\r\n\r\n");
        return;
    }
    printf("\nin the function\n");
    send_response(client_socket,"POST \r\n\r\n");
        
    // Lock the file
    struct flock lock;
    memset(&lock, 0, sizeof(lock));
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    fcntl(file_descriptor, F_SETLKW, &lock);

    // Read data from client and write to file
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;
    while ((bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0)) > 0) {
        int decoded_length;
        unsigned char *decoded_data = base64_decode(buffer, bytes_received, &decoded_length);
        // Update list.txt
        list_files(root_directory);
        write(file_descriptor, decoded_data, decoded_length);
        printf("bytes recv - %ld ,  the data recv - %s\n", bytes_received,decoded_data);
        free(decoded_data);
    }
    

    // Unlock the file
    lock.l_type = F_UNLCK;
    fcntl(file_descriptor, F_SETLK, &lock);

    // Close the file
    close(file_descriptor);
    close(client_socket);



}

// Function to handle client request
void handle_client_request(int client_socket, const char *root_directory) {
    char request[BUFFER_SIZE];
    ssize_t bytes_received = recv(client_socket, request, BUFFER_SIZE, 0);
    if (bytes_received <= 0) {
        close(client_socket);
        return;
    }
    
    request[bytes_received] = '\0';

    // Decode the request
    int decoded_length;
    unsigned char *decoded_request = base64_decode(request, bytes_received, &decoded_length);
    
    printf("bytes recv - %ld ,  the data recv - %s", bytes_received,decoded_request);
    decoded_request[decoded_length] = '\0';

    // Extract the HTTP method and path from the request
    char method[16], path[256];
    sscanf((char *)decoded_request, "%s %s", method, path);


    // Handle the request based on the method
    if (strcmp(method, "GET") == 0) {
        handle_get_request(client_socket, root_directory, path);
        return;
    } else if (strcmp(method, "POST") == 0) {
        printf("get in");
        handle_post_request(client_socket, root_directory, path);
        return;
    } else {
        // Invalid method
        send_response(client_socket, "500 INTERNAL ERROR\r\n\r\n");
        close(client_socket);
    }

    // Free allocated memory
    free(decoded_request);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <server_port> <root_directory>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int server_port = atoi(argv[1]);
    const char *root_directory = argv[2];

    // Create a socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Bind the socket
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_socket);
        exit(EXIT_FAILURE);
    }
    // Update list.txt
    list_files(root_directory);

    // Listen for connections
    if (listen(server_socket, 10) == -1) {
        perror("listen");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", server_port);


    // Accept connections and handle requests
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket == -1) {
            perror("accept");
            continue;
        }

        // Fork to handle the client request
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            close(client_socket);
            continue;
        } else if (pid == 0) {
            // Child process
            close(server_socket);  // Close the server socket in the child
            handle_client_request(client_socket, root_directory);
            exit(EXIT_SUCCESS);  // Exit the child process
        } else {
            // Parent process
            close(client_socket);  // Close the client socket in the parent
        }
    }

    // Close the server socket
    close(server_socket);

    return 0;
}