// Write to serial port in non-canonical mode
//
// Modified by: Eduardo Nuno Almeida [enalmeida@fe.up.pt]

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>

// Baudrate settings are defined in <asm/termbits.h>, which is
// included by <termios.h>
#define BAUDRATE B38400
#define _POSIX_SOURCE 1 // POSIX compliant source

#define FALSE 0
#define TRUE 1

#define BUF_SIZE 1
#define CMD_FRAME_SIZE 5

#define SET_FRAME 0x7e0303007e

volatile int STOP = FALSE;

#define FLAG_RCV 0x7e
#define A_RCV 0x03
#define C_RCV_UA 0x07
#define BCC_RCV A_RCV ^ C_RCV_UA 

int alarmEnabled = FALSE;
int alarmCount = 0;

// Alarm function handler
void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;

    printf("Alarm #%d\n", alarmCount);
}



enum RCV_STATE {START_RCV_ST, FLAG_RCV_ST, A_RCV_ST, C_RCV_ST, BCC_RCV_ST, STOP_RCV_ST} rcv_st;

int main(int argc, char *argv[])
{
    // Set alarm function handler
    (void)signal(SIGALRM, alarmHandler);

    // Program usage: Uses either COM1 or COM2
    const char *serialPortName = argv[1];

    if (argc < 2)
    {
        printf("Incorrect program usage\n"
               "Usage: %s <SerialPort>\n"
               "Example: %s /dev/ttyS1\n",
               argv[0],
               argv[0]);
        exit(1);
    }

    // Open serial port device for reading and writing, and not as controlling tty
    // because we don't want to get killed if linenoise sends CTRL-C.
    int fd = open(serialPortName, O_RDWR | O_NOCTTY);

    if (fd < 0)
    {
        perror(serialPortName);
        exit(-1);
    }

    struct termios oldtio;
    struct termios newtio;

    // Save current port settings
    if (tcgetattr(fd, &oldtio) == -1)
    {
        perror("tcgetattr");
        exit(-1);
    }

    // Clear struct for new port settings
    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    // Set input mode (non-canonical, no echo,...)
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 0; // Inter-character timer unused
    newtio.c_cc[VMIN] = 0;  // Blocking read until 0 chars received

    // VTIME e VMIN should be changed in order to protect with a
    // timeout the reception of the following character(s)

    // Now clean the line and activate the settings for the port
    // tcflush() discards data written to the object referred to
    // by fd but not transmitted, or data received but not read,
    // depending on the value of queue_selector:
    //   TCIFLUSH - flushes data received but not read.
    tcflush(fd, TCIOFLUSH);

    // Set new port settings
    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    printf("New termios structure set\n");

    //gets(buf);
    

    // In non-canonical mode, '\n' does not end the writing.
    // Test this condition by placing a '\n' in the middle of the buffer.
    // The whole buffer must be sent even with the '\n'.
    //buf[5] = '\n';

    /*
    buf[0] = 0x7e;
    buf[1] = 0x03;
    buf[2] = 0x03;
    buf[3] = 0x03 ^ 0x03;
    buf[4] = 0x7e;
    */

    

    //write set command
    unsigned char set_cmd[CMD_FRAME_SIZE];
    set_cmd[0] = 0x7e;
    set_cmd[1] = 0x03;
    set_cmd[2] = 0x03;
    set_cmd[3] = set_cmd[1] ^ set_cmd[2];
    set_cmd[4] = 0x7e;


    
    //printf("%d bytes written\n", bytes);



    // Wait until all bytes have been written to the serial port
    //sleep(1);


    unsigned char buf[BUF_SIZE];
    int a_byte;
    int c_byte;

    while ((rcv_st != STOP_RCV_ST) && (alarmCount < 4))
    {
        // Returns after 1 chars have been input
        

        if (alarmEnabled == FALSE)
        {
            int wbytes = write(fd, set_cmd, CMD_FRAME_SIZE);
            printf("SET_CMD SENT\n");
            alarm(3); // Set alarm to be triggered in 3s
            alarmEnabled = TRUE;
        }

        int bytes = read(fd, buf, BUF_SIZE);

        if (bytes != 0){
            int byte = buf[0];

            switch (rcv_st)
            {
            case START_RCV_ST:
                if (byte == FLAG_RCV) rcv_st = FLAG_RCV_ST;
                break;

            case FLAG_RCV_ST:
                if (byte == FLAG_RCV) rcv_st = FLAG_RCV_ST;
                else if (byte == A_RCV) {rcv_st = A_RCV_ST; a_byte = byte;}
                else rcv_st = START_RCV_ST;
                break;

            case A_RCV_ST:
                if (byte == FLAG_RCV) rcv_st = FLAG_RCV_ST;
                else if (byte == C_RCV_UA) {rcv_st = C_RCV_ST; c_byte = byte;}
                else rcv_st = START_RCV_ST;
                break;

            case C_RCV_ST:
                if (byte == FLAG_RCV) rcv_st = FLAG_RCV_ST;
                else if (byte == a_byte^c_byte) rcv_st = BCC_RCV_ST;
                else rcv_st = START_RCV_ST;
                break;
            
            case BCC_RCV_ST:
                if (byte == FLAG_RCV) {rcv_st = STOP_RCV_ST; alarm(0); printf("UA_ANS RECEIVED\n");}
                else rcv_st = START_RCV_ST;
                break;

            default:
                break;
            }
        }
            
    }
    //printf("RECEIVED UA MESSAGE");


    // Restore the old port settings
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    close(fd);

    return 0;
}
