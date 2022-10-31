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

    sleep(1);

    if(llopen(ll) == -1){
        perror("llopen fail");
        return;
    }
    printf("\n[/] LLOPEN ESTABLISHED\n");


    if (ll.role == LlTx) {
        printf("\n[/] STARTING FILE TRANSFER\n");

        int file;
        unsigned char buf[1000];
        float totalSent = 0;
        int nMessage = 0;
        
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

        int bytesRead;
        
        llwrite(start, sizeof(start));
        while ((bytesRead = read(file, buf, 256)) > 0){
            unsigned char* data = dataPackageI(buf, fileSize, &bytesRead, &nMessage);
            int bytesWrite = llwrite(data, bytesRead);
            if (bytesWrite == -1) return;
            printProgressBar(totalSent, fileSize, nMessage);
            totalSent += bytesWrite - 4;
            nMessage++;
            //sleep(1);
        }
        close(file);

        llwrite(end, sizeof(end));
        
        //printf("total bytes sent: %f\n", totalSent);
    }

    else if (ll.role == LlRx) {

        int file;
        unsigned char packet[1000];
        int bytesRead;
        float totalReceived = 0;
        int fileSize;
        int n = 0;

        if ((file = open(filename,O_WRONLY | O_CREAT)) == -1) printf("Cannot open file\n");

        while (1){
            if ((bytesRead = llread (packet)) > 0){
                printProgressBar(totalReceived, fileSize, n);
                totalReceived += bytesRead;

                if (packet[0] == 0x02){
                    //printf("receive start data packet\n");
                    //get file size from here
                    int len = (int)packet[2];
                    for (int i = 0; i < len; i++)
                    {
                        fileSize = fileSize * 256 + (int)packet[3 + i];
                    }
                }
                else if (packet[0] == 0x03){
                    //printf("received end data packet\n");
                    break;
                }
                else if (packet[0] == 0x01){//NOT IMPLEMENTED
                    //printf("received header data packet\n");
                    write(file, &packet[4], bytesRead - 4);
                    n = (n + 1 < 255) ? packet[1] : packet[1] + 255;
                }
                //printf("%d bytes received\n", bytesRead);
            }

            
        }
        close(file);
        //printf("total bytes received: %f\n", totalReceived);
    }

    if (llclose(0) == -1){
        perror("llclose fail");
        return;
    }
    printf("\n[+] LLCLOSE ESTABLISHED\n");
}
