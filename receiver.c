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
#define START 1
#define FLAG_RCV 2
#define A_RCV 3
#define C_RCV 4
#define BCC_OK 5
#define STOP_STATE 6
 
volatile int STOP=FALSE;
 
 
int main(int argc, char** argv)
{
    int fd,c, res;
    struct termios oldtio,newtio;
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
 
 
 
    /*
    O ciclo FOR e as instruções seguintes devem ser alterados de modo a respeitar
    o indicado no guião
    */
 
    char buf[255];

    const char flag = 0x5c;
	const char transmitter_address = 0x03;
	const char receiver_address = 0x01;
	const char control_set = 0x08;
	const char control_ua = 0x06;
 
	buf[0]=flag;
    buf[1]=transmitter_address;
    buf[2]=control_ua;
    buf[3]=buf[1]^buf[2];
    buf[4]=flag;
 
	char state = START;
    char recv[32];
    int recv_i=0;

    char received_byte;
 
    while (STOP==FALSE) {  
        printf("Reading\n");     /* loop for input */
        res = read(fd,&received_byte,1);   /* returns after 5 chars have been input */
        recv[recv_i] = received_byte;
        recv_i++;

        printf("State: %d, Byte: %x, Res: %d\n, ", state, received_byte, res);
        //UA state machine
        switch (state){
            case START:
                if (received_byte == flag){
                    state = FLAG_RCV;
                    printf("State: %d, Byte: %x\n", state, received_byte);
                }
                    
                break;
 
            case FLAG_RCV:
                if (received_byte == receiver_address)
                    state = A_RCV;
                else{
                    state = START;
                    recv_i = 0;
                }
                break;
 
            case A_RCV:
                if (received_byte == control_set)
                    state = C_RCV;
                else{
                    state = START;
                    recv_i = 0;
                }
                break;
 
            case C_RCV:
                if (received_byte == (recv[1]) ^ recv[2])
                    state = BCC_OK;
                else{
                    state = START;
                    recv_i = 0;
                }
                break;
 
            case BCC_OK:
                if (received_byte == flag){
                    state = STOP;
                    res = write(fd,buf,5);
                    STOP = TRUE;
				}
                break;
 
            case STOP_STATE:
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
 