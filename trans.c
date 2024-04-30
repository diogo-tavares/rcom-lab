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
    connectionParameters.role = TRANSMITTER;
    connectionParameters.baudRate = 38400;


    fd = llopen(connectionParameters);
    printf("Transmitter: Port opened\n");

    char data[10] = {1,2,3,4,5,6,7,8,9,10};
    //byte_stuffing(data, new);
    llwrite(fd, data, 10);

    llclose(fd, connectionParameters, 0);
    printf("Transmitter: Port Closed\n");

   sleep(1);



    return 0;
}
