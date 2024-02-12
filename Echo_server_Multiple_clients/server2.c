#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>

#define PORT "8888"

void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) 
	{
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void)
{
	fd_set master;
	fd_set read_fds;
	int fdmax;
	int listener;
	int newfd;
	struct sockaddr_storage remoteaddr; // client address
	socklen_t addrlen;
	
	char buf[256];
	int nbytes;
	char remoteIP[INET6_ADDRSTRLEN];
	int yes=1;
	int i, j, rv;
	
	struct addrinfo hints, *ai, *p;

	FD_ZERO(&master);
	FD_ZERO(&read_fds);
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) 
	{
		fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
		exit(1);
	}
	
	for(p = ai; p != NULL; p = p->ai_next) 
	{
		listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (listener < 0) 
		{
			continue;
		}
		// lose the pesky "address already in use" error message
		setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
		if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) 
		{
			close(listener);
			continue;
		}
		break;
	}
	
	if (p == NULL) 
	{
		fprintf(stderr, "selectserver: failed to bind\n");
		exit(2);
	}
	
	freeaddrinfo(ai);
	
	if (listen(listener, 10) == -1) 
	{
		perror("listen");
		exit(3);
	}
	
	FD_SET(listener, &master);
	fdmax = listener;
	
	for(;;) 
	{
		read_fds = master; // copy it
		if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) 
		{
			perror("select");
			exit(4);
		}
		
		for(i = 0; i <= fdmax; i++) 
		{
			if (FD_ISSET(i, &read_fds)) 
			{ 
				if (i == listener) 
				{
					// handle new connections
					addrlen = sizeof remoteaddr;
					newfd = accept(listener,(struct sockaddr*)&remoteaddr,&addrlen);
					if (newfd == -1) 
					{
						perror("accept");
					}
					else 
					{
						FD_SET(newfd, &master); // add to master set
						if (newfd > fdmax) 
						{
						// keep track of the max
						fdmax = newfd;
						}
						printf("selectserver: new connection from %s on socket %d\n",inet_ntop(remoteaddr.ss_family,get_in_addr((struct sockaddr*)&remoteaddr),remoteIP, INET6_ADDRSTRLEN),newfd);
					}
				}
				else 
				{
					// handle data from a client
					if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0) 
					{
					
						perror("recv");
						close(i); // bye!
						FD_CLR(i, &master); // remove from master set
					}
					else 
					{
                                       	for(j = 0; j <= fdmax; j++) 
                                       	{
							// send to everyone!
							if (FD_ISSET(j, &master)) 
							{
								// except the listener and ourselves
								if (j != listener && j == i) 
								{
									if (send(j, buf, nbytes, 0) == -1) 
									{
										perror("send");
									}
								}
							}
						}
                                      }
                               }
                        }
                }
        }
        return 0;
}
		
