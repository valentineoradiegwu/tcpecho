#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "lib/utils.h"

int main(int argc, char** argv)
{
	int connfd;
	if ( (connfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		std::cerr << "Socket creation failed." << std::endl;
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(7686);

        if (inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr) <= 0)
	{
		std::cerr << "Presentation to numeric translation failed." << std::endl;
		exit(EXIT_FAILURE);

	}

	if (connect(connfd, (sockaddr*)&servaddr, sizeof(servaddr)) < 0)
	{
		std::cerr << "Connection failed with error: " <<  strerror(errno) << std::endl;
		exit(EXIT_FAILURE);
	}

	std::cout << "Enter a text to echo or SHUTDOWN to close client." << std::endl;
	val::utils::str_cli_select(connfd);
	close(connfd);
	return 0;
}
