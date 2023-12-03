#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <stddef.h>
#include <limits.h>
#include <sys/stat.h>
#include <regex.h>
#include "parser.h"

int parse_uri(RequestObj *R, char *uri) {

    regex_t regex;
    const char *pattern = "/([a-zA-Z0-9.-]{1,63})";
    int ret;

    // compile the regular expression
    if (regcomp(&regex, pattern, REG_EXTENDED) != 0) {
        dprintf(R->response, "HTTP/1.1 400 Bad Request\r\nContent-Length: 12\r\n\r\nBad Request\n");
        return 1;
    }

    // perform the regex matching
    ret = regexec(&regex, uri, 0, NULL, 0);

    if (ret == 0) {

    } else if (ret == REG_NOMATCH) { // 400 error if pattern does not match
        dprintf(R->response, "HTTP/1.1 400 Bad Request\r\nContent-Length: 12\r\n\r\nBad Request\n");
        return 1;
    } else {
        char errbuf[100];
        regerror(ret, &regex, errbuf, sizeof(errbuf));
        fprintf(stderr, "Regex match failed: %s\n", errbuf);
    }

    // free the compiled regex
    regfree(&regex);

    return 0;
}

int parse_version(RequestObj *R, char *version) {

    regex_t regex;
    const char *pattern = " HTTP/[0-9].[0-9]\r\n";
    int ret;

    // compile the regex
    if (regcomp(&regex, pattern, REG_EXTENDED) != 0) {
        dprintf(
            R->response, "HTTP/1.1 400 Bad Request\r\nContent-Length: %d\r\n\r\nBad Request\n", 12);
        return (EXIT_FAILURE);
    }

    // Perform the regex matching
    ret = regexec(&regex, version, 0, NULL, 0);

    if (ret == 0) {
        if (version[6] != '1' || version[8] != '1') {
            dprintf(R->response, "HTTP/1.1 505 Version Not Supported\r\nContent-Length: "
                                 "22\r\n\r\nVersion Not Supported\n");
            return (EXIT_FAILURE);
        }
    }

    else {
        //printf("NO match\n");
        dprintf(
            R->response, "HTTP/1.1 400 Bad Request\r\nContent-Length: %d\r\n\r\nBad Request\n", 12);
        return (EXIT_FAILURE);
    }

    // free the compiled regex
    regfree(&regex);

    // set the version
    R->version = "HTTP/1.1";

    return 0;
}

int parse_req(RequestObj *R, int socket) {
    if (R->buffer == NULL) {

        dprintf(
            R->response, "HTTP/1.1 400 Bad Request\r\nContent-Length: %d\r\n\r\nBad Request\n", 12);
        return (EXIT_FAILURE);
    }

    // initialize the fields to NULL
    R->method = NULL;
    R->URI = NULL;
    R->version = NULL;
    R->sock = socket;

    // compare the request buffer with expected values
    if (strncmp(R->buffer, "GET ", 4) == 0) {
        R->method = "GET ";

    } else if (strncmp(R->buffer, "PUT ", 4) == 0) {
        R->method = "PUT ";
    } else {
        dprintf(R->response,
            "HTTP/1.1 501 Not Implemented\r\nContent-Length: 16\r\n\r\nNot Implemented\n");
        return 1;
    }

    // find the position of the space character after the method
    char *version = strchr(R->buffer + 4, ' ');
    if (version == NULL) {
        dprintf(
            R->response, "HTTP/1.1 400 Bad Request\r\nContent-Length: %d\r\n\r\nBad Request\n", 12);
        return (EXIT_FAILURE);
    }
    // calculate the length of the URI
    int uri_length = version - (R->buffer + 4);

    // check for a valid URI length
    if (uri_length < 2 || uri_length > 64) {
        dprintf(
            R->response, "HTTP/1.1 400 Bad Request\r\nContent-Length: %d\r\n\r\nBad Request\n", 12);
        return (EXIT_FAILURE);
    }

    // copy the URI to the RequestObj
    R->URI = strndup(R->buffer + 4, uri_length);
    parse_uri(R, R->URI);

    if (strcmp(R->method, "GET ") == 0) {

        int getting = get(R, version);
        if (getting != 0) {
            return 1;
        }

    } else if (strcmp(R->method, "PUT ") == 0) {

        parse_version(R, version);
        int putting = put(R, R->sock);
        if (putting != 0) {
            return 1;
        }
    }

    return 0;
}

int get(RequestObj *R, char *version) {

    int parsed = parse_version(R, version);
    if (parsed != 0) {
        return (EXIT_FAILURE);
    }

    int is_valid = 1;
    if (R->URI == NULL) {
        is_valid = 1;
        dprintf(R->response, "HTTP/1.1 403 Forbidden\r\nContent-Length: 10\r\n\r\nForbidden\n");
        return (EXIT_FAILURE);
    } else { // if filename is too long
        size_t filename_length = strlen(R->URI);
        if (filename_length >= PATH_MAX) {
            is_valid = 1;
        } else { //  ensure valid file
            struct stat filename_info;
            if (stat(R->URI + 1, &filename_info) == 0) {
                is_valid = 0;
            }
        }
    }
    if (is_valid == 1) {
        dprintf(R->response, "HTTP/1.1 404 Not Found\r\nContent-Length: 10\r\n\r\nNot Found\n");
        return (EXIT_FAILURE);
    }

    int fd = open(R->URI + 1, O_RDONLY | __O_DIRECTORY); // check if directory, 403 error if so

    if (fd != -1) {
        close(fd);
        dprintf(R->response, "HTTP/1.1 403 Forbidden\r\nContent-Length: 10\r\n\r\nForbidden\n");
        return (EXIT_FAILURE);
    }

    fd = open(R->URI + 1, O_RDONLY);
    if (fd == -1) {
        if (errno == EACCES) {
            dprintf(R->response, "HTTP/1.1 403 Forbidden\r\nContent-Length: 10\r\n\r\nForbidden\n");
        } else if (errno == ENOENT) {
            dprintf(R->response, "HTTP/1.1 404 Not Found\r\nContent-Length: 10\r\n\r\nNot Found\n");
        } else {
            dprintf(R->response,
                "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 22\r\n\r\nInternal Server "
                "Error\n");
        }
        return (EXIT_FAILURE);
    }

    // calculates the size of the response
    off_t size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);

    dprintf(R->response, "HTTP/1.1 200 OK\r\nContent-Length: %ld\r\n\r\n", size);

    int bytes_written = pass_n_bytes(fd, R->response, size);

    if (bytes_written == -1) {
        dprintf(R->response,
            "HTTP/1.1 500 Internal Server Error\r\nContent-Length: %d\r\n\r\nInternal Server "
            "Error\n",
            22);
        return (EXIT_FAILURE);
    }
    close(fd);

    return 0;
}

int parse_header(RequestObj *R, char *buffer) {

    char input_copy[4096]; // allocate enough space.
    strcpy(input_copy, buffer);

    regex_t regex;
    regmatch_t matches[3]; // 3 matches: 0 for the whole match, 1 for key, and 2 for  value.

    // pattern to match header
    const char *pattern = "([^\r\n]+): (([\x20-\x7E]{0,128})[^\r\n]+)";

    // compile therregex
    if (regcomp(&regex, pattern, REG_EXTENDED) != 0) {
        fprintf(stderr, "Failed to compile regular expression\n");
        return 1;
    }

    int found_content_length = 1;
    char *input_ptr = input_copy; // separate pointer to iterate thru the string

    // execute regex to find matches
    while (regexec(&regex, input_ptr, 3, matches, 0) == 0) {
        char key[256]; // allocate enough space for the key and value
        char value[256];

        strncpy(key, &input_ptr[matches[1].rm_so], matches[1].rm_eo - matches[1].rm_so);
        key[matches[1].rm_eo - matches[1].rm_so] = '\0'; // null-terminate the key
        strncpy(value, &input_ptr[matches[2].rm_so], matches[2].rm_eo - matches[2].rm_so);
        value[matches[2].rm_eo - matches[2].rm_so] = '\0'; // null-terminate the value
        if (strncmp(key, "Content-Length", 24) == 0) {
            found_content_length = 0;
            R->content_length = strtol(value, NULL, 10);
        }

        // move the input_ptr to the character after the match
        input_ptr += matches[0].rm_eo;
    }
    if (found_content_length != 0) {
        fprintf(stderr, "HTTP/1.1 400 Bad Request\r\nContent-Length: %d\r\n\r\nBad Request\n", 12);
        exit(EXIT_FAILURE);
    }
    regfree(&regex);
    return 0;
}

int put(RequestObj *R, int socket) {

    int head = parse_header(R, R->buffer);
    if (head != 0) {
        exit(EXIT_FAILURE);
    }

    memset(R->buffer, '\0', BUFSIZE);

    int fd;
    int file_created = 1;

    fd = open(R->URI + 1, O_WRONLY);

    if (fd == -1) {
        // If it doesn't exist, create the file
        fd = open(R->URI + 1, O_WRONLY | O_CREAT, 0644);
        if (fd == -1) {
            close(fd);
            fprintf(stderr, "Error writing to file.\n");
            exit(EXIT_FAILURE);
        }
        file_created = 0;
    } else {

        fd = open(R->URI + 1, O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
        if (fd < 0) {
            close(fd);
            fprintf(stderr, "Error writing to file.\n");
            exit(EXIT_FAILURE);
        }
    }

    pass_n_bytes(socket, fd, R->content_length);

    if (file_created == 0) {
        dprintf(R->response, "HTTP/1.1 201 Created\r\nContent-Length: 8\r\n\r\nCreated\n");
    } else {
        dprintf(R->response, "HTTP/1.1 200 OK\r\nContent-Length: 3\r\n\r\nOK\n");
    }

    int bytes_read = 0;
    while (1) {
        bytes_read = read_n_bytes(socket, R->buffer, 4096);

        if (bytes_read < 0) {
            return -1;
        } else if (bytes_read >= 4096) {
            break;
        } else if (bytes_read == 0) {
            break;
        }
    }
    if (bytes_read == -1) {
        dprintf(
            R->response, "HTTP/1.1 400 Bad Request\r\nContent-Length: %d\r\n\r\nBad Request\n", 12);
    }
    memset(R->buffer, '\0', BUFSIZE);
    close(fd);
    return 0;
}
