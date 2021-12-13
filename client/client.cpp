#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <bits/stdc++.h>
#include <fstream>

using namespace std;

//#define PORT "8080"
#define HOST_NAME "localhost"
#define MAXDATASIZE 1024 // max number of bytes we can get at once 

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
bool notEmpty(string s){
	// Used to see if this line is the end of the headers
	int sz = s.length();
	while(sz-->0)
		if(isalpha(s[sz]))
			return true;
	return false;
}
string bufToString(char buf[], int size){
	// Extract the request from the buffer
	string res = "";
	for(int i=0; i<size; i++)
		res.push_back(buf[i]);
	return res;
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
int main(int argc, char const *argv[]){
	if(argc < 1){
		cout << "Error: Port Number is missing" << endl;
		return 1;
	}
    int sockfd, numbytes;  
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int status;
	char s[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET; // Using IPv4
    hints.ai_socktype = SOCK_STREAM; // Using TCP
	if ((status = getaddrinfo(HOST_NAME, argv[1], &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		return 1;
	}
    
	// connect with the first possible
	for(p = servinfo; p != NULL; p = p->ai_next) {

		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			perror("client: connect");
			close(sockfd);
			continue;
		}
        // success or failed all
		break;
	}
    // if failed all
	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, &((struct sockaddr_in*)&p->ai_addr)->sin_addr,
			s, sizeof s);
    
	printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // no longer needed

	string req;
	ifstream myfile ("requests.txt");
	if ( ! myfile.is_open()) cout << "Unable to open file" << endl; 
	while (getline(myfile,req)){
		cout << "------------------" << endl << "Currently requesting: " << req << endl;
		int i = req.find(' ');
		string part = req.substr(0, i);
		req = req.substr(i+1, req.size()-i-1);
		i = req.find(' ');
		string path = req.substr(0, i);
		string msg;
		string res;
		if(part.compare("POST") == 0){
			FILE *file;
			if ( !(file = fopen(path.substr(1, path.length()-1).c_str(), "r")) ) {
				cout << "------------------" << endl << "File: "<< path 
				<< " is not found on client." << endl << "Ignoring this request." << endl;
				continue;
			}
			fclose(file);
			string body = readFile(path.substr(1, path.length()-1));
			msg = "POST "+path+ " HTTP/1.1\r\n"
				+ "Content-Length: " + to_string(body.length()) + "\r\n"
				+ "Connection: keep-alive\r\n"
				+ "\r\n"
				+ body;
			send(sockfd, msg.c_str(), msg.length(), 0);
			// recieving the response
			int numBytes;
			if ((numBytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
				perror("recv");
				exit(1);
			}
			if(numBytes == 0){
				// Connection closed from server
				cout << "------------------" << endl << "Server closed the connection." << endl;
				break;
			}
			buf[numBytes] = '\0';
			res = bufToString(buf, numBytes);
			cout << "------------------" << endl << "Client got response:"<< endl << res << endl;
		}else{
			msg = "GET "+path+ " HTTP/1.1\r\n"
				+ "Content-Length: 0\r\n"
				+ "Connection: keep-alive\r\n"
				+ "\r\n";
			send(sockfd, msg.c_str(), msg.length(), 0);
			int numBytes;
			if ((numBytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
				perror("recv");
				exit(1);
			}
			if(numBytes == 0){
				// Connection closed from server
				cout << "------------------" << endl << "Server closed the connection." << endl;
				break;
			}
			buf[numBytes] = '\0';
			string res = bufToString(buf, numBytes);
			int bodyLen = getLen(res);
			cout << bodyLen << endl;
			string recievedBody = extractBody(res);
			int tempLen = recievedBody.length();//length of recieved body part
			while(tempLen < bodyLen){
				numBytes = recv(sockfd, buf, MAXDATASIZE-1, 0);
				tempLen += numBytes;
				res += bufToString(buf, numBytes);
			}
			cout << "------------------" << endl <<"Client got response:"<< endl << res << endl;
			int i = res.find(' ');
			string version = res.substr(0, i);
			res = res.substr(i+1, res.size()-i-1);
			i = res.find(' ');
			string code = res.substr(0, i);
			if(code.compare("200")==0){//success
				res = extractBody(res);
				res = res.substr(0, res.length()-2);
				saveFile(path.substr(1, path.length()-1), res);
			}
		}
	}
	myfile.close();
	close(sockfd);
    return 0;
}