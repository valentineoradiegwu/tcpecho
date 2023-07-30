#include "utils.h"
#include "sum.h"

#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string>
#include <iostream>
#include <sys/wait.h>
#include <sys/select.h>

namespace val::utils
{
	static constexpr size_t MAXLINE = 40;
	static char READBUFFER[MAXLINE];
	static ssize_t read_count = 0;
	static char* read_ptr = 0;

	ssize_t readn(int fd, void* buff, size_t nBytes)
	{
        	size_t remaining = nBytes;
		char* buff_start = (char*)buff;
		size_t bytes_read = 0;
		while (remaining > 0)
		{
			if( (bytes_read = read(fd, buff_start, remaining) ) < 0)
			{
				if (errno == EINTR)
					continue;
				else return -1;
			}
			else if (bytes_read == 0)
			{
				break;
			}
			remaining -= bytes_read;
			buff_start += bytes_read;
		}
		return nBytes - remaining;
	}
	ssize_t writen(int fd, const void* buff, size_t nBytes)
	{
		
        	size_t remaining = nBytes;
		char* buff_start = (char*)buff;
		size_t bytes_written = 0;
		while (remaining > 0)
		{
			if ( (bytes_written = write(fd, buff_start, remaining) ) < 0)
				if (errno == EINTR)
					continue;
				else return -1;
			remaining -= bytes_written;
			buff_start += bytes_written;
		}
		std::cout << "Socket has written " << nBytes << " Back to peer" << std::endl;
		return nBytes - remaining;
	}

	static ssize_t read_char(int fd, char* p)
	{
		if (read_count == 0)
		{
			//There is nothing in the local buffer so read from the socket
again:
			if ( (read_count = read(fd, READBUFFER, MAXLINE)) < 0)
			{
				if (errno == EINTR)
					goto again;
				else return -1;
			}
			else if (read_count == 0)
				return 0;
			read_ptr = READBUFFER;
		}
		*p = *read_ptr++;
		--read_count;
		return 1;
	}
        ssize_t readline(int fd, void* buff)
	{
		char c;
		char* buff_start = (char*)buff;
		ssize_t rc;
		size_t i = 0;

		for (; i < MAXLINE; ++i)
		{
			if ( (rc = read_char(fd, &c) ) == 1)
			{
				*buff_start++ = c;
				if (c == '\n')
					break;
			}
			else if (rc == 0)
			{
				*buff_start = 0;
				return i;
			}
			else
				return -1;
		}
		*buff_start = 0;
		return i+1;
	}

	void str_echo(int fd)
	{
		/*
		 * read n bytes from the connection socket
		 * Send back all the bytes you've read using writen
		 * We continue to read until 0 is returned (i.e socket is closed)
		 * or -1 returned i.e socket error (In this case print error and exit)
		*/
		char buff[MAXLINE];
		ssize_t bytes_read;
again:
		while((bytes_read = read(fd, buff, MAXLINE)) > 0)
		{
			std::cout << "Server received message from client of " << bytes_read << " bytes." << std::endl;
			writen(fd, buff, bytes_read);
		}
		if (bytes_read < 0) 
		{
			if (errno == EINTR)
				goto again;
			std::cerr << "Read failed with error: " << strerror(errno) << std::endl;
			exit(EXIT_FAILURE);
		}
	}

	/*
	 * Does not work in the following scenarios
	 * 1. The sending and receiving hosts have different byte orders.
	 * 2. The size of long is different in the hosts.
	 * 3. The structs are packed differently in the hosts.
	 */
	void str_echo_binary(int fd)
	{
		ssize_t bytes_read;
		struct args args;
	        struct result res;

		if ((bytes_read = readn(fd, &args, sizeof(args))) > 0)
		{
			std::cout << "Server received message from client of " << bytes_read << " bytes." << std::endl;
			res.sum = args.arg1 + args.arg2;
			writen(fd, &res, sizeof(res));
		}
	}

	void str_cli(int fd)
	{
		std::string line;
		char rcv_line[MAXLINE];
		while(std::getline(std::cin, line))
		{
			if (line == "SHUTDOWN")
				break;
			line += '\n';
			val::utils::writen(fd, line.c_str(), std::min(line.size(), MAXLINE));
			if(val::utils::readline(fd, rcv_line) == 0)
			{
				std::cerr << "Could not read from Server. Terminating" << std::endl;
				exit(EXIT_FAILURE);
			}
			std::cout << rcv_line;
		}
	}

	/*
	 * Does not work in the following scenarios
	 * 1. The sending and receiving hosts have different byte orders.
	 * 2. The size of long is different in the hosts.
	 * 3. The structs are packed differently in the hosts.
	 */
	void str_cli_binary(int fd)
	{
		std::string line;

		struct args args;
		struct result res;

		while(std::getline(std::cin, line))
		{
			if (line == "SHUTDOWN")
				break;
			line += '\n';
			if(sscanf(line.c_str(), "%ld%ld", &args.arg1, &args.arg2) != 2)
			{
				std::cerr << "Two operands are required for multiplication. Try again!" << std::endl;
				continue;
			}

			val::utils::writen(fd, &args, sizeof(args));
			if(val::utils::readn(fd, &res, sizeof(res)) <= 0)
			{
				std::cerr << "Could not read from Server. Terminating" << std::endl;
				exit(EXIT_FAILURE);
			}
			std::cout << res.sum << std::endl;
		}
	}

	void str_cli_select(int fd)
	{
		std::string line;
		char rcv_line[MAXLINE];
		const int cin_fd = fileno(stdin);
		const int maxfdp1 = std::max(fd, cin_fd) + 1;
		fd_set read_set;
		FD_ZERO(&read_set);

		while(true)
		{
			FD_SET(fd, &read_set);
			FD_SET(cin_fd, &read_set);

			//What if select is awaken by a signal? We should test for EINTR
			select(maxfdp1, &read_set, nullptr, nullptr, nullptr);
			if (FD_ISSET(cin_fd, &read_set))
			{
				if (std::getline(std::cin, line))
				{
					if (line == "SHUTDOWN")
						break;
					line += '\n';
					val::utils::writen(fd, line.c_str(), std::min(line.size(), MAXLINE));
				}
				else
					break;
			}
			if (FD_ISSET(fd, &read_set))
			{
				if(val::utils::readline(fd, rcv_line) == 0)
				{
					std::cerr << "Could not read from Server. Terminating" << std::endl;
					exit(EXIT_FAILURE);
				}
				std::cout << rcv_line;
			}
		}
	}

	Sigfunc* signal(int signo, Sigfunc* handler)
	{
		struct sigaction act, oact;
		act.sa_handler = handler;
		sigemptyset(&act.sa_mask);
		act.sa_flags = 0;

		if (signo == SIGALRM)
		{
	 #ifdef SA_INTERRUPT
			 act.sa_flags |= SA_INTERRUPT; /* SunOS 4.x */
	 #endif
		}
		else
		{
	#ifdef SA_RESTART
			act.sa_flags |= SA_RESTART; /* SVR4, 4.4BSD */
	#endif
		}
		if (sigaction(signo, &act, &oact) < 0)
			return SIG_ERR;
		return oact.sa_handler;
	}

	void sigchld_handler(int signo)
	{
		pid_t pid;
		int stat;
		while((pid = waitpid(-1, &stat, WNOHANG)) > 0)
		{
			std::cout << "Child with pid: " << pid << " terminated." << std::endl;
		}
		return;
	}
}
