#include "runner.h"

/* Pin Work */
void initPinsRelaySwitch(void)
{
    for (int j = 0; j < 8 ; j++) {
        gpio_init(PICO_ROLE.channelsOut[j]);
        gpio_set_dir(PICO_ROLE.channelsOut[j], GPIO_OUT);
        gpio_init(PICO_ROLE.channelsIn[j]);
        gpio_set_dir(PICO_ROLE.channelsIn[j], GPIO_IN);
    }
}

void message_arrivedRelaySwitch(MessageData *msg_data)
{
    MQTTMessage *message = msg_data->message;
    char* topic;
    topic = msg_data->topicName->lenstring.data;
    uint channel=10;
    for(int i=0;i<8;i++){
        if(strstr(topic,PICO_ROLE.topicCom[i])!=NULL){
            channel=i;
            printf("Channel detected:%d,Com:%d\n",channel,message->payloadlen);
            printf("Get:%s\n",topic);
            printf("Set:%s\n",PICO_ROLE.topicCom[i]);
            break;
        }
    }
    if(channel!=10){
        if (message->payloadlen==2) {
            PICO_ROLE.channelsStatus[channel]=1;
            g_mqtt_message.payload = "ON";
        }else{
            PICO_ROLE.channelsStatus[channel]=0;
            g_mqtt_message.payload = "OFF";
        }
        gpio_put(PICO_ROLE.channelsOut[channel], PICO_ROLE.channelsStatus[channel]);
        g_mqtt_message.payloadlen = strlen(g_mqtt_message.payload);
        MQTTPublish(&PICO_ROLE.mqtt.client, PICO_ROLE.topicStat[channel], &g_mqtt_message);
    }
    topic[0]='\0';
    printf("%.*s", (uint32_t)message->payloadlen, (uint8_t *)message->payload);
}

//TASK
void button_taskRelaySwitch(void *argument)
{
    while (1)
    {
        for(int i=0; i<8; i++){
            if (gpio_get(PICO_ROLE.channelsIn[i])) {
                vTaskDelay(200);
                if (!gpio_get(PICO_ROLE.channelsIn[i])) {
                    printf("PRESS");
                    if (PICO_ROLE.channelsStatus[i]) {
                        g_mqtt_message.payload = "OFF";
                        PICO_ROLE.channelsStatus[i]=0;
                    }else{
                        g_mqtt_message.payload = "ON";
                        PICO_ROLE.channelsStatus[i]=1;
                    }
                    gpio_put(PICO_ROLE.channelsOut[i], PICO_ROLE.channelsStatus[i]);
                    g_mqtt_message.payloadlen = strlen(g_mqtt_message.payload);
                    MQTTPublish(&PICO_ROLE.mqtt.client, PICO_ROLE.topicStat[i], &g_mqtt_message);
                    vTaskDelay(200);
                }else{
                    printf("LONG PRESS");
                    if (PICO_ROLE.channelsStatus[i]) {
                        g_mqtt_message.payload = "OFF";
                        PICO_ROLE.channelsStatus[i]=0;
                    }else{
                        g_mqtt_message.payload = "ON";
                        PICO_ROLE.channelsStatus[i]=1;
                    }
                    gpio_put(PICO_ROLE.channelsOut[i], PICO_ROLE.channelsStatus[i]);
                    g_mqtt_message.payloadlen = strlen(g_mqtt_message.payload);
                    MQTTPublish(&PICO_ROLE.mqtt.client, PICO_ROLE.topicStat[i], &g_mqtt_message);
                    vTaskDelay(200);

                }
            }
        }
        vTaskDelay(100);
    }
}