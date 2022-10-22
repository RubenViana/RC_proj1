// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"
#include "frame.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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

        FILE* file;
        unsigned char buf[1000];
        int total;
        
        if ((file = fopen(filename,"r")) == NULL) printf("Cannot open file\n");

        int filenameSize = strlen(filename);

        int controlPacketSize = 0;

        //obtain file size
        int fileSize;
        fseek(file, 0, SEEK_END);
        fileSize = ftell(file);
        rewind(file);


        unsigned char* start = controlPackageI(0, fileSize, filename, filenameSize, &controlPacketSize);
        unsigned char* end = controlPackageI(1, fileSize, filename, filenameSize, &controlPacketSize);

        FILE* t;

        llwrite(start, sizeof(start));

        while (!feof(file)){
            int bytesRead = fread(buf, sizeof(unsigned char), 900, file);

            unsigned char* data = dataPackageI(buf, fileSize, &bytesRead);
            int bytesWrite = llwrite(data, bytesRead);
            printf("%d bytes sent\n", bytesWrite);
            total += bytesWrite;
            
            t = fopen("test.gif","w");                      //for testing while not finish
            fwrite(buf, sizeof(unsigned char), bytesRead, t);
        }
        fclose(file);

        llwrite(end, sizeof(end));

        fclose(t);
        
        printf("TOTAL -> %d",total);
    }

    else if (ll.role == LlRx) {
        printf("\n----------------llread----------------\n");

        FILE* file;
        unsigned char packet[1000];
        int bytesRead;

        if ((file = fopen(filename,"w")) == NULL) printf("Cannot open file\n");

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
                    fwrite(packet + 4, sizeof(unsigned char), bytesRead, file);
                }
            }
            printf("%d bytes received\n", bytesRead);
            
        }
        fclose(file);
    }


    printf("\n----------------llclose----------------\n");
    llclose(0);

    printf("END APP\n");

}
