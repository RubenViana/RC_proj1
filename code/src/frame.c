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
extern int sequenceNumberI;

extern int receivedFrames;
extern int sentFrames;
extern int nTimeouts;

int writeFrame (unsigned int A_RCV, unsigned int C_RCV){

    unsigned char frame[CMD_ANS_FRAME_SIZE];

    frame[0] = 0x7e;
    frame[1] = A_RCV;
    frame[2] = C_RCV;
    frame[3] = frame[1] ^ frame[2];
    frame[4] = 0x7e;

    int bytes = write(fd, frame, CMD_ANS_FRAME_SIZE);

    //printf("FRAME SENT  C->%x\n",C_RCV);
    sentFrames++;
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
    //printf("FRAME RECEIVED  C->%x\n",C_RCV);
    receivedFrames++;
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
            nTimeouts+=alarmCount;
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
                if (byte == FLAG_RCV) {rcv_st = STOP_RCV_ST; receivedFrames++;/* printf("FRAME RECEIVED  C->%x\n",C_RCV_r);*/ return 1;}
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

unsigned char bcc_2(const unsigned char* frame, int length) {

  unsigned char bcc2 = frame[0];

  for(int i = 1; i < length; i++){
    bcc2 = bcc2 ^ frame[i];
  }

  return bcc2;
}

int check_bbc_2(unsigned char* frame, int length) {
    unsigned char bcc2 = frame[0];

    for(int i = 1; i < length; i++){
        bcc2 = bcc2 ^ frame[i];
    }

    if (bcc2 == frame[length - 1]) return TRUE;
    else return FALSE;
}

int writeIFrame (unsigned char* frame, int frameSize) {
    unsigned char buf[256]; 

    RCV_STATE rcv_st = START_RCV_ST;

    int retr = FALSE;

    unsigned char C;

    alarmCount = 0;
    alarmEnabled = FALSE;

    while (alarmCount < connectionParameters.nRetransmissions && rcv_st != STOP_RCV_ST)
    {
        if (alarmEnabled == FALSE || retr == TRUE)
        {
            write(fd, frame, frameSize);
            //printf("FRAME SENT I\n");
            alarm(connectionParameters.timeout);
            sentFrames++;
            nTimeouts+=alarmCount;
            alarmEnabled = TRUE;
            retr = FALSE;
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
                else if ((byte == C_RCV_RR1) || (byte == C_RCV_RR0) || (byte == C_RCV_REJ0) || (byte == C_RCV_REJ1)){
                    rcv_st = C_RCV_ST;
                    C = byte;
                }
                else rcv_st = START_RCV_ST;
                break;

            case C_RCV_ST:
                if (byte == FLAG_RCV) rcv_st = FLAG_RCV_ST;
                else if (byte == (A_RCV_cmdT_ansR^C)) rcv_st = BCC_RCV_ST;
                else rcv_st = START_RCV_ST;
                break;
            
            case BCC_RCV_ST:
                if (byte == FLAG_RCV) {
                    //rcv_st = STOP_RCV_ST;
                    if ((C == C_RCV_RR0 && sequenceNumberI == 1) || (C == C_RCV_RR1 && sequenceNumberI == 0)){
                        //printf("FRAME RECEIVED RR\n");
                        sequenceNumberI ^= 1;
                        receivedFrames++;
                        return 1;
                    }
                    else if ((C == C_RCV_REJ0) || (C == C_RCV_REJ1)){
                        //printf("FRAME RECEIVED REJ\n");
                        retr = TRUE;
                        alarmCount++;
                        break;
                    }
                }
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

    unsigned char byte; 

    RCV_STATE rcv_st = START_RCV_ST;

    while (rcv_st != STOP_RCV_ST)
    {
        int bytes = read(fd, &byte, sizeof(unsigned char));

        if (bytes > 0){

            switch (rcv_st)
            {
            case START_RCV_ST:
                i = 0;
                if (byte == FLAG_RCV) {rcv_st = FLAG_RCV_ST; frame[i++] = byte;}
                break;

            case FLAG_RCV_ST:
                if (byte == FLAG_RCV) rcv_st = FLAG_RCV_ST;
                else if (byte == A_RCV_cmdT_ansR) {rcv_st = A_RCV_ST; frame[i++] = byte;}
                else {rcv_st = START_RCV_ST; i = (int) rcv_st;}
                break;

            case A_RCV_ST:
                if (byte == FLAG_RCV) {rcv_st = FLAG_RCV_ST; i = (int) rcv_st;}
                else if (byte == C_RCV_I0){
                    rcv_st = C_RCV_ST;
                    //sequenceNumberI = 0;
                    frame[i++] = byte;
                }
                else if (byte == C_RCV_I1){
                    rcv_st = C_RCV_ST;
                    //sequenceNumberI = 1;
                    frame[i++] = byte;
                }
                else {rcv_st = START_RCV_ST; i = (int) rcv_st;}
                break;

            case C_RCV_ST:
                if (byte == FLAG_RCV) {rcv_st = FLAG_RCV_ST; i = (int) rcv_st;}
                else if (byte == (A_RCV_cmdT_ansR^frame[2])) {rcv_st = BCC_RCV_ST; frame[i++] = byte;}
                else {rcv_st = START_RCV_ST; i = (int) rcv_st;}
                break;
            
            case BCC_RCV_ST:
                if (byte == FLAG_RCV) {
                    frame[i] = byte;
                    rcv_st = STOP_RCV_ST;
                }
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
    receivedFrames++;
    return i - 4;
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

unsigned char* dataPackageI (unsigned char* buf, int fileSize, int* packetSize, int* nMessage){
    unsigned char* package = (unsigned char*)malloc(*packetSize + 4);
    package[0] = 0x01;
    package[1] = *nMessage % 255; //nº messages % 255
    package[2] = *packetSize/256; //l2
    package[3] = *packetSize % 256; //l1

    memcpy(package + 4, buf, *packetSize);

    *packetSize += 4;
    //printf("nMess %d", *nMessage);

    return package;
}


unsigned int byteStuffing (unsigned char* frame, int length)
{
    unsigned char aux[length + 6];

    for(int i = 0; i < length + 6 ; i++){
        aux[i] = frame[i];
    }

    // passes information from the frame to aux
    
    int finalLength = 4;
    // parses aux buffer, and fills in correctly the frame buffer
    for(int i = 4; i < (length + 6); i++){

        if(aux[i] == FLAG_RCV && i != (length + 5)) {
        frame[finalLength] = ESCAPE;
        frame[finalLength+1] = BYTE_STUFFING_FLAG;
        finalLength = finalLength + 2;
        }
        else if(aux[i] == ESCAPE && i != (length + 5)) {
        frame[finalLength] = ESCAPE;
        frame[finalLength+1] = BYTE_STUFFING_ESCAPE;
        finalLength = finalLength + 2;
        }
        else{
        frame[finalLength] = aux[i];
        finalLength++;
        }
    }

    return finalLength;
}

unsigned int byteDestuffing (unsigned char* frame, int length)
{   
    // allocates space for the maximum possible frame length read (length of the data packet + bcc2, already with stuffing, plus the other 5 bytes in the frame)
    unsigned char aux[length + 5];

    // copies the content of the frame (with stuffing) to the aux frame
    for(int i = 0; i < (length + 5) ; i++) {
        aux[i] = frame[i];
    }

    int finalLength = 4;

    // iterates through the aux buffer, and fills the frame buffer with destuffed content
    for(int i = 4; i < (length + 5); i++) {

        if(aux[i] == ESCAPE){
        if (aux[i+1] == BYTE_STUFFING_ESCAPE) {
            frame[finalLength] = ESCAPE;
        }
        else if(aux[i+1] == BYTE_STUFFING_FLAG) {
            frame[finalLength] = FLAG_RCV;
        }
        i++;
        finalLength++;
        }
        else{
        frame[finalLength] = aux[i];
        finalLength++;
        }
    }

    return finalLength;
}

const int PROGRESS_BAR_LENGTH = 51;

void printProgressBar(float current, float total, int n) {
    
    CLEAR_SCREEN;

    printf("[+] LLOPEN ESTABLISHED\n");
    printf("\n[+] STARTING FILE TRANSFER ...\n\n");

    float percentage = 0;
    percentage = (current / total)*100;
    percentage = (percentage > 100) ? 100 : percentage;

    char op = (sequenceNumberI == 0) ? '/' : '\\';
    if (percentage == 100) op = '+';
    printf("\r[%c] [", op);

    int i, len = PROGRESS_BAR_LENGTH;
    int pos = (percentage/100) * len ;

    for (i = 0; i < len; i++)
        i <= pos ? printf("#") : printf(" ");

    printf("]%6.2f%%\n\n", percentage);

    if (connectionParameters.role == LlTx){
        printf("    Sending Packet %d ...\n", n);
    }
    else if (connectionParameters.role == LlRx){
        printf("    Receiving Packet %d\n\n", n);       
    }
}