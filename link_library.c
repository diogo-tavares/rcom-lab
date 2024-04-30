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

struct termios oldtio,newtio;
/**
 * Open Port 'port'
 * and return file descriptor as int
 * if 'transmitter' is 1, we are a transmitter, if 'transmitter' is 0, we are a receiver
*/
int llopen(linkLayer connectionParameters){
   /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
    */
    printf("Opening Port\n");

   char port[50];
   strcpy(port, connectionParameters.serialPort);
   int role = connectionParameters.role;

    int fd, c, res;
    char buf[5];
    int i, sum = 0, speed = 0;

    fd = open(port, O_RDWR | O_NOCTTY );
    if (fd < 0) { perror(port); exit(-1); }

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
        perror("tcgetattr");
        exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = connectionParameters.baudRate | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */

    tcflush(fd, TCIOFLUSH);


    if (tcsetattr(fd,TCSANOW,&newtio) == -1) {
        perror("tcsetattr");
        return -1;
    }

    // TRANSMITTER SENDS SET and waits for UA
    if (role == TRANSMITTER){
        buf[0]=FLAG;
        buf[1]=RECEIVER_ADDRESS;
        buf[2]=CONTROLS_SET;
        buf[3]=buf[1]^buf[2];
        buf[4]=FLAG;
        res = write(fd,buf,5);
    }
	
	char recv[32];
    char received_byte;
    char state;
    int recv_i=0;

    // check if correct bytes arrived
    char expected_address = (role==TRANSMITTER) ? SENDER_ADDRESS : RECEIVER_ADDRESS;
    char expected_control = (role==TRANSMITTER) ? CONTROL_UA : CONTROLS_SET;
	
    int STOP=FALSE;
    while (STOP==FALSE) {       /* loop for input */
        res = read(fd,(&received_byte),1);   /* read byte by byte */
        recv[recv_i] = received_byte;
        recv_i++;
        printf("State: %d\n", state);
        printf("Received: %x\n", received_byte);


            //UA state machine
            switch (state){
                case START:
                    if (received_byte == FLAG)
                        state = FLAG_RCV;
                    break;

                case FLAG_RCV:
                    if (received_byte == expected_address){
                        state = A_RCV;
                    }else{
                        state = START;
                        recv_i = 0;
                    }
                    break;

                case A_RCV:
                    if (received_byte == expected_control)
                        state = C_RCV;
                    else{
                        state = START;
                        recv_i = 0;
                    }
                    break;

                case C_RCV:
                    if (received_byte == (recv[recv_i-2] ^ recv[recv_i-3]))
                        {state = BCC_OK;}
                    else{
                        state = START;
                        recv_i = 0;
                    }
                    break;

                case BCC_OK:
                    if (received_byte == FLAG){
                        STOP = TRUE;
                    }
                    break;

                default:
                    if (received_byte == FLAG){
                        state = FLAG_RCV;
                    }
                    break;
            
        }
    }
    // RECEIVER RESPONDS WITH UA
    if ((role==RECEIVER)){
        buf[0]=FLAG;
        buf[1]=SENDER_ADDRESS;
        buf[2]=CONTROL_UA;
        buf[3]=buf[1]^buf[2];
        buf[4]=FLAG;
        res = write(fd,buf,5);
    }
    
    return fd;
}

int llclose(int fd, linkLayer connectionParameters, int showStatistics){

   char port[50];
   int role = connectionParameters.role;

    // variable initialization
    int res;
    char send_buf[10]; // send buffer
    char recv_buf[10]; // receive buffer
    char received_byte;
    char state;
    int recv_i=0;
    int STOP = FALSE;

    // Transmitter sends DISC and waits for DISC from RECEIVER
    if (role==TRANSMITTER){
        send_buf[0]=FLAG;
        send_buf[1]=RECEIVER_ADDRESS;
        send_buf[2]=CONTROL_DISC;
        send_buf[3]=send_buf[1]^send_buf[2];
        send_buf[4]=FLAG;
        res = write(fd,send_buf,5);

        // check for DISC from RECEIVER
        STOP=FALSE;
        recv_i = 0;
        while (STOP==FALSE) {       /* loop for input */
            res = read(fd,(&received_byte),1);   /* read byte by byte */
            recv_buf[recv_i] = received_byte;
            recv_i++;

            printf("Received: %x\n", received_byte);
            
            //state machine
            switch (state){
                case START:
                    if (received_byte == FLAG)
                        state = FLAG_RCV;
                    break;

                case FLAG_RCV:
                    if (received_byte == SENDER_ADDRESS)
                        state = A_RCV;
                    else{
                        state = START;
                        recv_i = 0;
                    }
                    break;

                case A_RCV:
                    if (received_byte == CONTROL_DISC)
                        state = C_RCV;
                    else{
                        state = START;
                        recv_i = 0;
                    }
                    break;

                case C_RCV:
                    if (received_byte == (recv_buf[recv_i-2] ^ recv_buf[recv_i-3]))
                        state = BCC_OK;
                    else{
                        state = START;
                        recv_i = 0;
                    }
                    break;

                case BCC_OK:
                    if (received_byte == FLAG)
                        state = STOP;
                        STOP = TRUE;
                        break;
        }
        // if everything is ok, send back UA

        send_buf[0]=FLAG;
        send_buf[1]=RECEIVER_ADDRESS;
        send_buf[2]=CONTROL_UA;
        send_buf[3]=send_buf[1]^send_buf[2];
        send_buf[4]=FLAG;
        res = write(fd,send_buf,5);

     }
    }else if (role==RECEIVER){
    // RECEIVER WAITS FOR DISC, then sends back DISC and waits for UA from TRANSMITTER
       
       // 1. step check for DISC from RECEIVEr
        STOP=FALSE;
        recv_i = 0;
        while (STOP==FALSE) {       /* loop for input */
            res = read(fd,(&received_byte),1);   /* read byte by byte */
            recv_buf[recv_i] = received_byte;
            recv_i++;           

            printf("Receiver read byte: %x\n", received_byte);

            //WAIT FOR DISC from TRANSMITTER 
            switch (state){
                case START:
                    if (received_byte == FLAG)
                        state = FLAG_RCV;
                    break;

                case FLAG_RCV:
                    if (received_byte == RECEIVER_ADDRESS)
                        state = A_RCV;
                    else{
                        state = START;
                        recv_i = 0;
                    }
                    break;

                case A_RCV:
                    if (received_byte == CONTROL_DISC)
                        state = C_RCV;
                    else{
                        state = START;
                        recv_i = 0;
                    }
                    break;

                case C_RCV:
                    if (received_byte == (recv_buf[recv_i-2] ^ recv_buf[recv_i-3]))
                        state = BCC_OK;
                    else{
                        state = START;
                        recv_i = 0;
                    }
                    break;

                case BCC_OK:
                    if (received_byte == FLAG)
                        state = STOP;
                        STOP = TRUE;
                        break;
            }
        }

        // DISC received, send back DISC
        send_buf[0]=FLAG;
        send_buf[1]=SENDER_ADDRESS;
        send_buf[2]=CONTROL_DISC;
        send_buf[3]=send_buf[1]^send_buf[2];
        send_buf[4]=FLAG;
        res = write(fd,send_buf,5);

        // WAIT FOR UA from TRANSMITTER
        // 1. step check for DISC from RECEIVEr
        int STOP=FALSE;
        recv_i = 0;
        while (STOP==FALSE) {       /* loop for input */
            res = read(fd,(&received_byte),1);   /* read byte by byte */
            recv_buf[recv_i] = received_byte;
            recv_i++;

            //WAIT FOR DISC from TRANSMITTER 
            switch (state){
                case START:
                    if (received_byte == FLAG)
                        state = FLAG_RCV;
                    break;

                case FLAG_RCV:
                    if (received_byte == RECEIVER_ADDRESS)
                        state = A_RCV;
                    else{
                        state = START;
                        recv_i = 0;
                    }
                    break;

                case A_RCV:
                    if (received_byte == CONTROL_UA)
                        state = C_RCV;
                    else{
                        state = START;
                        recv_i = 0;
                    }
                    break;

                case C_RCV:
                    if (received_byte == (recv_buf[recv_i-2] ^ recv_buf[recv_i-3]))
                        state = BCC_OK;
                    else{
                        state = START;
                        recv_i = 0;
                    }
                    break;

                case BCC_OK:
                    if (received_byte == FLAG){
                        state = STOP;
                        STOP = TRUE;
                        break;
                    }
            }
        }
    }

    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }
    close(fd);
}

char Ns_send = 0;

int llwrite(int fd, char * buffer, int length){
    
    if (length>MAX_PAYLOAD_SIZE){
        return -1;
    }

    // 6 control bytes: flag, address, control, bcc, ..... , bcc, flag
    int send_buffer_len = length+6;
    char buf[send_buffer_len];
    buf[0] = FLAG;
    buf[1] = RECEIVER_ADDRESS; // we assume that only transmitter uses llwrite, but otherwise its strange
   
    char Ns = Ns_send;
    char control = 0b1000000 | (Ns<<7);
    buf[2] = control;

    buf[3] = buf[1]^buf[2];

    // copy the data into sending buffer
    memcpy((buf+4), buffer, length);

    buf[send_buffer_len-2] = 0; // TODO BCC VALUE
    buf[send_buffer_len-1] = FLAG;

    for (int i=0; i <send_buffer_len; i++)
    {
        printf("i: %d contains %x \n", i, buf[i]);
    }

    int res = write(fd,buf,send_buffer_len);
    printf("WRITING, %d bytes written\n", res);

    printf("Waiting for response\n");
    res = read(fd,(&received_byte),5); 
}



int read_control_packet(contrl_packet* packet){
    char recv[5];
    while (STOP==FALSE) {       /* loop for input */
        res = read(fd,(&received_byte),1);   /* read byte by byte */
        recv[recv_i] = received_byte;
        recv_i++;
        printf("State: %d\n", state);
        printf("Received: %x\n", received_byte);

        //UA state machine
        switch (state){
                case START:
                    if (received_byte == FLAG)
                        state = FLAG_RCV;
                    break;

                case FLAG_RCV:
                    if (received_byte == expected_address){
                        state = A_RCV;
                    }else{
                        state = START;
                        recv_i = 0;
                    }
                    break;

                case A_RCV:
                    if (received_byte == expected_control)
                        state = C_RCV;
                    else{
                        state = START;
                        recv_i = 0;
                    }
                    break;

                case C_RCV:
                    if (received_byte == (recv[recv_i-2] ^ recv[recv_i-3]))
                        {state = BCC_OK;}
                    else{
                        state = START;
                        recv_i = 0;
                    }
                    break;

                case BCC_OK:
                    if (received_byte == FLAG){
                        STOP = TRUE;
                    }
                    break;

                default:
                    if (received_byte == FLAG){
                        state = FLAG_RCV;
                    }
                    break;
            
        }
    }
    // RECEIVER RESPONDS WITH UA
    if ((role==RECEIVER)){
        buf[0]=FLAG;
        buf[1]=SENDER_ADDRESS;
        buf[2]=CONTROL_UA;
        buf[3]=buf[1]^buf[2];
        buf[4]=FLAG;
        res = write(fd,buf,5);
    }
    
    return fd;
}

int llread(int fd, char * buffer)
{
    // read character by character until flag is encountered
    int STOP=FALSE;
    int res;
    char state=START;

    char recv[32];
    int recv_i=0;
    char received_byte;
 

    while (STOP==FALSE) {  
        printf("Reading\n");     /* loop for input */
        res = read(fd,&received_byte,1); 
        recv[recv_i] = received_byte;
        recv_i++;

        printf("LLREAD -> State: %d, Byte: %x, Res: %d\n, ", state, received_byte, res);
        //UA state machine
        switch (state){
            case START:
                if (received_byte == FLAG){
                    state = FLAG_RCV;
                    printf("State: %d, Byte: %x\n", state, received_byte);
                }
                    
                break;
 
            case FLAG_RCV:
                if (received_byte == RECEIVER_ADDRESS)
                    state = A_RCV;
                else{
                    state = START;
                    recv_i = 0;
                }
                break;
 
            case A_RCV:
                if (received_byte == CONTROLS_SET)
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
                if (received_byte == FLAG){
                    state = NULL;
                    STOP = TRUE;
				}
                break;
        
            default:
            break;
        }
    }
}

int byte_stuffing(char *buf, char *newbuff)
{
    int i=0;
    int size = sizeof(buf);
    if (size==0)
    {
        return -1;
    }
    while (i<size)
    {
        if (buf[i]=='5c')
        {
            newbuff[i]='5d 7c';
        }
        else if(buf[i]=='5d')
        {
            newbuff[i]='5d 7d';
        }
        else
        {
            newbuff[i]=buf[i];
        }
    }
}

int byte_destuffing(char *buf, char *newbuff)
{
    int i=0;
    int size = sizeof(buf);
    if (size==0)
    {
        return -1;
    }
    
    while (i<size)
    {
        if (buf[i]=='5d 7c')
        {
            newbuff[i]='5c';
            i++;
        }
        else if(buf[i]=='5d 7d')
        {
            newbuff[i]='5d';
            i++;
        }
        else
        {
            newbuff[i]=buf[i];
            i++;
        }
    }
}