#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <bits/stdc++.h>
#include <fstream>
#include<ctype.h>
#include <thread>
#include <pthread.h>
#include <errno.h>
using namespace std;

//#define PORT "8080"
#define BACKLOG 20
#define MAXDATASIZE 1024 // max number of bytes we can get at once 
#define MAXCLIENTS 100
atomic<int> counter(0);
string readFile(string p){
	ifstream file;
    file.open(p);
    stringstream stream;
    stream << file.rdbuf();
	file.close();
	return stream.str();
}
void saveFile(string p, string body){
	ofstream file(p);
    file << body;
    file.close();
}
string bufToString(char buf[], int size){
	// Extract the request from the buffer
	string res = "";
	for(int i=0; i<size; i++)
		res.push_back(buf[i]);
	return res;
}
bool notEmpty(string s){
	// Used to see if this line is the end of the headers
	int sz = s.length();
	while(sz-->0)
		if(isalpha(s[sz]))
			return true;
	return false;
}
int getLen(string req){
	// Get content-length value
	int lenIndex = req.find("Content-Length: ") + 16;
	int bodyLen = 0;
	while(isdigit(req[lenIndex])){
		bodyLen = bodyLen * 10 + (req[lenIndex] - '0');
		lenIndex++;
	}
	return bodyLen;
}
string extractBody(string req){
	string temp;
	do{
		int i = req.find('\n');
		temp = req.substr(0, i+1);
		req = req.substr(i+1, req.size()-i-1);
	} while(notEmpty(temp));
	return req;
}
void threadFunc(int new_fd){
	// Handling all user requests until connection is closed
	int numBytes;
	char buf[MAXDATASIZE];
	while(1){
		if ((numBytes = recv(new_fd, buf, MAXDATASIZE-1, 0)) == -1) {
			if(errno == ETIMEDOUT || errno == EAGAIN || errno == EWOULDBLOCK || errno == EINPROGRESS)
				cout << "------------------" << endl << "Time out." << endl;
			else 
				cout << "------------------" << endl << "Connection closed from client." << endl;
			close(new_fd);
			counter--;
			return;
		}
		if(errno == ETIMEDOUT|| errno == EAGAIN || errno == EWOULDBLOCK || errno == EINPROGRESS){
			cout << "------------------" << endl << "Time out." << endl;
			close(new_fd);
			counter--;
			return;
		}
		if(numBytes == 0){
			// Connection closed from user
			cout << "------------------" << endl << "Client closed the connection." << endl;
			break;
		}
		buf[numBytes] = '\0';// just to be safe if buf is put into printf
		string req = bufToString(buf, numBytes);
		int bodyLen = getLen(req);
		string recievedBody = extractBody(req);
		int tempLen = recievedBody.length();//length of recieved body part
		while(tempLen < bodyLen){
			// Re-recieve until the message is completed
			numBytes = recv(new_fd, buf, MAXDATASIZE-1, 0);
			tempLen += numBytes;
			req += bufToString(buf, numBytes);
		}
		cout << "------------------" << endl << "Server recieved:" << endl << req << endl;
		int i = req.find(' ');
		string part = req.substr(0, i);
		req = req.substr(i+1, req.size()-i-1);
		i = req.find(' ');
		string path = req.substr(0, i);
		req = req.substr(i+1, req.size()-i-1);
		i = req.find('\n');
		req = req.substr(i+1, req.size()-i-1); // ignore the version
		if(part.compare("POST") == 0){
			req = extractBody(req);
			req = req.substr(0, req.length()-2);
			saveFile(path.substr(1, path.length()-1), req);
			string res = "HTTP/1.1 200 OK\r\n\r\n";
			send(new_fd, res.c_str(), res.length(), 0);
		}else{ // GET
			string code = "404 Not Found";
			string body = "";
			FILE * file;
			if (file = fopen(path.substr(1, path.length()-1).c_str(), "r")) {
				// file is found and can be opened.
				fclose(file);
				code = "200 Ok";
				body = readFile(path.substr(1, path.length()-1));
			}
			string res = "HTTP/1.1 " + code + "\r\n"
				+ "Content-Length: " + to_string(body.length()) + "\r\n"
				+ "\r\n"
				+ body;
			send(new_fd, res.c_str(), res.length(), 0);
		}
	}
	close(new_fd);
	counter--;
}
int main(int argc, char const *argv[]){
	if(argc < 1){
		cout << "Error: Port Number is missing" << endl;
		return 1;
	}
    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage connector_addr;
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	int status;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // Using IPv4
    hints.ai_socktype = SOCK_STREAM; // Using TCP
    hints.ai_flags = AI_PASSIVE; // Auto fill my IP

    if ((status = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		return 1;
	}
    // bind with the first possible
    for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}
        // solving error 'already in use'
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}
        // binding the socket
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}
        // success or all failed
		break;
	}

    freeaddrinfo(servinfo); // no longer needed

    // if all failed
    if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}

    // listening on the port with queue of size BACKLOG
	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}
	printf("server: waiting for connections...\n");
    // accepting connections part
    while(1) {
		sin_size = sizeof connector_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&connector_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}
		inet_ntop(connector_addr.ss_family,
            &(((struct sockaddr_in*)&connector_addr)->sin_addr),
			s, sizeof s);
		counter++;
		struct timeval timeout;
		timeout.tv_sec = 0;
		// Notice counter is at minimum = 1 so no division over zero
		timeout.tv_usec = 1000000 + 4000000 * (MAXCLIENTS/counter);
		setsockopt(new_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout,sizeof timeout);
		thread answeringThread (threadFunc, new_fd);
		answeringThread.detach();
	}
	close(sockfd);
    return 0;
}
