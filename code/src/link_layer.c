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

    if (connectionParameters.role == LlRx){
        if (readFrame(A_RCV_cmdT_ansR,C_RCV_SET) == 1) return writeFrame(A_RCV_cmdT_ansR, C_RCV_UA);
    }
    else if (connectionParameters.role == LlTx) return writeReadWithRetr(A_RCV_cmdT_ansR, C_RCV_UA, A_RCV_cmdT_ansR, C_RCV_UA);
    
    return -1;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
    // TODO

    return 0;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    // TODO

    return 0;
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
        if (writeFrame(A_RCV_cmdT_ansR,C_RCV_DISC) == 1) 
            if (readFrame(A_RCV_cmdT_ansR, C_RCV_DISC) == 1)
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
