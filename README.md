# Chat-Server

In this exercise we implemented a simple chat server using TCP and select().
When the server reads a message from the client, it reads it till a new line appears.
The server can talk with many clients, each on a different socket.

how to use it:
run ./server <port>

functions in chatserver.c:
* main - open socket and connection to the server, wait for clients to conncet , and handle the masseges.
* sig_handler - this function handle signals.
* isEmpty - check if the linked list is empty.
* slist_init - this method initilaize the lise.
* checkInput - this function check the user input, if the port is valid.
* error - the method handle all errors in the program, and free memory.
* goOverList - the method go over all file discreptors in the list and do FD_SET , and find the maxfd.
* readwrite - the method is going over the list and read from all file descriptors masseges and the server write to all clients the masseges that recived.
* add - this method add a new node to the linked list.
* removeFromList - the method remove spesific node that hold a file descriptor from the active list--the file descriptor is no longer active.
* slist_destroy - the method free the list from memory.


Thank you!
