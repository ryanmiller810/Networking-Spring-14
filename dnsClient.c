#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define MAXLINE 256


void dnsClient(FILE *fp, int socket_fd, char *dns);
int  validLoopbackIp(char *ip);

int main(int argc, char *argv[])
{
	int socket_fd;
	struct sockaddr_in servaddr;

	//make sure they compiled with a hostname port and a dns to lookup
	if (argc != 4)
	{
		printf("Usage: dnsClient <address | dns name> <port> <name>\n");
		return -1;
	}

	//create a socket
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_fd < 0)
	{
		fprintf(stderr, "Error creating socket, errno = %d (%s) \n", errno,
		strerror(errno));
		return -1;
	}

	//initialize the struct for the socket
	bzero(&servaddr, sizeof(servaddr));

	servaddr.sin_family = AF_INET;

	/*if they passed a DNS set that as the ip to connect on, if not see
		if you were passed an ip and in either circumstance if true set it
		as the connection ip*/
	if (inet_pton(AF_INET, argv[1], &(servaddr.sin_addr)) == 0 )
	{
		struct hostent *he;
		struct in_addr **addr_list;

		//get the first ip of the DNS (only works for loopback ips)
		if ((he = gethostbyname(argv[1])) != NULL)
		{
			addr_list = (struct in_addr **) he->h_addr_list;
			if(validLoopbackIp(inet_ntoa(*addr_list[0])) ==  0)
			{
				fprintf(stderr, "Error DNS does not represent a loopback ip \n");
				return -1;
			}
			else
			  {
			servaddr.sin_addr = *addr_list[0];
			  }
		}
		else
		{
			fprintf(stderr, "Error DNS for ip selection invalid\n");
			return -1;
		}
	}
	else
	{
		if(validLoopbackIp(argv[1])== 0)
		{
			fprintf(stderr, "Error is not a loopback ip\n");
			return -1;
		}
		else
		  {
		servaddr.sin_addr.s_addr = inet_addr(argv[1]);
		  }
	}

	servaddr.sin_port = htons(atoi(argv[2]));

	//try to connect to the socket
	if (connect(socket_fd, (struct sockaddr *) &servaddr, sizeof(servaddr))
			== -1)
	{
		fprintf(stderr,
				"Error unable to connect to socket, errno = %d (%s) \n", errno,
				strerror(errno));
		return -1;
	}

	//pass the server the DNS to lookup
	dnsClient(stdin, socket_fd, argv[3]);

	return 0;

}

/********************************************************************************
 * dnsClient
 * -----------------------------------------------------------------------------
 * @param fp			filestrem associated with the text received from server
 * @param socket_fd		socket id to write to server
 * @param dns			name the server is going to look up
 * @return
 ********************************************************************************/
void dnsClient(FILE *fp, int socket_fd, char * dns)
{
	//initialize the string recieved from server
	char rcvLine[MAXLINE];
	memset((void *) rcvLine, 0, MAXLINE);

	if (dns != NULL)
	{
		//write to the server
		write(socket_fd, (void *) dns, strlen(dns));

		//try and read the response back
		if (read(socket_fd, rcvLine, MAXLINE) == 0)
		{
			printf("ERROR: server terminated \n");
			return;
		}

		//write the response to stream
		fputs(rcvLine, stdout);
	}
}

int validLoopbackIp(char *ip)
{
	return(ip[0] == '1' && ip[1] == '2' && ip[2] == '7');
}
