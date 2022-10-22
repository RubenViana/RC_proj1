// Link layer protocol implementation

#include "link_layer.h"
#include "frame.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>




int fd;
LinkLayer connectionParameters;
struct termios oldtio;
struct termios newtio;

int sequenceNumberI = 0;


////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer conParameters)
{
    connectionParameters = conParameters;

    fd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY);

    if (fd < 0)
    {
        perror(connectionParameters.serialPort);
        exit(-1);
    }

    // Save current port settings
    if (tcgetattr(fd, &oldtio) == -1)
    {
        perror("tcgetattr");
        exit(-1);
    }

    // Clear struct for new port settings
    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag = connectionParameters.baudRate | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    // Set input mode (non-canonical, no echo,...)
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 0; // Inter-character timer unused
    newtio.c_cc[VMIN] = 0;  // Blocking read until 0 chars received

    //   TCIFLUSH - flushes data received but not read.
    tcflush(fd, TCIOFLUSH);

    // Set new port settings
    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    if (connectionParameters.role == LlRx){
        if (readFrame(A_RCV_cmdT_ansR,C_RCV_SET) == 1) return writeFrame(A_RCV_cmdT_ansR, C_RCV_UA);
    }
    else if (connectionParameters.role == LlTx) return writeReadWithRetr(A_RCV_cmdT_ansR, C_RCV_SET, A_RCV_cmdT_ansR, C_RCV_UA);
    
    return -1;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{   
    unsigned char buffer[6 + bufSize];

    //set I fields
    buffer[0] = FLAG_RCV;
    buffer[1] = A_RCV_cmdT_ansR;
    if (sequenceNumberI == 0) buffer[2] = C_RCV_I0;
    else buffer[2] = C_RCV_I1;
    buffer[3] = buffer[1]^buffer[2];

    for (int i = 0; i < bufSize; i++) buffer[i + 4] = buf[i];

    buffer[6 + bufSize - 2] = 0; //bcc_2(buf, bufSize);
    buffer[6 + bufSize - 1] = FLAG_RCV;

    //byte stuffing goes here


    if (writeIFrame(buffer ,sizeof(buffer)) == 1) return bufSize;

    return -1;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{   

    int bytes = readIFrame (packet);

    //byte destuffing goes here

    return bytes;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)
{
    if (connectionParameters.role == LlRx){
        if (readFrame(A_RCV_cmdT_ansR,C_RCV_DISC) == 1) 
            if (writeFrame(A_RCV_cmdT_ansR, C_RCV_DISC) == 1)
                if (readFrame(A_RCV_cmdT_ansR, C_RCV_UA) == 1){
                    // Restore the old port settings
                    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
                    {
                        perror("tcsetattr");
                        exit(-1);
                    }
                    close(fd);
                    return 1;
                }
    }
    else if (connectionParameters.role == LlTx){
        if (writeReadWithRetr(A_RCV_cmdT_ansR,C_RCV_DISC,A_RCV_cmdT_ansR,C_RCV_DISC) == 1) 
            if (writeFrame(A_RCV_cmdT_ansR, C_RCV_UA) == 1){
                // Restore the old port settings
                if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
                {
                    perror("tcsetattr");
                    exit(-1);
                }
                close(fd);
                return 1;
            }
    }
    return -1;
}
