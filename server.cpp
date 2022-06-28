#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include "server.hpp"

// POLLFD
struct pollfd	Server::new_pfd(int fd, short events, short revents)
{
	struct pollfd	ret;

	ret.fd = fd;
	ret.events = events;
	ret.revents = revents;
	return (ret);
}

// CONSTRUCTOR
Server::Server()
{
	struct addrinfo	hints;
	struct addrinfo	*res;
	struct addrinfo	*i;
	int				status;
	int				yes;

	yes = 1;
	// hints
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	// addinfo
	status = getaddrinfo(NULL, "6667", &hints, &res);
	if (status)
	{
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
		exit(1);
	}

	// bind
	for (i = res; i != NULL; i = i->ai_next)
	{
		// socket
		listener_fd = socket(i->ai_family, i->ai_socktype, i->ai_protocol);
		if (listener_fd == -1)
		{
			perror("server: socket");
			continue;
		}
		if (setsockopt(listener_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
		{
			perror("setsockopt");
			exit(1);
		}
		if (bind(listener_fd, i->ai_addr, i->ai_addrlen) == -1)
		{
			close(listener_fd);
			perror("server bind");
			continue;
		}
		break;
	}
	// free memory
	freeaddrinfo(res);

	// if couldn't bind to any address
	if (i == NULL || listen(listener_fd, 20) == -1)
	{
		perror("listener");
		exit(1);
	}

	// add listener fd in pfds
	pfds.push_back(new_pfd(listener_fd, POLLIN, 0));
	fcntl(listener_fd, F_SETFL, O_NONBLOCK);
}

// DESTRUCTOR
Server::~Server()
{

}

// RUN
void	Server::run()
{
	int						nfds;
	int						new_fd;
	struct sockaddr_in		remote_addr;
	socklen_t				addrlen;
	char					buff[256];

	while (1)
	{
		nfds = pfds.size();
		if (poll(&pfds[0], nfds, -1) == -1)
		{
			perror("poll");
			exit(1);
		}
		for (int i = 0; i < nfds; i++)
		{
			if (pfds[i].revents & POLLIN)
			{
				if (pfds[i].fd == listener_fd)
				{
					addrlen = sizeof(remote_addr);
					new_fd = accept(listener_fd, (sockaddr *)&remote_addr, &addrlen);
					if (new_fd == -1)
						perror("accept");
					else
					{
						pfds.push_back(new_pfd(new_fd, POLLIN, 0));
						fcntl(new_fd, F_SETFL, O_NONBLOCK);
						printf("pollserver: new connection from socket %s\n",
								inet_ntoa(remote_addr.sin_addr));
					}
				}
				else
				{
					int	nbytes = recv(pfds[i].fd, buff, sizeof(buff), 0);

					if (nbytes <= 0)
					{
						if (!nbytes)
							printf("pollserver: socket %d hung up\n", pfds[i].fd);
						else
							perror("recv");
						close(pfds[i].fd);
						pfds.erase(pfds.begin() + i);
					}
					else
					{
						buff[nbytes] = 0;
						printf("reveived = %s\n", buff);
					}
				}
			}
		}
		send(new_fd, ":denden 251 denden :yo", 256, 0);
	}
}
