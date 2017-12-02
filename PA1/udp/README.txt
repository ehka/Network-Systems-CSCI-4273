Ehsan Karimi PA1:

RUNNING THE PROGRAM:
Server Side:
In the server folder run "make" to compile udp_server.c file. To run the server use ./server <port Usage>. 
The server will continue to run until either receiving "exit" command from the client or by "CNTRL+C".

Client Side:
run 'make' to compile udp_client.c file. To execute the client use ./client <IP address> <port usage>.
The client will exit after execution of each command. to run the client again use the execution command above.

FUNCTIONALITY:
the server supports the following commands as eplained in the PA1:
get [file_name]
put [file_name]
delete [file_name]
ls
exit

for get & delete commands to work properly please store the files in the server folder.(Same logic for put command and client folder.)

The program supports files as large as 6KB for file transfers.


