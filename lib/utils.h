#pragma once
#include <sys/types.h>
#include <string.h>
#include <signal.h>

namespace val::utils
{
	using Sigfunc = void(int);

	ssize_t readn(int fd, void* buff, size_t nBytes);
	ssize_t writen(int fd, const void* buff, size_t nBytes);
        ssize_t readline(int fd, void* buff);

	void str_echo(int fd);
	void str_cli(int fd);
	void str_cli_binary(int fd);
	void str_cli_select(int fd);
	void str_echo_binary(int fd);

	void sigchld_handler(int signo);
	Sigfunc* signal(int signo, Sigfunc* handler);
}
