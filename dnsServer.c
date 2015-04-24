#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>

#define MAXLINE	256
const int backlog = 4;

void *clientHandler(void *arg);
char *dnsLookup(char dns[]);

int main(int argc, char *argv[])
{
	// listen and connect socket
	int listenfd, connfd;
	pthread_t tid;
	int clilen;
	struct sockaddr_in cliaddr, servaddr;

	//make sure they entered in the port to wait on
	if (argc != 2)
	{
		printf("Usage: dnsServer <port> \n");
		return -1;
	}

	//create the socket
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd == -1)
	{
		fprintf(stderr, "Error unable to create socket, errno = %d (%s) \n",
				errno, strerror(errno));
		return -1;
	}

	bzero(&servaddr, sizeof(servaddr));

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(atoi(argv[1]));

	//bind the socket
	if (bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) == -1)
	{
		fprintf(stderr, "Error binding to socket, errno = %d (%s) \n", errno,
		strerror(errno));

		return -1;
	}

	//listen to the socket
	if (listen(listenfd, backlog) == -1)
	{
		fprintf(stderr,
				"Error listening for connection request, errno = %d (%s) \n",
				errno, strerror(errno));

		return -1;
	}

	//wait for the client to send a request
	while (1)
	{
		clilen = sizeof(cliaddr);

		if ((connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen))
				< 0)
		{

			if (errno== EINTR)
				continue;
			else
			{
				fprintf(stderr,
						"Error connection request refused, errno = %d (%s) \n",
						errno, strerror(errno));
			}
		}

		//create a thread to handle the clients request
		if (pthread_create(&tid, NULL, clientHandler, (void *) &connfd) != 0)
		{
			fprintf(stderr,
					"Error unable to create thread, errno = %d (%s) \n", errno,
					strerror(errno));
		}

	}
}

/********************************************************************************
 * clientHandler
 * ------------------------------------------------------------------------------
 * @param arg		the control of the server to read and write
 * @return
 ********************************************************************************/
void *clientHandler(void *arg)
{
	char dns[MAXLINE];
	int i, n;

	int fd = *(int*) (arg);

	while (1)
	{
		if ((n = read(fd, dns, MAXLINE)) == 0)
		{
			close(fd);
			return;
		}

		char *str = dnsLookup(dns);

		write(fd, str, strlen(str));
	}

}

/********************************************************************************
 * dnsLookup
 * ------------------------------------------------------------------------------
 * @param dns		the string to lookup
 * @return			the formatted string of the lookup
 ********************************************************************************/
char *dnsLookup(char dns[])
{
	//initialize the string to format and return
	char *str = malloc(MAXLINE * sizeof(char));
	int i;
	struct hostent *he;
	struct in_addr **addr_list;

	//lookup the dns and make sure its valid
	if ((he = gethostbyname(dns)) == NULL)
	{
		// get the host info
		return ("ERROR not a valid DNS\n");
	}

	// format the string with information about the dns
	strncpy(str, "Official name is: ", MAXLINE );
	strncat(str, he->h_name, (MAXLINE - sizeof(str)));
	strncat(str, "\nIP addresses: ", (MAXLINE - sizeof(str)));

	addr_list = (struct in_addr **) he->h_addr_list;

	for (i = 0; addr_list[i] != NULL; i++)
	{
		strncat(str, inet_ntoa(*addr_list[i]), (MAXLINE - sizeof(str)));
		strncat(str, " ", (MAXLINE - sizeof(str)));
	}
	strncat(str, "\n", (MAXLINE - sizeof(str)));

	return str;
}
