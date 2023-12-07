#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <string.h>

#ifndef _PICO_ROLE_DEF_
#define _PICO_ROLE_DEF_                      relaySwitch   // relaySwitch
#endif

#define _PICO_RELAYSWITCH_SERIAL_ "1"
#define _PICO_RELAYSWITCH_NAME_ "picoRelaySwitch"
#define _PICO_RELAYSWITCH_TYPE_ "switch"
#define _PICO_RELAYSWITCH_CHANNELS_ 8


typedef struct __PICO_ROLE
{
    char topicStat[8][80];
    char topicCom[8][80];
    uint channelsOut[8];
    uint channelsIn[8];
    uint channelsStatus[8];
    char* (*getConfigPayload)(int j);
}_PICO_ROLE;
extern _PICO_ROLE  PICO_ROLE;

void initPicoRole(void);
char * _getConfigPayload(int j);
void generateTopicStat(void);
void generateTopicCom(void);