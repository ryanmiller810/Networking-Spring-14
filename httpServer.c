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

#define MAXLINE 256
#define PORT_NUMBER 9001
const int backlog = 4;

char *requestHandler(char request[]);
void *clientHandler(void *arg);
void Get(char reponse[], char request[]);
char* Head(char response[]);
void Delete(char request[], char response[]);
char *Put(char request[]);
char* substring(char *string, int position, int length);
int contains(char str1[], char str2[]);

int main(int argc, char *argv[])
{
	// listen and connect socket
	int listenfd, connfd;
	pthread_t tid;
	int clilen;
	struct sockaddr_in cliaddr, servaddr;
	unsigned short portNumber;

	//get the port to wait on
	if (argc == 1)
	{
		portNumber = PORT_NUMBER;
	}
	else if (argc == 2)
	{
		portNumber = (unsigned short) atoi(argv[1]);
	}
	else if (argc > 2)
	{
		printf("Usage: htmlServer <port> \n");
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
	servaddr.sin_port = htons(portNumber);

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
		"Error listening for connection request, errno = %d (%s) \n", errno,
		strerror(errno));

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
				"Error connection request refused, errno = %d (%s) \n", errno,
				strerror(errno));
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
 * @param arg the control of the server to read and write
 * @return
 ********************************************************************************/
void *clientHandler(void *arg)
{

	int i, n;
	int count = 0;

	int fd = *(int*) (arg);

		char request[MAXLINE];
		if ((n = read(fd, request, MAXLINE)) == 0)
		{
			close(fd);
			return;
		}

		// Process the HTTP Request the user sent to the server.
		if (contains(request, "HTTP/1.1") == 0 && contains(request, "HTTP/1.0")
				== 0)
		{
			printf("The message received was not an HTTP message %s", " ");
			close(fd);
			return;
		}

		char *str = requestHandler(request);

		write(fd, str, strlen(str));

		close(fd);
}

/********************************************************************************
 * requestHandler
 * ------------------------------------------------------------------------------
 * @param request the command issued from the client
 * @return response string to hold the contents of the command issued
 ********************************************************************************/
char *requestHandler(char request[])
{
	char* response = malloc(sizeof(char) * 9999);

	if (contains(request, "GET") == 1)
	{
		Get(response, request);
		if (strlen(response) > 0)
		{
			response = Head(response);
		}
	}
	else if (contains(request, "PUT") == 1)
	{
		response = Put(request);
	}
	else if (contains(request, "DELETE") == 1)
	{
		Delete(response, request);
	}
	else if (contains(request, "HEAD") == 1)
	{
		response = Head(response);
	}
	else
	{
		strcpy(response, "Invalid Request\n");
	}

	return response;
}

void Get(char response[], char request[])
{
	char script[999];
	char directory[MAXLINE];

	// Gets the filename from the sent request.
	char *fileName = (char*) malloc(sizeof(request) - 13);
	fileName = substring(request, 14, strlen(request) - 15);

	FILE * fp;

	// Process a script to execute on the shell
	getcwd(directory, sizeof(directory));
	strcat(script, "cat ");
	strcat(script, directory);
	strcat(script, "/");
	strcat(script, fileName);
	//printf("The script will run: %s\n", script);

	// The script is now generated, just run it.
	fp = popen(script, "r");
	if (fp == NULL)
	{
		strcat(response, "404 error: File not found");
		return;
	}

	char line[999];
	while (fgets(line, 999, fp) != NULL)
	{
		strcat(response, line);
	}

	pclose(fp);
}

char* Head(char response[])
{
	char *headerResponse = malloc(sizeof(char) * 9999);
	char date[256];

	char *ctime();
	time_t now;

	(void) time(&now);
	sprintf(date, "%s", ctime(&now));

	strcat(headerResponse, "Content-Type: text/html; ");
	strcat(headerResponse, "charset=UTF-8 \n");
	strcat(headerResponse, "Server:CS436 Project \n");
	strcat(headerResponse, "Date: ");
	strcat(headerResponse, date);
	strcat(headerResponse, "Content-Length: ");
	char length[20];
	sprintf(length, "%d", strlen(response));
	strcat(headerResponse, length);
	strcat(headerResponse, "\n");
	strcat(headerResponse, response);
	strcat(headerResponse, "\n");

	return (headerResponse);
}

char* Put(char request[])
{
	// Gets the filename from the sent request.
	char *temp = strstr(request, "<filedata>");
	int filenameOffset = temp - request;

	//File name is set
	char *fileName = (char*) malloc(filenameOffset - 1);
	fileName = substring(request, 14, filenameOffset - 14);

	//Get the file Data
	char *fileData = (char*) malloc(sizeof(request) - filenameOffset);

	fileData = substring(request, strlen(fileName) + filenameOffset + 1,
			strlen(request) - (strlen(fileName) + filenameOffset + 13));
	printf("The file data is: %s\n", fileData);

	FILE * fileOutput;
	fileOutput = fopen(fileName, "w");
	if (fileOutput != NULL)
	{
		fputs(fileData, fileOutput);
		fclose(fileOutput);
	}
	else
	{
		return ("Error Writing file\n");
	}
	return ("PUT request processed \n");
}

void Delete(char response[], char request[])
{
	char script[999];
	char directory[MAXLINE];
	// Gets the filename from the sent request.
	char *fileName = (char*) malloc(sizeof(request) - 16);
	fileName = substring(request, 17, strlen(request) - 18);

	FILE * fp;
	getcwd(directory, sizeof(directory));
	strcat(script, "rm ");
	strcat(script, directory);
	strcat(script, "/");
	strcat(script, fileName);
	//printf("The script will run: %s\n", script);
	// The script is now generated, just run it.

	fp = popen(script, "r");
	if (fp == NULL)
	{
		strcat(response, "404 error: File not found");
		return;
	}

	char line[999];
	while (fgets(line, 999, fp) != NULL)
	{
		strcat(response, line);
	}

	pclose(fp);
}

char *substring(char *string, int position, int length)
{
	char *pointer;
	int index;
	pointer = malloc(length + 1);
	for (index = 0; index < (position - 1); index++)
	{
		string++;
	}

	for (index = 0; index < length; index++)
	{
		*(pointer + index) = *string;
		string++;
	}

	//null terminate the string
	*(pointer + index) = '\0';

	return pointer;
}

int contains(char str1[], char str2[])
{
	char* ptr;
	ptr = strstr(str1, str2);
	if (ptr == NULL)
	{
		return 0;
	}
	return 1;
}
