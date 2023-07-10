#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "lib/utils.h"

int main(int argc, char** argv)
{
	int listenfd, connfd;
	pid_t childpid;
	socklen_t clientaddrlen;
	struct sockaddr_in servaddr, clientaddr;


	if ( (listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		std::cerr << "Socket creation failed." << std::endl;
		exit(EXIT_FAILURE);
	}

	memset(&servaddr, 0, sizeof(servaddr));
	
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(7686);
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if ( bind(listenfd, (sockaddr*)&servaddr, sizeof(servaddr)) < 0 )
	{
		std::cerr << "Socket bind failed with error: " << strerror(errno) << std::endl;
		exit(EXIT_FAILURE);

	}
	if ( listen(listenfd, 16) < 0 )
	{
		std::cerr << "Socket listen failed with error: " << strerror(errno) << std::endl;
		exit(EXIT_FAILURE);
	}
	
	val::utils::signal(SIGCHLD, val::utils::sigchld_handler);
	clientaddrlen = sizeof(clientaddr);

	while (true)
	{
		if ( (connfd = accept(listenfd, (sockaddr*)&clientaddr, &clientaddrlen)) < 0)
		{
			if (errno == EINTR) continue;
			std::cerr << "Socket accept failed with error: " << strerror(errno) << std::endl;
			exit(EXIT_FAILURE);
		}
		if ((childpid = fork()) == 0)
		{
			std::cout << "Child process is waiting for data" << std::endl;
			close(listenfd);
			val::utils::str_echo(connfd);
			close(connfd);
			exit(EXIT_SUCCESS);
		}
		close(connfd);
	}


}
