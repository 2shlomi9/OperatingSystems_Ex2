Operation Systems (OSs) Course Assignment 3
For Computer Science B.S.c Ariel University

By Shlomi Zecharia
Description

The task involves creating a simple HTTP server and client system for transferring files, using process and asynchronous I/O.:

    HTTP Protocol:
        HTTP (Hypertext Transfer Protocol) is a standard protocol used for communication between web servers and clients.
        It defines how messages are formatted and transmitted, as well as how servers and clients respond to various commands.

    Server Implementation:
        The server program should listen for incoming connections from clients.
        It should handle GET requests by serving files requested by clients and POST requests by accepting file uploads from clients.
        The server should respond appropriately to different scenarios, such as file not found or internal errors.

    Client Implementation:
        The client program should be able to send GET requests to download files from the server and POST requests to upload files.
        It should handle responses from the server and display appropriate messages to the user.

    File Transfer:
        File transfer between the client and server should be done using Base64 encoding to ensure compatibility and reliable transmission of binary data.
        For GET requests, the server sends the requested file's contents encoded in Base64.
        For POST requests, the client sends the file's contents encoded in Base64 to the server.

    Concurrency:
        Both the server and client should be able to handle multiple connections simultaneously using asynchronous I/O.
        This allows multiple clients to interact with the server concurrently without blocking operations.

    Error Handling and Status Codes:
        The system should handle various error scenarios gracefully, such as file not found or internal server errors.
        It should use appropriate HTTP status codes (e.g., 200 for success, 404 for file not found, 500 for internal server errors) to communicate the outcome of requests.

    Additional Requirements:
        The system should support basic file management operations like uploading and downloading files.
        Proper synchronization mechanisms should be implemented to prevent conflicts when multiple clients access the same file simultaneously.
        Both the server and client programs should be built using recursive make for ease of compilation and deployment.

Overall, the task involves implementing a simple HTTP file transfer system with basic functionality while ensuring proper error handling, concurrency, and compatibility with the HTTP protocol.

Requirements

    Linux machine (Ubuntu 22.04 LTS preferable)
    OpenSSL crypto library
    GNU C Compiler
    Make

Building

# Cloning the repo to local machine.
git clone https://github.com/2shlomi9/OperatingSystems_Ex2.git

# Install OpenSSL crypto library
sudo apt-get install libssl-dev

# Building all the necessary files & the main programs.
make all

Running

# Run server, add path to server files directory.
./server <PORT> <path>

# Run client for GET request.
./client <IP><PORT><GET><FILENAME>

# Run client for POST request.
./client <IP><PORT><POST><FILENAME>

# Run client for LIST request (DOWNLOAD ALL FILES).
./client <IP><PORT><LIST>

Options for TYPE and PARAM:

# IPv4 & TCP mode
ipv4 tcp

# IPv6 & TCP mode
ipv6 tcp

Blocking vs. Non-blocking I/O:
        In traditional I/O operations, when you read from or write to a file descriptor, the operation blocks the program until the operation completes. This means that if you have multiple file descriptors to read from or write to, you'd typically need to manage them individually, and if one of them blocks, it could potentially delay the handling of others.
        Non-blocking I/O, on the other hand, allows your program to continue executing even if the read or write operation would block. Instead of blocking, the function returns immediately with a special value indicating that the operation would block.

Using poll to Handle Multiple File Descriptors:
        The poll function is a system call that allows you to monitor multiple file descriptors to see if any of them become ready for I/O operations (e.g., reading or writing) without blocking the program.
        It takes an array of struct pollfd structures, each describing a file descriptor and the events to monitor for that descriptor.
        You can specify which events you are interested in monitoring (e.g., POLLIN for data to read, POLLOUT for space to write, etc.).
        The poll function will block until at least one of the specified events occurs on one of the monitored file descriptors or until a timeout occurs.
        Once an event occurs, poll returns, and you can examine the revents field of the struct pollfd structures to determine which descriptors triggered events.

Explanation of the Code:
        The code initializes an array of struct pollfd named fds, initially with space for one file descriptor.
        For each socket that needs to be monitored, the code adds an entry to the fds array and updates the nfds variable to reflect the total number of descriptors being monitored.
        The poll function is then called with the fds array and the number of descriptors to monitor.
        Inside the loop, the code iterates over each entry in the fds array and checks if the corresponding file descriptor has triggered an event (e.g., data is available for reading).
        If an event occurs on a socket, the code takes appropriate action based on the event type (e.g., reading data, writing data, etc.).
        If no events occur, the poll function blocks until an event occurs on any of the monitored file descriptors.

