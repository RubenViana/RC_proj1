// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"
#include "frame.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>  

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    LinkLayer ll;
    strcpy(ll.serialPort, serialPort);
    ll.baudRate = baudRate;
    ll.nRetransmissions = nTries;
    ll.timeout = timeout;
    if (strcmp(role,"rx") == 0) ll.role = LlRx;
    if (strcmp(role,"tx") == 0) ll.role = LlTx;

    printf("\n----------------llopen----------------\n");
    llopen(ll);


    if (ll.role == LlTx) {
        printf("\n----------------llwrite----------------\n");

        int file;
        unsigned char buf[1000];
        int total;
        
        if ((file = open(filename,O_RDONLY)) == -1) printf("Cannot open file\n");

        int filenameSize = strlen(filename);

        int controlPacketSize = 0;

        //obtain file size
        int fileSize;

        struct stat st;
        stat(filename, &st);
        fileSize = st.st_size;


        unsigned char* start = controlPackageI(0, fileSize, filename, filenameSize, &controlPacketSize);
        unsigned char* end = controlPackageI(1, fileSize, filename, filenameSize, &controlPacketSize);

        int t;
        t = open("test.gif",O_WRONLY);                      //for testing while not finish
        llwrite(start, sizeof(start));

        int bytesRead;
        while ((bytesRead = read(file, buf, 400)) > 0){
            write(t, buf, bytesRead);//for testing
            unsigned char* data = dataPackageI(buf, fileSize, &bytesRead);
            int bytesWrite = llwrite(data, bytesRead);
            printf("%d bytes sent\n", bytesWrite);
            total += bytesWrite;
        }
        close(file);

        llwrite(end, sizeof(end));

        close(t);
        
        printf("TOTAL -> %d",total);
    }

    else if (ll.role == LlRx) {
        printf("\n----------------llread----------------\n");

        int file;
        unsigned char packet[1000];
        int bytesRead;

        if ((file = open(filename,O_WRONLY)) == -1) printf("Cannot open file\n");

        while (1){
            if ((bytesRead = llread (packet)) > 0){
                if (packet[0] == 0x02){
                    printf("receive start data packet\n");  //NOT FINISHED
                }
                else if (packet[0] == 0x03){
                    printf("received end data packet\n");
                    break;
                }
                else if (packet[0] == 0x01){//NOT IMPLEMENTED
                    printf("received header data packet\n");
                    write(file, packet + 4, bytesRead);
                }
            }
            printf("%d bytes received\n", bytesRead);
            
        }
        close(file);
    }


    printf("\n----------------llclose----------------\n");
    llclose(0);

    printf("END APP\n");

}
