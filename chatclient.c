/*******************************************************************************
 * Name: Sanjay Athrey
 * Pledge: I pledge my honor that I have abided by the Stevens Honor System
 ******************************************************************************/
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "util.h"

volatile sig_atomic_t running_status = true;

void catch_signal(int sig) {
  running_status = false;
  write(STDOUT_FILENO, "\n", 1);
}

int received_bytes(const int *client_socket, char *inbuf) {
  int sock_bytes = recv(*client_socket, inbuf, BUFLEN, 0);
  inbuf[sock_bytes] = '\0';
  return sock_bytes;
}

int sent_bytes(const int *client_socket, char *outbuf, size_t size) {
  int bytes_sent = send(*client_socket, outbuf, size, 0);
  return bytes_sent;
}

int main(const int argc, const char *argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <server IP> <port>\n", argv[0]);
    return EXIT_FAILURE;
  }

  int port_num;
  const char* ip = argv[1];

  struct sockaddr_in server_addr;
  socklen_t addrlen = sizeof(struct sockaddr_in);
  memset(&server_addr, 0, addrlen);
  server_addr.sin_family = AF_INET;
  
    
  if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
    fprintf(stderr, "Error: Invalid IP address '%s'.\n", argv[1]);
    return EXIT_FAILURE;
  }
  if (!is_integer(argv[2])) {
    fprintf(stderr, "Error: Invalid input '%s' received for port number.\n", argv[2]);
    return EXIT_FAILURE;
  }
  if (!parse_int(argv[2], &port_num, "port number")) {
    return EXIT_FAILURE;
  }
  server_addr.sin_port = htons(port_num);
  if (port_num < 1024 || port_num > 65535) {
    fprintf(stderr, "Error: Port must be in range [1024, 65535].\n");
    return EXIT_FAILURE;
  }

  char username[MAX_NAME_LEN + 1];
  char inbuf[BUFLEN + 1];
  char outbuf[MAX_MSG_LEN + 1];

 
  printf("Enter your username: ");
  fflush(stdout);

  int name;
    while (name != OK) {
      if (name == TOO_LONG) {
        fprintf(stderr, "Sorry, limit your username to %d characters.\n", MAX_NAME_LEN);
        printf("Enter your username: ");
        fflush(stdout);
      }
      if (name == NO_INPUT) {
        printf("Enter your username: ");
        fflush(stdout);
      }
      name = get_string(username, MAX_NAME_LEN);
    }
  
  
  printf("Hello, %s. Let's try to connect to the server.\n", username);

  int client_socket = -1;
  int retval;
  if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    fprintf(stderr, "Error: Failed to create socket. %s.\n", strerror(errno));
    retval = EXIT_FAILURE;
    goto EXIT;
  }

  if (connect(client_socket, (struct sockaddr *)&server_addr, addrlen) < 0) {
    fprintf(stderr, "Error: Failed to connect to server. %s.\n", strerror(errno));
    retval = EXIT_FAILURE;
    goto EXIT;
  }

  int welcome_bytes = received_bytes(&client_socket, inbuf);
  if (welcome_bytes < 0) {
    fprintf(stderr,  "Error: Failed to receive message from server. %s.\n", strerror(errno));
    retval = EXIT_FAILURE;
    goto EXIT;
  }
  if (welcome_bytes == 0) {
    fprintf(stderr, "All connections are busy. Try again later.\n");
    retval = EXIT_FAILURE;
    goto EXIT;
  }
  
  printf("\n%s\n\n", inbuf);
  strcpy(outbuf, username);
  
  int username_bytes = sent_bytes(&client_socket, outbuf, strlen(username));
  if (username_bytes < 0) {
    fprintf(stderr, "Error: Failed to send username to server. %s.\n", strerror(errno));
    retval = EXIT_FAILURE;
    goto EXIT;
  }

  struct sigaction action;
  memset(&action, 0, sizeof(struct sigaction));
  action.sa_handler = catch_signal;
  if (sigaction(SIGINT, &action, NULL) == -1) {
    fprintf(stderr, "Error: Failed to register signal handler. %s.\n", strerror(errno));
    return EXIT_FAILURE;
  }

  printf("[%s]: ", username);
  fflush(stdout);

  fd_set sockset;
  while (running_status) {
    FD_ZERO(&sockset);
    FD_SET(STDIN_FILENO, &sockset);
    FD_SET(client_socket, &sockset);

    if ((select(client_socket + 1, &sockset, NULL, NULL, NULL) < 0) && (errno != EINTR)) {
      fprintf(stderr, "Error: select() failed. %s.\n", strerror(errno));
      retval = EXIT_FAILURE;
      goto EXIT;
    }

    // handle stdin
    if (running_status && FD_ISSET(STDIN_FILENO, &sockset)) {
          if ((get_string(outbuf, MAX_MSG_LEN) == OK) && sent_bytes(&client_socket, outbuf, strlen(outbuf)) >= 0) {
            if ((strcmp(outbuf, "bye") == 0)) {
              printf("Goodbye.\n");
              retval = EXIT_SUCCESS;
              goto EXIT;
            } else {
              printf("[%s]: ", username);
              fflush(stdout);
            }
          }
           else if (get_string(outbuf, MAX_MSG_LEN) == TOO_LONG){
            printf("Sorry, limit your message to %d characters.\n", MAX_MSG_LEN);
            printf("[%s]: ", username);
            fflush(stdout);
          }
          else {
            printf("[%s]: ", username);
            fflush(stdout);
          }

    }

    //handle client socket
    if (running_status && FD_ISSET(client_socket, &sockset)) {
      int sock_bytes = received_bytes(&client_socket, inbuf);
      if (sock_bytes < 0 ) {
        fprintf(stderr, "Warning: Failed to receive incoming message. %s.\n", strerror(errno));
      }
      if (sock_bytes == 0) {
        retval = EXIT_FAILURE;
        goto EXIT;
      } 
      if (sock_bytes > 0) {
        if (strcmp(inbuf, "bye") == 0) {
          printf("\nServer initiated shutdown.\n");
          retval = EXIT_SUCCESS;
          goto EXIT;
        } else {
          printf("\n%s\n", inbuf);
          printf("[%s]: ", username);
          fflush(stdout);
        }
      }
    }
  }

EXIT:
  if (fcntl(client_socket, F_GETFD) >= 0) {
    close(client_socket);
  }
  return retval;
}
