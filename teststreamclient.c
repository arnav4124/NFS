#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int sockfd;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];
    
    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IP address
    if (inet_pton(AF_INET, "0.0.0.0", &serv_addr.sin_addr) <= 0) {
        perror("Invalid address or Address not supported");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Connect to server
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    printf("Connected to server\n");

    // Open pipe to mpg123 to play audio
    FILE *audio_player = popen("ffplay -autoexit -nodisp -", "w"); // for ffplay
    if (audio_player == NULL) {
        perror("Error opening audio player");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Receive and play audio data
    ssize_t bytes_received;
    while ((bytes_received = recv(sockfd, buffer, BUFFER_SIZE, 0)) > 0) {
        fwrite(buffer, 1, bytes_received, audio_player);
        fflush(audio_player);
    }

    if (bytes_received < 0) {
        perror("Error receiving audio data");
    } else {
        printf("Audio stream ended\n");
    }

    // Clean up
    pclose(audio_player);
    close(sockfd);

    return 0;
}
