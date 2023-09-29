# ChatApp

This project consists of a chat server (chat_server.cpp) and chat client(s) (chat_client.cpp).

Building and running chat_server.cpp starts up a chat server on the specified port and ip (default port = 3490, default ip = 127.0.0.1).

Building and running chat_client.cpp starts up a chat client which can connect to the chat server via the port and ip specified by the server.

The chat server supports a maximum number of users specified by `MAX_USERS`.
On establishing a connection with a new chat client, a separate thread is created to handle communication with that thread until the connection is closed by either side.
Clients can communicate with each other via the server. The server supports the following commands:
- `!quit`: disconnect from the server.
- `!users`: get a list of online users.
- `@username msg`: send a message `msg` to user `username`.

Each chat client creates two separate threads: one watching stdin for output and another watching the socket for input.
