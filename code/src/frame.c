#include "frame.h"
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

int alarmEnabled = FALSE;
int alarmCount = 0;

extern int fd;

// Alarm function handler
void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;

    printf("Alarm #%d\n", alarmCount);
}

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

int writeSetFrame_readUaFrame () {

    // Set alarm function handler
    (void)signal(SIGALRM, alarmHandler);

    unsigned char buf[BUF_BYTE_SIZE]; 

    RCV_STATE rcv_st = START_RCV_ST;
    int a_byte;
    int c_byte;

    while ((rcv_st != STOP_RCV_ST) && (alarmCount < 4))
    {
        

        if (alarmEnabled == FALSE)
        {
            writeFrame(A_RCV_cmdT_ansR,C_RCV_SET);
            alarm(3); // Set alarm to be triggered in 3s
            alarmEnabled = TRUE;
        }

        int bytes = read(fd, buf, BUF_BYTE_SIZE);

        if (bytes != 0){
            int byte = buf[0];

            switch (rcv_st)
            {
            case START_RCV_ST:
                if (byte == FLAG_RCV) rcv_st = FLAG_RCV_ST;
                break;

            case FLAG_RCV_ST:
                if (byte == FLAG_RCV) rcv_st = FLAG_RCV_ST;
                else if (byte == A_RCV_cmdT_ansR) {rcv_st = A_RCV_ST; a_byte = byte;}
                else rcv_st = START_RCV_ST;
                break;

            case A_RCV_ST:
                if (byte == FLAG_RCV) rcv_st = FLAG_RCV_ST;
                else if (byte == C_RCV_UA) {rcv_st = C_RCV_ST; c_byte = byte;}
                else rcv_st = START_RCV_ST;
                break;

            case C_RCV_ST:
                if (byte == FLAG_RCV) rcv_st = FLAG_RCV_ST;
                else if (byte == (a_byte^c_byte)) rcv_st = BCC_RCV_ST;
                else rcv_st = START_RCV_ST;
                break;
            
            case BCC_RCV_ST:
                if (byte == FLAG_RCV) {rcv_st = STOP_RCV_ST; alarm(0); printf("FRAME RECEIVED  C->%x\n",C_RCV_UA);return 1;}
                else rcv_st = START_RCV_ST;
                break;

            default:
                break;
            }
        }
    }
    return -1;
}

int readFrame (unsigned int A_RCV, unsigned int C_RCV){

    unsigned char buf[BUF_BYTE_SIZE]; 

    RCV_STATE rcv_st = START_RCV_ST;
    int a_byte;
    int c_byte;

    while (rcv_st != STOP_RCV_ST)
    {
        // Returns after 1 chars have been input
        int bytes = read(fd, buf, BUF_BYTE_SIZE);

        //printf("buf: %d\n", buf[0]);
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
                else if (byte == C_RCV) {rcv_st = C_RCV_ST; c_byte = byte;}
                else rcv_st = START_RCV_ST;
                break;

            case C_RCV_ST:
                if (byte == FLAG_RCV) rcv_st = FLAG_RCV_ST;
                else if (byte == (a_byte^c_byte)) rcv_st = BCC_RCV_ST;
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