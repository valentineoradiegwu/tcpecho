#include <iostream>
#include <string>
#include <cstring>
#include <algorithm>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>
#include <sys/select.h>
#include "lib/utils.h"

static constexpr int MAX_CONNECTIONS = 36;
static constexpr size_t MAXLINE = 40;

int main(int argc, char** argv)
{
	int listenfd, connfd;
	struct sockaddr_in servaddr;
	char buffer[MAXLINE];

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

	if ( listen(listenfd, MAX_CONNECTIONS) < 0 )
	{
		std::cerr << "Socket listen failed with error: " << strerror(errno) << std::endl;
		exit(EXIT_FAILURE);
	}

	fd_set all_fds;
	FD_ZERO(&all_fds);
	FD_SET(listenfd, &all_fds);
	int maxfd = listenfd;

	std::vector<int> client_connections{};
	client_connections.reserve(MAX_CONNECTIONS);

	while (true)
	{
		fd_set read_set = all_fds;
		auto nready = select(maxfd + 1, &read_set, nullptr, nullptr, nullptr);

		if (FD_ISSET(listenfd, &read_set))
		{
			if ((connfd = accept(listenfd, nullptr, nullptr)) < 0)
			{
				if (errno == EINTR) continue;
				std::cerr << "Socket accept failed with error: " << strerror(errno) << std::endl;
				exit(EXIT_FAILURE);
			}

			if (client_connections.size() == MAX_CONNECTIONS)
			{
				std::cerr << "Maximum supported connections has been reached." << std::endl;
				close(connfd);
				continue;
			}

			FD_SET(connfd, &all_fds);
			maxfd = std::max(maxfd, connfd);
			client_connections.push_back(connfd);

			if (--nready <= 0)
				continue;
		}
		else
		{
			for (auto fd = client_connections.begin(); fd != client_connections.end(); ++fd)
			{
				if (FD_ISSET(*fd, &read_set))
				{
					if (int bytes_read = read(*fd, buffer, MAXLINE))
					{
						val::utils::writen(*fd, buffer, bytes_read);
					}
					else
					{
						close(*fd);
						FD_CLR(*fd, &read_set);
						client_connections.erase(fd);
					}

					if (--nready <= 0)
						break;
				}
			}
		}
	}


}
