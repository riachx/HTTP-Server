#include "asgn2_helper_funcs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>

#define BUFSIZE 4096

typedef struct RequestObj {
    char *buffer;
    char *method;
    char *URI;
    char *version;
    int content_length;
    int response;
    int sock;
    int read_bytes;

} RequestObj;

int get(RequestObj *R, char *space_pos);

int put(RequestObj *R, int socket);

int parse_version(RequestObj *R, char *version);

int parse_req(RequestObj *R, int socket);

int parse_header(RequestObj *R, char *buffer);
