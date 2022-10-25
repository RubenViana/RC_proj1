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

int receivedFrames;
int sentFrames;
int nTimeouts;
//missing some ...


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

    buffer[bufSize + 4] = bcc_2(buf, bufSize);
    buffer[bufSize + 5] = FLAG_RCV;

    unsigned int bufStuffedSize = byteStuffing (buffer, bufSize);

    if (writeIFrame(buffer ,bufStuffedSize) == 1) return bufStuffedSize;

    return 0;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{   
    int numBytes;
    int readValue;
    int isBufferFull = FALSE;

    unsigned char frame[1000];

    while (!isBufferFull){
        readValue = readIFrame(frame);
        //printf("FRAME RECEIVED I\n");

        numBytes = byteDestuffing(frame, readValue);


        int controlByteRead;
        if (frame[2] == C_RCV_I0)
            controlByteRead = 0;
        else if (frame[2] == C_RCV_I1)
            controlByteRead = 1;

        unsigned char responseByte;
        if (frame[numBytes - 2] == bcc_2(&frame[DATA_START], numBytes - 6))
        { // if bcc2 is correct

        if (controlByteRead != sequenceNumberI)
        { // duplicated trama; discard information

            if (controlByteRead == 0)
            {
            responseByte = C_RCV_RR1;
            sequenceNumberI = 1;
            }
            else
            {
            responseByte = C_RCV_RR0;
            sequenceNumberI = 0;
            }
        }
        else
        { // new frame

            // passes information to the buffer
            for (int i = 0; i < numBytes - 6; i++)
            {
            packet[i] = frame[DATA_START + i];
            }

            isBufferFull = TRUE;

            if (controlByteRead == 0)
            {
            responseByte = C_RCV_RR1;
            sequenceNumberI = 1;
            }
            else
            {
            responseByte = C_RCV_RR0;
            sequenceNumberI = 0;
            }
        }
        }
        else
        { // if bcc2 is not correct
        if (controlByteRead != sequenceNumberI)
        { // duplicated trama

            // ignores frame data

            if (controlByteRead == 0)
            {
            responseByte = C_RCV_RR1;
            sequenceNumberI = 1;
            }
            else
            {
            responseByte = C_RCV_RR0;
            sequenceNumberI = 0;
            }
        }
        else
        { // new trama

            // ignores frame data, because of error

            if (controlByteRead == 0)
            {
            responseByte = C_RCV_REJ0;
            sequenceNumberI = 0;
            }
            else
            {
            responseByte = C_RCV_REJ1;
            sequenceNumberI = 1;
            }
        }
        }

        writeFrame(A_RCV_cmdT_ansR, responseByte);
    }
    return (numBytes - 6);
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

                    //print statistics
                    printf("CONNECTION STATS\n");
                    printf("Received Frames: %d\n", receivedFrames);
                    printf("Sent Frames: %d\n", sentFrames);
                    printf("Number Timeouts: %d\n", nTimeouts);


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

                //print statistics
                printf("CONNECTION STATS\n");
                printf("Received Frames: %d\n", receivedFrames);
                printf("Sent Frames: %d\n", sentFrames);
                printf("Number Timeouts: %d\n", nTimeouts);

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
