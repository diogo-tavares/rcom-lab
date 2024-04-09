/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

volatile int STOP=FALSE;


int main(int argc, char** argv)
{
    int fd,c, res;
    struct termios oldtio,newtio;
    char buf[5];
    int i, sum = 0, speed = 0;

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

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
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
    newtio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */



    /*
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
    leitura do(s) próximo(s) caracter(es)
    */


    tcflush(fd, TCIOFLUSH);


    if (tcsetattr(fd,TCSANOW,&newtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }


    printf("New termios structure set\n");


    //state machinw


    buf[0]=0x5c;
    buf[1]=0x03;
    buf[2]=0x08;
    buf[3]=buf[1]^buf[2];
    buf[4]=0x5c;


    res = write(fd,buf,5);
    printf("%d bytes written\n", res);


    /*
    O ciclo FOR e as instruções seguintes devem ser alterados de modo a respeitar
    o indicado no guião
    */


    const char flag = 0x5c;
	const char sender_address = 0x03;
	const char receiver_address = 0x01;
	const char control_set = 0x08;
	const char control_ua = 0x06;
	
	char recv[32];
	
    while (STOP==FALSE) {       /* loop for input */
        res = read(fd,recv,8);   /* returns after 5 chars have been input */
        char received_byte = *buf;


        //UA state machine
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

   sleep(1);


    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }
        



    close(fd);
    return 0;
}

