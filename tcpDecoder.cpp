#include <stdio.h>
#include <iostream>

using namespace std;

// An unsigned char can store 1 Bytes (8bits) of data (0-255)
typedef unsigned char BYTE;
typedef unsigned int TWOBYTES;
typedef unsigned long FOURBYTES;
typedef bool FLAG;
const int HIBIT = 1;
const int LOBIT = 0;


unsigned int twobytes(BYTE bytes[]);
unsigned long fourbytes(BYTE bytes[]);
unsigned int fourbits(BYTE byte, bool hiNib);
bool onebit(BYTE byte, int offset);

//TODO define all the contents of a tcpHeader and store it
struct tcpHeader
{
	TWOBYTES srcPort;
	TWOBYTES desPort;
	FOURBYTES seqNum;
	FOURBYTES ackNum;
	unsigned short dataOffset;
	unsigned short reserved;
	FLAG CWR;
	FLAG ECE;
	FLAG URG;
	FLAG ACK;
	FLAG PSH;
	FLAG RST;
	FLAG SYN;
	FLAG FIN;
	TWOBYTES window;
	TWOBYTES checksum;
	TWOBYTES urgentPtr;
} header; //this is an instantiation of tcpHeader type

// Get the size of a file
long getFileSize(FILE *file)
{
    long lCurPos, lEndPos;
    lCurPos = ftell(file);
    fseek(file, 0, 2);
    lEndPos = ftell(file);
    fseek(file, lCurPos, 0);
    return lEndPos;
}

int main()
{
    const char *filePath = "sampleTcpDump.dat";
    BYTE *fileBuf;          // Pointer to our buffered data
    FILE *file = NULL;      // File pointer

    // Open the file in binary mode using the "rb" format string
    // This also checks if the file exists and/or can be opened for reading correctly
    if ((file = fopen(filePath, "rb")) == NULL)
    {
        cout << "Could not open specified file" << endl;
        return -1;
    }
    else
        cout << "File contents:" << endl;

    // Get the size of the file in bytes
    long fileSize = getFileSize(file);

    // Allocate space in the buffer for the whole file
    fileBuf = new BYTE[fileSize];

    // Read the file in to the buffer
    fread(fileBuf, fileSize, 1, file);

    // Now that we have the entire file buffered, we can take a look at some binary infomation
    // Lets take a look in hexadecimal
    for (int i = 0; i < fileSize; i++)
    {
    	if((i+1) % 10 == 0)
    		printf("%02X\n", fileBuf[i]);
    	else
    		printf("%02X ", fileBuf[i]);
    }


    header.srcPort = twobytes(&fileBuf[0]);
    printf("\nSource Port: %d", header.srcPort);

    header.desPort = twobytes(&fileBuf[2]);
    printf("\nDestination Port: %d", header.desPort);

    header.seqNum = fourbytes(&fileBuf[4]);
    printf("\nSequence Number: %lu", header.seqNum);

    header.ackNum = fourbytes(&fileBuf[8]);
    printf("\nAcknowledgement Number: %lu", header.ackNum);

    header.dataOffset = fourbits(fileBuf[12], HIBIT);
    printf("\nData Offset: %d", header.dataOffset);

    header.reserved = fourbits(fileBuf[12], LOBIT);
    printf("\nReserved: %d", header.reserved);

    header.CWR = onebit(fileBuf[13], 7);
	header.ECE = onebit(fileBuf[13], 6);
	header.URG = onebit(fileBuf[13], 5);
	header.ACK = onebit(fileBuf[13], 4);
	header.PSH = onebit(fileBuf[13], 3);
	header.RST = onebit(fileBuf[13], 2);
	header.SYN = onebit(fileBuf[13], 1);
	header.FIN = onebit(fileBuf[13], 0);
    printf("\nFLAGS\n------------\n"
    		"CWR:%d\n"
    		"ECE:%d\n"
    		"ACK:%d\n"
    		"URG:%d\n"
    		"PSH:%d\n"
    		"RST:%d\n"
    		"SYN:%d\n"
    		"FIN:%d\n------------", header.CWR, header.ECE, header.URG, header.ACK, header.PSH, header.RST,
    		header.SYN, header.FIN);

    header.window = twobytes(&fileBuf[14]);
    printf("\nWindow: %d", header.window);
    header.checksum = twobytes(&fileBuf[16]);
    printf("\nCheck Sum: %d", header.checksum);
    header.urgentPtr = twobytes(&fileBuf[18]);
    printf("\nUrgent Pointer: %d", header.urgentPtr);

    delete[]fileBuf;
    fclose(file);
    return 0;
}

unsigned int twobytes(BYTE bytes[])
{
	return((bytes[0] << 8) + bytes[1]);
}

unsigned long fourbytes(BYTE bytes[])
{
	return((bytes[0] << 24) + (bytes[1] << 16) + (bytes[2] << 8) + bytes[3]);
}

unsigned int fourbits(BYTE byte, bool bitSide)
{
	if(bitSide)
	{
		return((byte & 0xf0) >> 4);
	}
	return( byte & 0x0f);
}


bool onebit(BYTE byte, int offset)
{
	return( (1 << offset) & byte );
}
