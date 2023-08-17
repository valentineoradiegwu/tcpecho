#include <iostream>
#include <string>
#include <cstring>
#include <algorithm>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>
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

	struct pollfd watched_fds[MAX_CONNECTIONS];
	int max_idx = 0;
	int i;

	watched_fds[max_idx].fd = listenfd;
	watched_fds[max_idx].events = POLLRDNORM;

	for (i = 1; i < MAX_CONNECTIONS; ++i)
		watched_fds[i].fd = -1;

	while (true)
	{
		auto nready = poll(watched_fds, max_idx + 1, -1);

		if (watched_fds[0].revents & POLLRDNORM)
		{
			if ((connfd = accept(listenfd, nullptr, nullptr)) < 0)
			{
				if (errno == EINTR) continue;
				std::cerr << "Socket accept failed with error: " << strerror(errno) << std::endl;
				exit(EXIT_FAILURE);
			}

			for (i = 1; i < MAX_CONNECTIONS; ++i)
			{
				if(watched_fds[i].fd == -1)
				{
					watched_fds[i].fd = connfd;
					watched_fds[i].events = POLLRDNORM;
					break;
				}
			}

			if (i == MAX_CONNECTIONS)
			{
				std::cerr << "Maximum supported connections has been reached." << std::endl;
				close(connfd);
				continue;
			}

			max_idx = std::max(max_idx, i);

			if (--nready <= 0)
				continue;
		}
		else
		{
			for (i = 1; i < max_idx + 1; ++i)
			{
				if (watched_fds[i].fd != -1 && watched_fds[i].revents & (POLLRDNORM|POLLERR))
				{
					const int bytes_read = read(watched_fds[i].fd, buffer, MAXLINE);
					if (bytes_read > 0)
					{
						val::utils::writen(watched_fds[i].fd, buffer, bytes_read);
					}
					else if (bytes_read == 0)
					{
						close(watched_fds[i].fd);
						watched_fds[i].fd = -1;
					}
					else
					{
						if (errno == ECONNRESET)
						{
							close(watched_fds[i].fd);
							watched_fds[i].fd = -1;
						}
						else
						{
							std::cerr << "Socket read failed with: " << strerror(errno) << std::endl;
							exit(EXIT_FAILURE);
						}

					}

					if (--nready <= 0)
						break;
				}
			}
		}
	}


}
