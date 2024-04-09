/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define START  0
#define FLAG_RCV 1
#define A_RCV 2
#define C_RCV 3
#define BCC_OK 4
#define STOP 5

#define sender_address  0x03;
#define receiver_address  0x01;
#define control_set  0x08;
#define control_ua  0x06;


volatile int STOP=FALSE;

int main(int argc, char** argv)
{
    int fd,c, res;
    struct termios oldtio,newtio;
    char buf[255];

    if ( (argc < 2) ||
         ((strcmp("/dev/ttyS0", argv[1])!=0) &&
          (strcmp("/dev/ttyS1", argv[1])!=0) )) {
        printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
        exit(1);
    }


    /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
    */


    fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd < 0) { perror(argv[1]); exit(-1); }

    if (tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
        perror("tcgetattr");
        exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 5;   /* blocking read until 5 chars received */

    /*
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
    leitura do(s) próximo(s) caracter(es)
    */


    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd,TCSANOW,&newtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }

    sleep(1);
	
    printf("New termios structure set\n");

	const char flag = 0x5c;
	const char sender_address = 0x03;
	const char receiver_address = 0x01;
	const char control_set = 0x08;
	const char control_ua = 0x06;
	
	char respond_buff[32];
    char state = START;
	
    while (STOP==FALSE) {       /* loop for input */
        res = read(fd,buf,8);   /* returns after 5 chars have been input */
        char received_byte = *buf;

        switch (state){
            case START:
                if (received_byte == flag)
                    state = FLAG_RCV;
                break;

            case FLAG_RCV:
                if (received_byte == sender_address)
                    state = A_RCV;
                else
                    state = START;
                break;

            case A_RCV:
                if (received_byte == sender_address)
                    state = A_RCV;
                break;

            case C_RCV:
                if (received_byte == sender_address)
                    state = A_RCV;
                break;

        }
		
    }



    /*
    O ciclo WHILE deve ser alterado de modo a respeitar o indicado no guião
    */
	sleep(1);

    tcsetattr(fd,TCSANOW,&oldtio); 
    close(fd);
    return 0;
}
