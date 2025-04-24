#define _XOPEN_SOURCE 700 //needed this for VSCode Intellisense......
#include <sys/socket.h>
#include <signal.h>
#include <features.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <netinet/in.h>
#include <syslog.h>
#include <fcntl.h>
#include <stdbool.h>
#include <arpa/inet.h>

#define PORT 9000
#define FILE_PATH "/var/tmp/aesdsocketdata"
#define BUF_SIZE 1024

int sockfd;
struct sockaddr_in server_addr;

volatile sig_atomic_t exit_requested = false;

//get rid of socket if SIGTERM or SIGINT is sent
void handle_signal(int sig) {
    syslog(LOG_INFO, "Caught signal %d, exiting", sig);
    exit_requested = true;  // set flag
}

int main(int argc, char *argv[]) {

    //allow -d for daemon mode
    int daemon_mode = 0;
    if (argc == 2 && strcmp(argv[1], "-d") == 0) {
        daemon_mode = 1;
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        syslog(LOG_ERR, "Failed to create socket");
        return -1;
    }

    // needed to add a sockopt for ./full-test.sh, gets rid of "address in use"
    // was failing .full-test, seemed to work!!!
    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        syslog(LOG_ERR, "setsockopt failed");
        close(sockfd);
        return -1;
    }

    //fill up server_addr with zeroes
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        syslog(LOG_ERR, "Binding failed");
        close(sockfd);
        return -1;
    }

    //try 5 times
    if (listen(sockfd, 5) == -1) {
        syslog(LOG_ERR, "Listen failed");
        close(sockfd);
        return -1;
    }

    if (daemon_mode) {
        pid_t pid = fork();
        if (pid < 0) {
            syslog(LOG_ERR, "Fork failed");
            exit(EXIT_FAILURE);
        }
        if (pid > 0) exit(EXIT_SUCCESS);
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
    }

    openlog("aesdsocket", LOG_PID, LOG_USER);

    //link sigaction to happen on SIGINT or SIGTERM to handle_signal, which sends a bool to stop the while loop
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    int clear_fd = open(FILE_PATH, O_WRONLY | O_TRUNC | O_CREAT, 0644);
    if (clear_fd >= 0) {
        close(clear_fd);
    } else {
        syslog(LOG_ERR, "Failed to clear file %s", FILE_PATH);
    }

    // keep looping until a term signal is received!
    while (!exit_requested) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int connfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_len);
        if (connfd == -1) {
            if (errno == EINTR && exit_requested) break;
            syslog(LOG_ERR, "Accept failed: %s", strerror(errno));
            continue;
        }

        syslog(LOG_INFO, "Accepted connection from %s", inet_ntoa(client_addr.sin_addr));

        char buffer[BUF_SIZE];
        size_t total_received = 0;
        char *data_buffer = NULL;

        while (1) {
            ssize_t bytes_received = recv(connfd, buffer, BUF_SIZE, 0);
            if (bytes_received <= 0) break;

            char *new_buffer = realloc(data_buffer, total_received + bytes_received);
            if (!new_buffer) {
                syslog(LOG_ERR, "Memory allocation failed");
                free(data_buffer);
                close(connfd);
                continue;
            }

            data_buffer = new_buffer;
            memcpy(data_buffer + total_received, buffer, bytes_received);
            total_received += bytes_received;

            //keep getting data from buffer until we see \n
            if (memchr(buffer, '\n', bytes_received)) {
                break;
            }
        }

        if (total_received > 0) {
            int file_fd = open(FILE_PATH, O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (file_fd >= 0) {
                write(file_fd, data_buffer, total_received);
                close(file_fd);
            } else {
                syslog(LOG_ERR, "Could not open file %s", FILE_PATH);
            }

            file_fd = open(FILE_PATH, O_RDONLY);
            if (file_fd >= 0) {
                ssize_t bytes_read;
                while ((bytes_read = read(file_fd, buffer, BUF_SIZE)) > 0) {
                    send(connfd, buffer, bytes_read, 0);
                }
                close(file_fd);
            } else {
                syslog(LOG_ERR, "Failed to open %s for reading", FILE_PATH);
            }
        }

        free(data_buffer);
        syslog(LOG_INFO, "Closed connection from %s", inet_ntoa(client_addr.sin_addr));
        close(connfd);
    }

    close(sockfd);
    unlink(FILE_PATH);
    closelog();
    return 0;
}