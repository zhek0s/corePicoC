#include "runner.h"

_PICO_ROLE  PICO_ROLE;
void initPicoRole(void)
{
    generateTopicStat();
    generateTopicCom();
    uint channelsOut[8] = {7,6,5,4,3,2,22,28};
    uint channelsIn[8] = {15,14,13,12,11,10,9,8};
    uint channelsStatus[8] = {0,0,0,0,0,0,0,0};
    for(int i=0;i<8;i++){
        PICO_ROLE.channelsOut[i] = channelsOut[i];
        PICO_ROLE.channelsIn[i] = channelsIn[i];
        PICO_ROLE.channelsStatus[i] = channelsStatus[i];
    }
    PICO_ROLE.getConfigPayload = _getConfigPayload;
#if _PICO_ROLE_DEF_ == relaySwitch
    PICO_ROLE.initPins = initPinsRelaySwitch;
    PICO_ROLE.mqtt.message_arrived = message_arrivedRelaySwitch;
#endif
    printf("Role set!\n");
}

void generateTopicStat(void)
{
    char serial[10];
    char name[20];
    char type[20];
    int channels = 1;
#if _PICO_ROLE_DEF_ == relaySwitch
    strcpy(name,_PICO_RELAYSWITCH_NAME_);
    strcpy(type,_PICO_RELAYSWITCH_TYPE_);
    strcpy(serial,_PICO_RELAYSWITCH_SERIAL_);
    channels = _PICO_RELAYSWITCH_CHANNELS_;
#endif
     //"homeassistant/switch/picoRelaySwitch1Channel1/switch"
    for(int i=0;i<channels;i++){
        char topic[80];
        sprintf(topic,"homeassistant/%s/%s%sChannel%d/%s",type,name,serial,i+1,type);
        strcpy(PICO_ROLE.topicStat[i],topic);
    }
}

void generateTopicCom(void)
{
    char serial[10];
    char name[20];
    char type[20];
    int channels = 1;
#if _PICO_ROLE_DEF_ == relaySwitch
    strcpy(name,_PICO_RELAYSWITCH_NAME_);
    strcpy(type,_PICO_RELAYSWITCH_TYPE_);
    strcpy(serial,_PICO_RELAYSWITCH_SERIAL_);
    channels = _PICO_RELAYSWITCH_CHANNELS_;
#endif
     //"homeassistant/switch/picoRelaySwitch1Channel1/set"
    for(int i=0;i<channels;i++){
        char topic[80];
        sprintf(topic,"homeassistant/%s/%s%sChannel%d/set",type,name,serial,i+1);
        strcpy(PICO_ROLE.topicCom[i],topic);
    }
}

char * _getConfigPayload(int j)
{
    int i = j;
    char _topic[] = "homeassistant";
    char conf[450];
    char serial[10];
    char name[20];
    char type[20];
#if _PICO_ROLE_DEF_ == relaySwitch
    strcpy(name,_PICO_RELAYSWITCH_NAME_);
    strcpy(type,_PICO_RELAYSWITCH_TYPE_);
    strcpy(serial,_PICO_RELAYSWITCH_SERIAL_);
#endif
    sprintf(conf,
            "{\n"
            "\"unique_id\":\"%s%sChannel%d\",\n"
            "\"name\":\"%s%sChannel%d\",\n"
            "\"state_topic\":\"%s/%s/%s%sChannel%d/%s\",\n"
            "\"command_topic\":\"%s/%s/%s%sChannel%d/set\",\n"
            //"\"availability\":\"%s%s/switch/available\",\n"
            "\"payload_on\":\"ON\",\n"
            "\"payload_off\":\"OFF\",\n"
            "\"state_on\":\"ON\",\n"
            "\"state_off\":\"OFF\"\n"
            "}",
            name,serial, i,
            name,serial,i,
            _topic,type,name,serial,i,type,
            _topic,type,name,serial,i);
            //_topic,name);
    char *resp = conf;
    return resp;
}