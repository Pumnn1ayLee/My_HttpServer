# Simple HTTP Server

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen)](https://shields.io/)
[![License](https://img.shields.io/badge/license-MIT-blue)](./LICENSE)
[![GitHub stars](https://img.shields.io/github/stars/yourusername/yourrepository)](https://github.com/yourusername/yourrepository/stargazers)
[![GitHub forks](https://img.shields.io/github/forks/yourusername/yourrepository)](https://github.com/yourusername/yourrepository/network)
[![GitHub issues](https://img.shields.io/github/issues/yourusername/yourrepository)](https://github.com/yourusername/yourrepository/issues)
[![GitHub contributors](https://img.shields.io/github/contributors/yourusername/yourrepository)](https://github.com/yourusername/yourrepository/graphs/contributors)

This is a simple HTTP server written in C using the Windows Sockets API (Winsock). It demonstrates basic networking concepts such as socket creation, binding, listening, and accepting connections.

## Features

- Handles GET and POST requests
- Serves static files from the `htdocs` directory
- Supports multiple MIME types
- Returns 404 Not Found for missing resources
- Multithreaded to handle multiple clients

## Getting Started

### Prerequisites

- Windows OS
- [Winsock2 library](https://docs.microsoft.com/en-us/windows/win32/api/winsock/)

### Compilation

To compile the program, use a C compiler like `gcc` or `cl`.

```bash
gcc -o http_server http_server.c -lws2_32

### Running the Server

After compiling, you can run the server with the following command:

```bash
http_server
```

By default, the server will listen on port `8080`. You can change the port by modifying the `port` variable in the `main` function.

### Directory Structure

The server expects a directory structure as follows:

```
.
├── http_server.exe
└── htdocs
    ├── index.html
    └── 404.html
```

Ensure that the `htdocs` directory contains the `index.html` file for the server to serve.

## Usage

1. Start the server by running the executable.
2. Open a web browser and navigate to `http://localhost:8080`.

### Handling Requests

- **GET**: The server handles GET requests and serves the requested file if it exists.
- **POST**: The server currently does not process POST data but acknowledges the request.

## Functions

### `int startup(unsigned short *port)`

Initializes the Winsock library, creates a socket, binds it to a port, and starts listening for incoming connections.

### `int get_line(int sock, char* buff, int size)`

Reads a line of data from the specified socket.

### `void cat(int client, FILE *resource)`

Sends the contents of a file to the client.

### `void headers(int client, const char* type)`

Sends the HTTP headers to the client.

### `void unimplement(int client)`

Sends a 501 Not Implemented response to the client.

### `void not_found(int client)`

Sends a 404 Not Found response to the client.

### `const char* getHeadType(const char* fileName)`

Determines the MIME type based on the file extension.

### `void server_file(int client, const char* fileName)`

Serves the requested file to the client.

### `DWORD WINAPI accept_request(LPVOID arg)`

Handles incoming client requests in a new thread.
