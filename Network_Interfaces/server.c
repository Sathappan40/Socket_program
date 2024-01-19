#include <stdio.h>
#include <stdlib.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void print_interface_inf(const struct ifaddrs *ifa)
{
	printf("Interface Name : %s\n", ifa->ifa_name);

	if(ifa->ifa_addr->sa_family == AF_INET)
	{
		struct sockaddr_in *addr = (struct sockaddr_in *)ifa->ifa_addr;
		char ip[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(addr->sin_addr), ip, INET_ADDRSTRLEN);
		printf("IPv4 Address : %s\n", ip);
	

	if(ifa->ifa_netmask != NULL)
	{
		struct sockaddr_in *netmask = (struct sockaddr_in *)ifa->ifa_netmask;
		char nm[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(netmask->sin_addr), nm, INET_ADDRSTRLEN);
		printf("Netmask : %s\n", nm);
	}

	if(ifa->ifa_broadaddr != NULL)
	{
		struct sockaddr_in *broadaddr = (struct sockaddr_in *)ifa->ifa_broadaddr;
		char baddr[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(broadaddr->sin_addr), baddr, INET_ADDRSTRLEN);
		printf("Broadcast Address : %s\n", baddr);
	}
	}

	else if(ifa->ifa_addr->sa_family == AF_INET6)
        {
                struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)ifa->ifa_addr;
                char ip6[INET6_ADDRSTRLEN];
                inet_ntop(AF_INET6, &(addr6->sin6_addr), ip6, INET6_ADDRSTRLEN);
                printf("IPv6 Address : %s\n", ip6);


        if(ifa->ifa_netmask != NULL)
        {
                struct sockaddr_in6 *netmask6 = (struct sockaddr_in6 *)ifa->ifa_netmask;
                char nm6[INET6_ADDRSTRLEN];
                inet_ntop(AF_INET6, &(netmask6->sin6_addr), nm6, INET6_ADDRSTRLEN);
                printf("Netmask : %s\n", nm6);
        }
	
	if(ifa->ifa_broadaddr != NULL)
        {
                struct sockaddr_in6 *broadaddr6 = (struct sockaddr_in6 *)ifa->ifa_broadaddr;
                char baddr6[INET6_ADDRSTRLEN];
                inet_ntop(AF_INET6, &(broadaddr6->sin6_addr), baddr6, INET6_ADDRSTRLEN);
                printf("Broadcast Address : %s\n", baddr6);
        }
	}
	
	printf("\n");
}


int main()
{
	struct ifaddrs *ifaddr, *ifa;

	if(getifaddrs(&ifaddr) == -1)
	{
		perror("getifaddrs");
		exit(1);
	}

	for(ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
	{
		if(ifa->ifa_addr != NULL)
		{
			print_interface_inf(ifa);
		}
	}
	
	freeifaddrs(ifaddr);
	return 0;
}

