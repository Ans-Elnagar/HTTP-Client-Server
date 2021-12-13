# HTTP-Client-Server
Client and Server implementation with C++. Exchanging HTTP requests and responses
# How server works: (Multi-threaded)
  The server starts with reserving the port that is
supposed to run on in the main thread. Then the main
thread waits for clients to start the connection. For every
new connection, a socket is reserved and a new thread is
created to use this socket and answer requests on this
socket with a timeout value specified depending on the
number of currently running connections while the main
thread returns to wait for new connections. \
  The created thread then starts to listen on the socket
for requests. When a request is received, the thread first
checks if the request is received fully by comparing the
value of the header Content-Length and the size of the
received body if not then it receives again until the
request is fully received. Then the thread starts parsing the
request and determines if it was a POST or a GET
request. \
  If it was a POST request: The thread extracts the file
path and content (body of the request) and saves them.
Then it composes a 200 Ok response and sends it to the
client.If it was a GET request: The thread first check if the
file exists if not it composes a 404 Not Found response
and sends it to the client. I the file is found, The thread
reads the file and composes a 200 Ok response with the
content of the file in the body and the size of the in the
Content-Length header. \
  After completing the received request the thread
waits for more request on this socket until a timeout
occurs or the client closes the connection from its side
then the thread closes its socket and terminate.
# How client works:
The client starts with connecting to the server and
opens the file requests.txt to get the requests from it. For
every request in the requests.txt file, the client composes
the appropriate HTTP request for it. \
  For POST requests the client checks if the file exists
if not it will print a message in the console and skips this
request to handle the next request in the file. If it exists
then the client reads the content of the file and puts it in
the request body, sets the Content-Length header to its
size, and sends the full request to the server. \
  For GET requests the client composes the appropriate
HTTP request and sends it to the server. When receiving
the response it just does as the server thread did in
checking if the message is fully received and re-receives
until it is fully received. If the response code is 200 then
the client extracts the file content from the response body
and saves it.
