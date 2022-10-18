#include "frame.h"
#include "link_layer.h"
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

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

    unsigned char buf[256]; 

    RCV_STATE rcv_st = START_RCV_ST;

    while (alarmCount < connectionParameters.nRetransmissions)
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