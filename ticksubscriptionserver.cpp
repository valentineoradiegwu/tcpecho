#include <iostream>
#include <string>
#include <cstring>
#include <array>
#include <algorithm>
#include <vector>
#include <cassert>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "lib/utils.h"
#include "lib/crc32.h"

static constexpr uint8_t BBG_LEN   = 9;
static constexpr uint8_t CUSIP_LEN = 12;
static constexpr uint8_t CRC_SIZE  = 4;

static constexpr std::array<uint8_t, 2> FIELD_SIZES = {BBG_LEN, CUSIP_LEN};
static_assert(FIELD_SIZES.size() <= 256);
static std::byte WI_SUBSCRIPTION[256];

int encode_subscriptions(const std::vector<std::string>& wis)
{
	std::byte* start = WI_SUBSCRIPTION + 1;
	constexpr std::byte* end = WI_SUBSCRIPTION + sizeof(WI_SUBSCRIPTION);
        for (const auto& wi: wis)
	{
		if (start + wi.size() + CRC_SIZE >= end)
			throw "Buffer Overflow";
		const auto iter = std::find(FIELD_SIZES.cbegin(), FIELD_SIZES.cend(), wi.size());
		if (iter == FIELD_SIZES.end())
			throw "Invalid instrument length";
		const std::byte id = static_cast<std::byte>(iter - FIELD_SIZES.cbegin());
		*start++ = id;
		memcpy(start, wi.data(), wi.size());
		start += wi.size();
	}
	const auto payload_size = start - WI_SUBSCRIPTION - 1;
	assert(payload_size < 252);
	WI_SUBSCRIPTION[0] = static_cast<std::byte>(payload_size);

	const uint32_t crc32 = htonl(val::utils::crc32f((const char*)WI_SUBSCRIPTION, start - WI_SUBSCRIPTION));
	memcpy(start, &crc32, CRC_SIZE);
	start += CRC_SIZE;
	return start - WI_SUBSCRIPTION;
}

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
	
	clientaddrlen = sizeof(clientaddr);

	while (true)
	{
		if ( (connfd = accept(listenfd, (sockaddr*)&clientaddr, &clientaddrlen)) < 0)
		{
			if (errno == EINTR) continue;
			std::cerr << "Socket accept failed with error: " << strerror(errno) << std::endl;
			exit(EXIT_FAILURE);
		}
                const auto len = encode_subscriptions({"9999T30Y3", "US91282CHK09"});
		if (len <= 0)
			exit(EXIT_FAILURE);
		std::cout << "Encoded subs" << std::endl;
		for ( int i = 0; i < len; ++i)
		{
			std::cout << std::hex << (int)WI_SUBSCRIPTION[i] << " ";
		}
		if (val::utils::writen(connfd, WI_SUBSCRIPTION, len) <= 0)
			exit(EXIT_FAILURE);
	}
	close(connfd);
	close(listenfd);
}
