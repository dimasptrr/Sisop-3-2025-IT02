=void daemonize() {
  pid_t pid, sid;

  pid = fork();

  if (pid < 0)
    exit(EXIT_FAILURE);
  if (pid > 0)
    exit(EXIT_SUCCESS);

  umask(0);

  sid = setsid();
  if (sid < 0)
    exit(EXIT_FAILURE);

  if ((chdir("/home/kali/modul3/soal_1/")) < 0)
    exit(EXIT_FAILURE);

  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);
}

void write_log(const char *source, const char *action, const char *info) {
  FILE *log_file = fopen("server.log", "a");
  if (!log_file) return;

  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  char timestamp[32];
  strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);

  fprintf(log_file, "[%s][%s]: [%s] [%s]\n", source, timestamp, action, info);
  fclose(log_file);
}

void parse_buffer(char *buffer, char *command, char *data) {
  char temp[BUFFER_SIZE];
  strncpy(temp, buffer, BUFFER_SIZE);
  temp[BUFFER_SIZE - 1] = '\0';

  char *token = strtok(temp, "-");
  if (token != NULL) {
      strcpy(command, token);

      token = strtok(NULL, "-");
      if (token != NULL) {
          strcpy(data, token);
      } else {
          strcpy(data, "");
      }

  } else {
      strcpy(command, "");
      strcpy(data, "");
  }

  command[strcspn(command, "\r\n")] = 0;
  data[strcspn(data, "\r\n")] = 0;
}

void handle_ping(int client_fd) {
  write(client_fd, "pong", 4);
}

void handle_decrypt(int client_fd, const char *data) {
  int len = strlen(data);

  // 1) Reverse the hex text string (per char, bukan per byte!)
  char reversed[BUFFER_SIZE];
  for (int i = 0; i < len; i++) {
    reversed[i] = data[len - i - 1];
  }
  reversed[len] = '\0';

  // 2) Convert reversed hex string -> byte array
  unsigned char byte_array[BUFFER_SIZE];
  int byte_len = 0;
  for (int i = 0; i < len; i += 2) {
    char byte_str[3] = { reversed[i], reversed[i+1], 0 }; // 2 char + null
    byte_array[byte_len++] = (unsigned char)strtol(byte_str, NULL, 16);
  }

  // 3) Save to file with timestamp name
  time_t now = time(NULL);
  char filename[64];
  snprintf(filename, sizeof(filename), "database/%ld.jpeg", now);

  FILE *fp = fopen(filename, "wb");
  if (fp) {
    fwrite(byte_array, 1, byte_len, fp);
    fclose(fp);
  }

  // 4) Send response back
  char response[BUFFER_SIZE];
  snprintf(response, sizeof(response), "Saved %d bytes to %s", byte_len, filename);
  write(client_fd, response, strlen(response));
}

void bytes_to_hex(const unsigned char *bytes, size_t len, char *hex_output) {
    const char hex_digits[] = "0123456789ABCDEF";
    for (size_t i = 0; i < len; ++i) {
        hex_output[i * 2] = hex_digits[(bytes[i] >> 4) & 0x0F];
        hex_output[i * 2 + 1] = hex_digits[bytes[i] & 0x0F];
    }
    hex_output[len * 2] = '\0'; // Null-terminate
}

void handle_download(int client_sockfd, const char *filename) {
    char path[64];
snprintf(path, sizeof(path), "database/%s", filename);
//send(client_sockfd, path, strlen(path), 0);
        FILE *file = fopen(path, "rb");
    if (!file) {
        char *error_msg = "Failed to open file";
        send(client_sockfd, error_msg, strlen(error_msg), 0);
        return;
    }

    unsigned char file_buffer[BUFFER_SIZE];
    char hex_buffer[BUFFER_SIZE * 2 + 1]; // Hex string buffer
    size_t bytes_read;

    while ((bytes_read = fread(file_buffer, 1, BUFFER_SIZE, file)) > 0) {
        bytes_to_hex(file_buffer, bytes_read, hex_buffer);
        send(client_sockfd, hex_buffer, strlen(hex_buffer), 0);
    }

    fclose(file);
}

void handle_invalid(int client_fd) {
  char *msg = "Invalid Command\n";
  write(client_fd, msg, strlen(msg));
}

void run_rpc_server() {
  int server_fd, client_fd;
  struct sockaddr_in server_addr, client_addr;
  socklen_t addr_len;
  char buffer[BUFFER_SIZE];
  char command[BUFFER_SIZE];
  char data[BUFFER_SIZE];

  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd == -1) {
    exit(EXIT_FAILURE);
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(PORT);

  if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  if (listen(server_fd, 5) < 0) {
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  while (1) {
    addr_len = sizeof(client_addr);
    client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
    if (client_fd < 0) {
      continue;
    }

    memset(buffer, 0, BUFFER_SIZE);
    read(client_fd, buffer, BUFFER_SIZE);

    memset(command, 0, BUFFER_SIZE);
    memset(data, 0, BUFFER_SIZE);
    parse_buffer(buffer, command, data);

    if (strcmp(command, "ping") == 0) {
      handle_ping(client_fd);
    } else if (strcmp(command, "decrypt") == 0) {
      handle_decrypt(client_fd, data);
      write_log("Client", "DECRYPT", "Text Data");

    } else if (strcmp(command, "download") == 0) {
      handle_download(client_fd,data);
      write_log("Client", "DOWNLOAD", "Text Data");

    } else if (strcmp(command, "exit") == 0) {
      close(client_fd);
      write_log("Client", "EXIT", "Client Requested Exit");

    } else {
      handle_invalid(client_fd);
      write_log("Client", "INVALID", buffer);
    }

    close(client_fd);
  }

  close(server_fd);
}

int main() {
  daemonize();

  if (mkdir("database", 0755) == -1 && errno != EEXIST) {
    perror("mkdir failed");
    exit(EXIT_FAILURE);
  }

  FILE *log_file = fopen("server.log", "a");
  if (!log_file) {
    perror("Failed to create/open server.log");
    exit(EXIT_FAILURE);
  }
  fclose(log_file);

  run_rpc_server();
  return 0;
}
