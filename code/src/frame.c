#include "frame.h"
#include "link_layer.h"
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

extern int fd;
extern LinkLayer connectionParameters;

extern int alarmEnabled;
extern int alarmCount;


int writeFrame (unsigned int A_RCV, unsigned int C_RCV){

    unsigned char frame[CMD_ANS_FRAME_SIZE];

    frame[0] = 0x7e;
    frame[1] = A_RCV;
    frame[2] = C_RCV;
    frame[3] = frame[1] ^ frame[2];
    frame[4] = 0x7e;

    int bytes = write(fd, frame, CMD_ANS_FRAME_SIZE);

    printf("FRAME SENT  C->%x\n",C_RCV);
    if (bytes > 0) return 1;
    return -1;
}

int readFrame (unsigned int A_RCV, unsigned int C_RCV){

    unsigned char buf[BUF_BYTE_SIZE]; 

    RCV_STATE rcv_st = START_RCV_ST;

    while (rcv_st != STOP_RCV_ST)
    {
        int bytes = read(fd, buf, BUF_BYTE_SIZE);

        if (bytes > 0){
            int byte = buf[0];

            switch (rcv_st)
            {
            case START_RCV_ST:
                if (byte == FLAG_RCV) rcv_st = FLAG_RCV_ST;
                break;

            case FLAG_RCV_ST:
                if (byte == FLAG_RCV) rcv_st = FLAG_RCV_ST;
                else if (byte == A_RCV) rcv_st = A_RCV_ST;
                else rcv_st = START_RCV_ST;
                break;

            case A_RCV_ST:
                if (byte == FLAG_RCV) rcv_st = FLAG_RCV_ST;
                else if (byte == C_RCV) rcv_st = C_RCV_ST;
                else rcv_st = START_RCV_ST;
                break;

            case C_RCV_ST:
                if (byte == FLAG_RCV) rcv_st = FLAG_RCV_ST;
                else if (byte == (A_RCV^C_RCV)) rcv_st = BCC_RCV_ST;
                else rcv_st = START_RCV_ST;
                break;
            
            case BCC_RCV_ST:
                if (byte == FLAG_RCV) rcv_st = STOP_RCV_ST;
                else rcv_st = START_RCV_ST;
                break;

            default:
                break;
            }
        }
    }
    printf("FRAME RECEIVED  C->%x\n",C_RCV);
    return 1;
}



int writeReadWithRetr(unsigned int A_RCV_w, unsigned int C_RCV_w, unsigned int A_RCV_r, unsigned int C_RCV_r){

    alarmCount = 0;
    alarmEnabled = FALSE;

    unsigned char buf[256]; 

    RCV_STATE rcv_st = START_RCV_ST;

    while (alarmCount < connectionParameters.nRetransmissions && rcv_st != STOP_RCV_ST)
    {
        if (alarmEnabled == FALSE)
        {
            writeFrame(A_RCV_w,C_RCV_w);
            alarm(connectionParameters.timeout);
            alarmEnabled = TRUE;
            rcv_st = START_RCV_ST;
        }

        int bytes = read(fd, buf, BUF_BYTE_SIZE);

        if (bytes > 0){
            int byte = buf[0];

            switch (rcv_st)
            {
            case START_RCV_ST:
                if (byte == FLAG_RCV) rcv_st = FLAG_RCV_ST;
                break;

            case FLAG_RCV_ST:
                if (byte == FLAG_RCV) rcv_st = FLAG_RCV_ST;
                else if (byte == A_RCV_r) rcv_st = A_RCV_ST;
                else rcv_st = START_RCV_ST;
                break;

            case A_RCV_ST:
                if (byte == FLAG_RCV) rcv_st = FLAG_RCV_ST;
                else if (byte == C_RCV_r) rcv_st = C_RCV_ST;
                else rcv_st = START_RCV_ST;
                break;

            case C_RCV_ST:
                if (byte == FLAG_RCV) rcv_st = FLAG_RCV_ST;
                else if (byte == (A_RCV_r^C_RCV_r)) rcv_st = BCC_RCV_ST;
                else rcv_st = START_RCV_ST;
                break;
            
            case BCC_RCV_ST:
                if (byte == FLAG_RCV) {rcv_st = STOP_RCV_ST; printf("FRAME RECEIVED  C->%x\n",C_RCV_r); return 1;}
                else rcv_st = START_RCV_ST;
                break;

            default:
                break;
            }
        }
    }
    printf("NUMBER OF RETRANSMISSIONS EXCEEDED\n");
    return -1;
}

unsigned char bcc_2(unsigned char* frame, int length) {

  unsigned char bcc2 = frame[0];

  for(int i = 1; i < length; i++){
    bcc2 = bcc2 ^ frame[i];
  }

  return bcc2;
}

int writeIFrame (unsigned char* frame, int frameSize) {
    unsigned char buf[256]; 

    RCV_STATE rcv_st = START_RCV_ST;

    alarmCount = 0;
    alarmEnabled = FALSE;

    while (alarmCount < connectionParameters.nRetransmissions && rcv_st != STOP_RCV_ST)
    {
        if (alarmEnabled == FALSE)
        {
            write(fd, frame, frameSize);
            printf("FRAME SENT I\n");
            alarm(connectionParameters.timeout);
            alarmEnabled = TRUE;
            rcv_st = START_RCV_ST;
        }

        int bytes = read(fd, buf, BUF_BYTE_SIZE);

        if (bytes > 0){
            int byte = buf[0];

            switch (rcv_st)
            {
            case START_RCV_ST:
                if (byte == FLAG_RCV) rcv_st = FLAG_RCV_ST;
                break;

            case FLAG_RCV_ST:
                if (byte == FLAG_RCV) rcv_st = FLAG_RCV_ST;
                else if (byte == A_RCV_cmdT_ansR) rcv_st = A_RCV_ST;
                else rcv_st = START_RCV_ST;
                break;

            case A_RCV_ST:
                if (byte == FLAG_RCV) rcv_st = FLAG_RCV_ST;
                else if (byte == C_RCV_RR) rcv_st = C_RCV_ST; //NOT FINISHED, INCOMPLETE
                else rcv_st = START_RCV_ST;
                break;

            case C_RCV_ST:
                if (byte == FLAG_RCV) rcv_st = FLAG_RCV_ST;
                else if (byte == (A_RCV_cmdT_ansR^C_RCV_RR)) rcv_st = BCC_RCV_ST;
                else rcv_st = START_RCV_ST;
                break;
            
            case BCC_RCV_ST:
                if (byte == FLAG_RCV) {rcv_st = STOP_RCV_ST; printf("FRAME RECEIVED  C->%x\n",C_RCV_RR); return 1;}
                else rcv_st = START_RCV_ST;
                break;

            default:
                break;
            }
        }
    }
    printf("NUMBER OF RETRANSMISSIONS EXCEEDED\n");
    return -1;
}

int readIFrame (unsigned char* frame) {

    int i = 0;
    
    unsigned char buf[256]; 

    RCV_STATE rcv_st = START_RCV_ST;

    while (rcv_st != STOP_RCV_ST)
    {
        int bytes = read(fd, buf, BUF_BYTE_SIZE);

        if (bytes > 0){
            int byte = buf[0];

            switch (rcv_st)
            {
            case START_RCV_ST:
                if (byte == FLAG_RCV) rcv_st = FLAG_RCV_ST;
                break;

            case FLAG_RCV_ST:
                if (byte == FLAG_RCV) rcv_st = FLAG_RCV_ST;
                else if (byte == A_RCV_cmdT_ansR) rcv_st = A_RCV_ST;
                else rcv_st = START_RCV_ST;
                break;

            case A_RCV_ST:
                if (byte == FLAG_RCV) rcv_st = FLAG_RCV_ST;
                else if (byte == 0) rcv_st = C_RCV_ST; // INCOMPLETE, verificar N(s)
                else rcv_st = START_RCV_ST;
                break;

            case C_RCV_ST:
                if (byte == FLAG_RCV) rcv_st = FLAG_RCV_ST;
                else if (byte == (A_RCV_cmdT_ansR^0)) rcv_st = BCC_RCV_ST;
                else rcv_st = START_RCV_ST;
                break;
            
            case BCC_RCV_ST:
                if (byte == FLAG_RCV) rcv_st = STOP_RCV_ST;
                else {
                    //printf("i -> %d\n",i);
                    frame[i++] = byte; 
                }
                break;

            default:
                break;
            }
        }
    }
    printf("FRAME RECEIVED I\n");
    return (i - 1);
}


unsigned char* controlPackageI (int st, int fileSize, const char* fileName, int fileNameSize, int* controlPackageSize) {
    *controlPackageSize = 9 * sizeof(unsigned char) + fileNameSize;
    unsigned char* package = (unsigned char*)malloc(*controlPackageSize);

    if (st == 0) package[0] = 0x02;
    else package[0] = 0x03;
    package[1] = 0x00; //t1
    package[2] = 0x04; //l1
    package[3] = (fileSize >> 24) & 0xff;
    package[4] = (fileSize >> 16) & 0xff;
    package[5] = (fileSize >> 8) & 0xff;
    package[6] = fileSize & 0xff;
    package[7] = 0x01; //t2
    package[8] = fileNameSize;
    for (int i = 0; i < fileNameSize; i++) package[9 + i] = fileName[i];

    return package;
}

unsigned char* dataPackageI (unsigned char* buf, int fileSize, int* packetSize){
    unsigned char* package = (unsigned char*)malloc(*packetSize + 4);
    package[0] = 0x01;
    package[1] = 0; //nº messages % 255
    package[2] = 0; //l2
    package[3] = 0; //l1

    memcpy(package + 4, buf, *packetSize);

    *packetSize += 4;

    //missing some parameters

    return package;
}
