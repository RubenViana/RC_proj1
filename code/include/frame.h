#ifndef _FRAME_H_
#define _FRAME_H_

#define _POSIX_SOURCE 1 // POSIX compliant source

#define BUF_BYTE_SIZE 1
#define CMD_ANS_FRAME_SIZE 5

#define FLAG_RCV 0x7e
#define ESCAPE 0x7d
#define A_RCV_cmdT_ansR 0x03
#define A_RCV_cmdR_ansT 0x01
#define C_RCV_SET 0x03
#define C_RCV_UA 0x07
#define C_RCV_DISC 0x0b
#define C_RCV_RR0 0x05
#define C_RCV_RR1 0x85
#define C_RCV_I0 0x00
#define C_RCV_I1 0x40
#define C_RCV_REJ0 0x01
#define C_RCV_REJ1 0x81
#define BYTE_STUFFING_ESCAPE 0x5D
#define BYTE_STUFFING_FLAG 0x5E
#define DATA_START 4


typedef enum {START_RCV_ST, FLAG_RCV_ST, A_RCV_ST, C_RCV_ST, BCC_RCV_ST, STOP_RCV_ST} RCV_STATE;

int writeFrame (unsigned int A_RCV, unsigned int C_RCV);

int readFrame (unsigned int A_RCV, unsigned int C_RCV);

int writeReadWithRetr (unsigned int A_RCV_w, unsigned int C_RCV_w, unsigned int A_RCV_r, unsigned int C_RCV_r);

unsigned char bcc_2 (const unsigned char* frame, int length);

int check_bbc_2(unsigned char* frame, int length);

int writeIFrame (unsigned char* frame, int frameSize);

int readIFrame (unsigned char* frame);

unsigned char* controlPackageI (int st, int fileSize, const char* fileName, int fileNameSize, int* controlPackageSize);

unsigned char* dataPackageI (unsigned char* buf, int fileSize, int* packetSize);

unsigned int byteStuffing (unsigned char* frame, int length);

unsigned int byteDestuffing (unsigned char* frame, int length);

#endif // _FRAME_H_