Step 1: Designing the Thread Structure
Keyboard Input Thread:
Use pthread_create to create a thread that waits for keyboard input.
This thread should read input from the user and add it to a shared list of messages.
UDP Output Thread:
Create another thread that sends messages from the list to the remote client using UDP.
Use a socket for sending UDP messages (socket, sendto).
UDP Input Thread:
Create a thread that waits for UDP datagrams from the remote client.
Upon receiving a message, add it to the shared list of messages.
Screen Output Thread:
Create a thread that prints messages from the list to the local screen.
Ensure proper synchronization using mutexes and condition variables.
Step 2: Implementing Socket Communication
Creating Sockets:
Use socket to create a socket for communication.
Use bind to bind the socket to a specific port.
Sending Messages:
Use sendto to send messages to the remote client.
Receiving Messages:
Use recvfrom to receive messages from the remote client.
Step 3: Data Structure for Shared List
List ADT:
Use the list ADT to manage the shared list.
Mutexes and Condition Variables:
Implement mutexes to control access to the shared list.
Use condition variables for signaling/waiting to avoid busy waiting.
Step 4: Integrating Threads and Socket Communication
Initializing Threads and Sockets:
In your main function, initialize threads and sockets.
Passing Information:
Make sure the threads have access to the necessary information (e.g., socket descriptors, list pointers).
User Input:
Parse user input to get the required information for the connection (port number, remote machine).
Step 5: Testing
Local Testing:
Test your s-talk locally first before trying to communicate between different machines.
Ensure that threads are synchronized and communication is working as expected.
Remote Testing:
Once local testing is successful, try communicating between two machines.

List ADT:
Critical section:
	Trim for write()
	Prepend for receiver()
	Use the pthreads mutexes and conditions for the CS
