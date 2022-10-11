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

    llopen(ll);

    printf("\n...TRANSFER...\n\n");

    llclose(0);

    printf("END APP\n");

}
