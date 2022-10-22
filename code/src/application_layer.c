// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"
#include <string.h>
#include <stdio.h>

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

        FILE* t;
        while (!feof(file)){
            int bytesRead = fread(buf, sizeof(unsigned char), sizeof(buf), file);
            int bytesWrite = llwrite(buf, bytesRead);
            printf("%d bytes sent\n", bytesWrite);
            total += bytesWrite;
            
            t = fopen("test.gif","w");                      //for testing while not finish
            fwrite(buf, sizeof(unsigned char), bytesRead, t);
        }
        fclose(file);

        fclose(t);
        buf[0] = 0xAA;      //for testing
        buf[1] = 0xAA;
        
        int bytesWrite = llwrite(buf, 2);
        printf("%d bytes sent\n", bytesWrite);
        
        printf("TOTAL -> %d",total);
    }

    else if (ll.role == LlRx) {
        printf("\n----------------llread----------------\n");

        FILE* file;
        unsigned char packet[1000];
        int bytesRead;

        if ((file = fopen(filename,"w")) == NULL) printf("Cannot open file\n");

        while ((bytesRead = llread (packet)) > 0){
             printf("%d bytes received\n", bytesRead);
             if (packet[0] == 0xAA && packet[1] == 0xAA) break;    //for testing
             fwrite(packet, sizeof(unsigned char), bytesRead, file);
        }
        fclose(file);
    }


    printf("\n----------------llclose----------------\n");
    llclose(0);

    printf("END APP\n");

}
