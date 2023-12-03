# HTTP Server

Program that builds an HTTP Server. The server executes “forever” without crashing (i.e., it runs until a user types CTRL-C on the terminal). The server will create, listen, and accept connections from clients that arrive on a port. Server processes the bytes sent by clients using the HTTP protocol. 

The HTTP server that handles client connections, processes valid HTTP commands, and produces responses according to a defined set of rules. Key components involve setting up the server to listen for connections, accepting incoming connections, parsing HTTP requests, executing GET and PUT commands on valid URIs, and crafting responses in adherence to the HTTP protocol. Ensures resilience against malformed inputs and implementing error codes for different scenarios. Implements string manipulation, memory management, and efficient socket handling.

## Build

    $ make

## Start

    $ ./httpserver <port number>

## Running

    -T indicates PUT request, -o indicates GET request.
    $ curl http://localhost:1234/foo.txt -o download.txt
    $ curl http://localhost:1234/new.txt -T download.txt

## Format

    $ make format

## Files

#### httpserver.c

Main program, starts the httpserver

#### parser.c

Parses request and header and does all the reading/writing

#### parser.h

Header for parser

#### Makefile

Makefile contains the commands to compile, clean, and format the file.
