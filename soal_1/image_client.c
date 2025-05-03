#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define clear_screen() system("clear")

#define PORT 6969
#define LOCALHOST "127.0.0.1"
#define BUFFER_SIZE 6969

int create_connection() {
    int sockfd;
    struct sockaddr_in server_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, LOCALHOST, &server_addr.sin_addr);

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

void send_message(int sockfd, const char *message) {
    write(sockfd, message, strlen(message));
}

void receive_message(int sockfd, char *buffer) {
    memset(buffer, 0, BUFFER_SIZE);
    read(sockfd, buffer, BUFFER_SIZE);
}

void handle_exit() {
    int sockfd = create_connection();
    send_message(sockfd, "exit-null");
    close(sockfd);
    printf("Exiting...\n");
    exit(EXIT_SUCCESS);
}

void handle_send() {
    char filename[256];
    printf("Enter filename to send ➤ ");
    scanf("%s", filename);
    getchar();  // consume newline

    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Failed to open file");
        return;
    }

    char file_content[BUFFER_SIZE];
    memset(file_content, 0, BUFFER_SIZE);

    fread(file_content, 1, BUFFER_SIZE - 1, fp);
    fclose(fp);

    int sockfd = create_connection();
    char buffer[BUFFER_SIZE];

    char message[BUFFER_SIZE];
    snprintf(message, sizeof(message), "decrypt-%s", file_content);

    send_message(sockfd, message);
    receive_message(sockfd, buffer);
//    clear_screen();
    
    printf("Server Response: %s\n\n", buffer);

    close(sockfd);
}

void hex_to_bytes(const char *hex_str, unsigned char *byte_array, size_t *byte_len) {
    size_t hex_len = strlen(hex_str);
    *byte_len = hex_len / 2;
    for (size_t i = 0; i < *byte_len; ++i) {
        sscanf(&hex_str[i * 2], "%2hhx", &byte_array[i]);
    }
}

void handle_download(const char *filename) {
    int sockfd = create_connection();
    char buffer[BUFFER_SIZE * 2];

    char message[300];
    snprintf(message, sizeof(message), "download-%s", filename);

    send_message(sockfd, message);

    FILE *outfile = fopen(filename, "wb");
    if (!outfile) {
        perror("Failed to open output file");
        close(sockfd);
        return;
    }

  //  clear_screen();

    printf("Downloading and saving to file: %s\n", filename);

    ssize_t bytes_received;
    while ((bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';

        unsigned char byte_array[BUFFER_SIZE];
        size_t byte_len;
        hex_to_bytes(buffer, byte_array, &byte_len);

        fwrite(byte_array, 1, byte_len, outfile);
    }

    if (bytes_received < 0) {
        perror("recv failed");
    }

    printf("Download complete!\n");

    fclose(outfile);
    close(sockfd);
}

void show_menu() {
    printf("========================================\n");
    printf("        Image Decoder Client            \n");
    printf("========================================\n");
    printf(" 1. Send input file to server\n");
    printf(" 2. Download file from server\n");
    printf(" 3. Exit\n");
    printf("========================================\n");
    printf("Enter your choice ➤ ");
}

int main() {
    char choice;

    while (1) {
        show_menu();
        choice = getchar();
        getchar(); 

        switch (choice) {
            case '1':
                handle_send();
                break;
            case '2': {
                char filename[256];
                printf("Enter filename to download: ");
                scanf("%255s", filename);
                getchar();  // consume newline
                handle_download(filename);
                break;
            }
            case '3':
                handle_exit();
                break;
            default:
                printf("Invalid choice, please try again.\n");
        }
    }

    return 0;
}
