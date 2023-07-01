# tcp-udp-client-server-subscribe-application

Time to implement:25h

For testing:
  sudo python3 test.py
Manually:
  ./server <Port>
  ./client <ID_CLient> <IP_Server> <Port_server>

# Application that respects the client-server model for message management.
 Objectives:
-  understanding the mechanisms used to develop network applications using TCP and UDP;
-  multiplexing of TCP and UDP connections;
-  defining and using a data type for a predefined protocol over UDP;
-  evelopment of a client-server application that uses sockets.

# We have three components in the platform:
-  the (single) server makes the connection between the clients in the platform, with the purpose of publishing and subscribing to messages;
-  TCP clients will have the following behavior: a TCP client connects to the server, can receive (at any time) from the keyboard (interaction with the human user) subscribe type commands
and unsubscribe and display the messages received from the server on the screen;
-  UDP clients publish, by sending to the server, messages in the proposed platform using a predefined protocol.

- The functionality is for each TCP client to receive from the server those messages, coming from
UDP clients, who refer to the topics to which they are subscribed
- The application also includes a SF (store-and-forward) functionality regarding the messages sent when
TCP clients are disconnected.

# Generalities:
- We split the port as a number
-  We get a TCP, respectively UDP socket for receiving connections
-  Fill in the server address, address family and port in serv_addr
-  We make the socket address reusable, so that we don't get an error in case we run 2 times fast
- Associate the server address with the socket created using bind
- setting the tcpsocket socket for listening
- the new file descriptor (the socket on which connections are listened to (TCP, UDP, STDIN_FILENO)) is added to the poll_fds set
- I used the functions send_all and recv_all
- Also, to make sure that the number of recieved / sent bytes is the one we
expect, we make the sending and the recieving in a loop, taking into account
the number of remaining bytes to send / recieve, and the number of bytes
already sent / recieved. This way, we make sure that we recieve the whole
expected message in case the message gets split mid transmission or the number
of sent / recieved bytes is somehow capped.

# subscriber (TCP):
- if I received a message from the STDIN socket, I check to see if the message is exit
- if so, I stop the client
- otherwise, I received a subscribe/unsubscribe command - I send the entire buffer on (in the server I take care of parsing it)
 - if I received a message from the socket with the server, I check to see if the message is exit (if so, I stop the client)
- otherwise, I received the message from UDP and I am displaying it directly (I did the parsing of the message on the server)

## udp message parsing:
- I use "memcpy" to parse the buffer received from udp clients
- this way I extract the topic, the type of data and the content
- depending on the type of data received, I create the required values according to the statement
- to send all the required data to the client, I put the data in a buffer using "sprintf"

### I used a customer structure to store all the data about a customer
- in the id field, keep the id
- in the nr_topics field, I keep for each client the number of topics to which he is subscribed
- I use the verification field to remember that I have to receive the id from the client
after a connection request came on the inactive socket (tcp), I accept it and change the "verification" field
 when I will receive data on one of the client sockets, and the verification field is set to 1, then the data contains the id
- in the client_addr field I use it to keep the client's address and IP
- the last field is a "subscription structure" type vector
- in the subscription structure, for each topic: topic: the actual topic ||
                         unsenttopic: all messages that have not been sent ||
                         SF : the sf received as a parameter
- I implemented the store and forward part with the help of this structure
                         for each topic, if the client was offline, I put the message in the vector "unsenttopic" (concatenate)

### in the implementation I created 2 vectors for the client structure
- one I used for clients who are online
- the other for customers who are offline
- when a client disconnects, we put all the data from the vector for online clients into it
- I used a function to copy all the data from one vector to the other (get_all_data function)
- after disconnecting, we reinitialize the client index from the vector for online clients with zero
                     to be used by the next client (set_zero function)
- for a client that reconnects, we check the received id with those in the vector for offline clients
                     if we found a match, we copied all the data back, then the index was removed from the vector for offline clients

# server
## if I received a message on the udp socket
- I parse the message using the function above for customers who are offline, I check what topic SF 1 has
- for topics with SF 1, I put the messages in the "unsenttopic" vector to retain them
- for clients who are online, I check which client is subscribed to the topic sent by udp
- for subscribers I send the message on the socket
- if I received a message on the STDIN socket
## if the message contains exit, the clients and the server stop
        
## if I received a message on the tcp socket
- I accept the connection
- the new socket returned by accept() is added to the set of read descriptors
-  change the verification field

## otherwise data was received on one of the client sockets
### if the verification field is set to one, then the received data contains the id
- if the received id is already in the customers who are online close the connection with the current socket
- I'm looking to see if the current id is among the old ones
- if so, I put the old data in the structure for the online client (data from the vector for offline clients)
- I send all retained messages
- otherwise it is a new customer
                
### otherwise he received an order from the customer
- I parse the message using sscanf
   ### if the message is to subscribe:
- if the desired topic already exists and has a different name, then I will change it
-  otherwise I add it to the structure
   ### if the message is to unsubscribe:
- I check to see if the topic is in the structure
- the topic is removed from the type vector (subscribers structure)
### in both cases, I send the confirmation message on the socket


  
