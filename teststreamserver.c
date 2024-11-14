#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    char buffer[BUFFER_SIZE];
    FILE *audio_file;

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }


    // Bind socket to the specified port
    address.sin_family = AF_INET;
    inet_pton(AF_INET, "0.0.0.0", &address.sin_addr);

    address.sin_port = htons(PORT);

    int opt = 1;
if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    perror("setsockopt failed");
    exit(EXIT_FAILURE);
}

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(server_fd, 1) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("Server is listening on port %d\n", PORT);

    // Accept a connection
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen)) < 0) {
        perror("Accept failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("Client connected\n");

    // Open the audio file (change to "audio.wav" if streaming a WAV file)
    audio_file = fopen("audio.wav", "rb");
    if (audio_file == NULL) {
        perror("Could not open audio file");
        close(new_socket);
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Send audio file in chunks
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, audio_file)) > 0) {
        if (send(new_socket, buffer, bytes_read, 0) == -1) {
            perror("Failed to send data to client");
            break;
        }
        fflush(stdout);  // Flush to ensure timely sending of data
    }


    printf("Audio file sent successfully\n");

    // Clean up
    fclose(audio_file);
    close(new_socket);
    close(server_fd);
    return 0;
}
