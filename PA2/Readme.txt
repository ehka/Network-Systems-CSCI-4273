#Ehsan Karimi


#This is Programming Assignment 2 for Network systems.

-Language used is c.

-Execute with 'gcc server.c -o server' & then ./server. Then type localhost:{port-number}/{resource}
in the web browser. Run the server inside 'www' folder.

-NOTE: it has been tested only on google chrome.


-How Program Works:

{This is a webserver that handles http requests concurrently using new process with fork().
the program parses the config file in the beginning and stores the value in the designated
struct fields. The webserver parses each HTTP request and take appropriate action based the 
the given information in the config file. 
If the HTTP request is default the program will use the file index.html for response.}

-Everything else is according to the PA2 writeup.

-No extra credit implemented. 


