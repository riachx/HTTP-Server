#include "parser.h"
#include "asgn2_helper_funcs.h"

// reads from the socket and stops at \r\n\r\n
int my_read(int in, char *buf, ssize_t nbytes) {
    ssize_t total_bytes_read = 0; // total bytes read so far
    ssize_t bytes_read;
    while (total_bytes_read < nbytes) {
        bytes_read = read(in, buf + total_bytes_read, 1);

        if (bytes_read < 0) {

            return -1;
        } else if (bytes_read == 0) {

            return total_bytes_read;
        }

        total_bytes_read += bytes_read;

        // check if the pattern is in buffer
        if (total_bytes_read >= 4) {
            buf[total_bytes_read] = '\0';
            if (strstr(buf, "\r\n\r\n") != NULL) {
                break;
            }
        }
    }
    return total_bytes_read;
}

int main(int argc, char *argv[]) {

    if (argc != 2) {
        fprintf(stderr, "Error: Invalid number of arguments.\n");
        exit(EXIT_FAILURE);
    }

    int port = strtol(argv[1], NULL, 10);
    if (port < 1 || port > 65585) {
        fprintf(stderr, "Invalid Port\n");
        exit(EXIT_FAILURE);
    }
    Listener_Socket socket;
    int socket_status = listener_init(&socket, port);

    if (socket_status == -1) {
        fprintf(stderr, "Error starting server\n");
        exit(EXIT_FAILURE);
    }

    printf("argv: %s", argv[1]);
    printf("loop starting");

    while (1) {
        int socket_accepted = listener_accept(&socket);
        if (socket_accepted == -1) {
            fprintf(stderr, "Error connecting.\n");
            exit(EXIT_FAILURE);
        }
        close(socket_accepted);
    }
    return 0;
}
