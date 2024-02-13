# HTTP Web Server README

This project implements a simple HTTP web server in C that supports both HTTP 1.0 and HTTP 1.1 protocols. The server listens for connections on a specified port, allowing clients to retrieve files using a text-based protocol.

## Features

- Supports HTTP/1.0 and HTTP/1.1 protocols.
- Implements concurrency using a multi-threading approach for HTTP/1.1 requests.
- Sets a timeout for idle connections to close after 30 seconds.
- Handles various HTTP status codes:
  - 200: Successful request execution.
  - 404: File not found error.
  - 403: Forbidden request error.
  - 400: Bad request error.
- Keeps connections open for error scenarios to allow clients to resend requests.

## Implementation Details

- Utilizes a fixed-size buffer to store client requests.
- Parses client requests to execute appropriate actions.
- Addresses challenges such as memory management and request parsing.
- Evaluated server performance using ApacheBench tool, demonstrating concurrency support.
