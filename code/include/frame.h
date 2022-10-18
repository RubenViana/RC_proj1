#ifndef _FRAME_H_
#define _FRAME_H_

#define _POSIX_SOURCE 1 // POSIX compliant source

#define BUF_BYTE_SIZE 1
#define CMD_ANS_FRAME_SIZE 5

#define FLAG_RCV 0x7e
#define A_RCV_cmdT_ansR 0x03
#define A_RCV_cmdR_ansT 0x01
#define C_RCV_SET 0x03
#define C_RCV_UA 0x07
#define C_RCV_DISC 0x0b
//#define BCC_RCV A_RCV ^ C_RCV_SET

typedef enum {START_RCV_ST, FLAG_RCV_ST, A_RCV_ST, C_RCV_ST, BCC_RCV_ST, STOP_RCV_ST} RCV_STATE;

int writeFrame (unsigned int A_RCV, unsigned int C_RCV);

int readFrame (unsigned int A_RCV, unsigned int C_RCV);

int changeState (int byte, unsigned int A_RCV, unsigned int C_RCV);

int writeReadWithRetr (unsigned int A_RCV_w, unsigned int C_RCV_w, unsigned int A_RCV_r, unsigned int C_RCV_r);

void alarmHandler(int signal);

#endif // _FRAME_H_