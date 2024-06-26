/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "link_library.h"

int main(int argc, char** argv)
{
    int fd;
    linkLayer connectionParameters;

    if ( (argc < 2)) {
        printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
        exit(1);
    }

    char* port = argv[1];

    printf("Port %s\n", port);
    strcpy(connectionParameters.serialPort,port);
    connectionParameters.role = RECEIVER;
    connectionParameters.baudRate = 38400;


    fd = llopen(connectionParameters);
    printf("Receiver: Port opened\n");

    //FILE *received;
    char buf[1024];
    int bytes;

    //received = fopen("test.txt", "wb");

    llread(fd, buf);

    /*while (( bytes=llread(fd, buf)) > 0)
    {
        fwrite(buf, sizeof(char), bytes, received);
    }*/

    //byte_destuffing();

    //print readed buf

    //llwrite(fd, )


    llclose(fd, connectionParameters, 0);
    printf("Receiver: Closed ports\n");


   sleep(1);



    return 0;
}
