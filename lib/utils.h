#pragma once
#include <sys/types.h>
#include <string.h>
namespace val::utils
{
	ssize_t readn(int fd, void* buff, size_t nBytes);
	ssize_t writen(int fd, const void* buff, size_t nBytes);
        ssize_t readline(int fd, void* buff);
	void str_echo(int fd);
	void str_cli(int fd);
}
