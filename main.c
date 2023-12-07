#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

#include "port/port_common.h"

#include "wizchip_conf.h"
#include "w5x00_spi.h"

#include "mqtt_interface.h"
#include "MQTTClient.h"

#include "timer.h"

#include "roles/runner.h"
/**
 * ----------------------------------------------------------------------------------------------------
 * Macros
 * ----------------------------------------------------------------------------------------------------
 */

/* Task */
#define MQTT_TASK_STACK_SIZE 2048
#define MQTT_TASK_PRIORITY 10

#define YIELD_TASK_STACK_SIZE 512
#define YIELD_TASK_PRIORITY 8

#define BUTTON_TASK_STACK_SIZE 512
#define BUTTON_TASK_PRIORITY 7

/* Clock */
#define PLL_SYS_KHZ (133 * 1000)

/* Buffer */
#define ETHERNET_BUF_MAX_SIZE (1024 * 2)

/* Socket */
#define SOCKET_MQTT 0

/* Port */
#define PORT_MQTT 1883

/* Timeout */
#define DEFAULT_TIMEOUT 1000 // 1 second

/* MQTT */
#define MQTT_CLIENT_ID "rpi-pico"
#define MQTT_USERNAME "wiznet"
#define MQTT_PASSWORD "0123456789"
#define MQTT_CONFIG_TOPIC "homeassistant/switch/picoRelaySwitch1Channel"
#define MQTT_PUBLISH_TOPIC "mainController"
#define MQTT_PUBLISH_PAYLOAD "Hello, World!"
#define MQTT_PUBLISH_PERIOD (1000 * 10) // 10 seconds
#define MQTT_SUBSCRIBE_TOPIC "mainController"
#define MQTT_KEEP_ALIVE 60 // 60 milliseconds

/**
 * ----------------------------------------------------------------------------------------------------
 * Variables
 * ----------------------------------------------------------------------------------------------------
 */

/* Network */
static wiz_NetInfo g_net_info =
    {
        .mac = {0x00, 0x08, 0xDC, 0x12, 0x34, 0x56}, // MAC address
        .ip = {192, 168, 0, 121},                     // IP address
        .sn = {255, 255, 255, 0},                    // Subnet Mask
        .gw = {192, 168, 0, 1},                     // Gateway
        .dns = {8, 8, 8, 8},                         // DNS server
        .dhcp = NETINFO_STATIC                       // DHCP enable/disable
};

/* MQTT */
static uint8_t g_mqtt_send_buf[ETHERNET_BUF_MAX_SIZE] = {
    0,
};
static uint8_t g_mqtt_recv_buf[ETHERNET_BUF_MAX_SIZE] = {
    0,
};
static uint8_t g_mqtt_broker_ip[4] = {192, 168, 0, 189};
static Network g_mqtt_network;
static MQTTPacket_connectData g_mqtt_packet_connect_data = MQTTPacket_connectData_initializer;
static uint8_t g_mqtt_connect_flag = 0;

/* Timer  */
static volatile uint32_t g_msec_cnt = 0;


/**
 * ----------------------------------------------------------------------------------------------------
 * Functions
 * ----------------------------------------------------------------------------------------------------
 */

/* Task */
void mqtt_task(void *argument);
void yield_task(void *argument);
void button_task(void *argument);

/* Clock */
static void set_clock_khz(void);

/* Timer  */
static void repeating_timer_callback(void);
static time_t millis(void);

/**
 * ----------------------------------------------------------------------------------------------------
 * Main
 * ----------------------------------------------------------------------------------------------------
 */
int main()
{
    /* Initialize */
    set_clock_khz();

    stdio_init_all();

    initPicoRole();
    PICO_ROLE.initPins();

    wizchip_spi_initialize();
    wizchip_cris_initialize();

    wizchip_reset();
    wizchip_initialize();
    wizchip_check();

    wizchip_1ms_timer_initialize(repeating_timer_callback);

    xTaskCreate(mqtt_task, "MQTT_Task", MQTT_TASK_STACK_SIZE, NULL, MQTT_TASK_PRIORITY, NULL);
    xTaskCreate(yield_task, "YIEDL_Task", YIELD_TASK_STACK_SIZE, NULL, YIELD_TASK_PRIORITY, NULL);
    xTaskCreate(button_task, "BUTTON_Task", BUTTON_TASK_STACK_SIZE, NULL, BUTTON_TASK_PRIORITY, NULL);

    vTaskStartScheduler();

    while (1)
    {
        ;
    }
}

/**
 * ----------------------------------------------------------------------------------------------------
 * Functions
 * ----------------------------------------------------------------------------------------------------
 */

/* Task */
void mqtt_task(void *argument)
{
    uint8_t retval;

    network_initialize(g_net_info);

    /* Get network information */
    print_network_information(g_net_info);

    NewNetwork(&g_mqtt_network, SOCKET_MQTT);
    retval = ConnectNetwork(&g_mqtt_network, g_mqtt_broker_ip, PORT_MQTT);

    if (retval != 1)
    {
        printf(" Network connect failed\n");

        while (1)
        {
            vTaskDelay(1000 * 1000);
        }
    }

    /* Initialize MQTT client */
    MQTTClientInit(&g_mqtt_client, &g_mqtt_network, DEFAULT_TIMEOUT, g_mqtt_send_buf, ETHERNET_BUF_MAX_SIZE, g_mqtt_recv_buf, ETHERNET_BUF_MAX_SIZE);

    /* Connect to the MQTT broker */
    g_mqtt_packet_connect_data.MQTTVersion = 3;
    g_mqtt_packet_connect_data.cleansession = 1;
    g_mqtt_packet_connect_data.willFlag = 0;
    g_mqtt_packet_connect_data.keepAliveInterval = MQTT_KEEP_ALIVE;
    g_mqtt_packet_connect_data.clientID.cstring = MQTT_CLIENT_ID;
    g_mqtt_packet_connect_data.username.cstring = MQTT_USERNAME;
    g_mqtt_packet_connect_data.password.cstring = MQTT_PASSWORD;

    retval = MQTTConnect(&g_mqtt_client, &g_mqtt_packet_connect_data);

    if (retval < 0)
    {
        printf(" MQTT connect failed : %d\n", retval);

        while (1)
        {
            vTaskDelay(1000 * 1000);
        }
    }

    printf(" MQTT connected\n");

    /* Configure publish message */
    g_mqtt_message.qos = QOS0;
    g_mqtt_message.retained = 0;
    g_mqtt_message.dup = 0;
    g_mqtt_message.payload = "";
    g_mqtt_message.payloadlen = strlen(g_mqtt_message.payload);

    int j;
    for (j = 1; j<9; j++) {
        char topic[50];
        sprintf(topic, "%s%d/config", MQTT_CONFIG_TOPIC,j);
        char *resp = PICO_ROLE.getConfigPayload(j);
        g_mqtt_message.payload = resp;
        g_mqtt_message.payloadlen = strlen(g_mqtt_message.payload);
        retval = MQTTPublish(&g_mqtt_client, topic, &g_mqtt_message);
        if (retval < 0)
        {
            printf(" Publish Config failed : %d\n", retval);
            while (1)
            {
                vTaskDelay(1000 * 1000);
            }
        }
        printf("send config:   %d\n", j );
    }
    

    g_mqtt_message.payload = MQTT_PUBLISH_PAYLOAD;
    g_mqtt_message.payloadlen = strlen(g_mqtt_message.payload);

    /* Subscribe */
    for (j = 1; j<9; j++) {
        //sprintf(topicCom[j-1],"homeassistant/switch/picoRelaySwitch1Channel%d/set",j);
        retval = MQTTSubscribe(&g_mqtt_client, PICO_ROLE.topicCom[j-1], QOS0, PICO_ROLE.mqtt.message_arrived);
        if (retval < 0)
        {
            printf(" Subscribe failed : %d\n", retval);

            while (1)
            {
                vTaskDelay(1000 * 1000);
            }
        }
        printf(" Subscribed Channel %d\n%s\n",j,PICO_ROLE.topicCom[j-1]);
    }


    g_mqtt_connect_flag = 1;

    while (1)
    {
        /* Publish */
        g_mqtt_message.payload = MQTT_PUBLISH_PAYLOAD;
        g_mqtt_message.payloadlen = strlen(g_mqtt_message.payload);
        retval = MQTTPublish(&g_mqtt_client, MQTT_PUBLISH_TOPIC, &g_mqtt_message);

        if (retval < 0)
        {
            printf(" Publish failed : %d\n", retval);

            while (1)
            {
                vTaskDelay(1000 * 1000);
            }
        }

        printf(" Published\n");

        vTaskDelay(MQTT_PUBLISH_PERIOD);
    }
}

void yield_task(void *argument)
{
    int retval;

    while (1)
    {
        if (g_mqtt_connect_flag == 1)
        {
            if ((retval = MQTTYield(&g_mqtt_client, g_mqtt_packet_connect_data.keepAliveInterval)) < 0)
            {
                printf(" Yield error : %d\n", retval);

                while (1)
                {
                    vTaskDelay(1000 * 1000);
                }
            }
        }

        vTaskDelay(10);
    }
}

void button_task(void *argument)
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
                    MQTTPublish(&g_mqtt_client, PICO_ROLE.topicStat[i], &g_mqtt_message);
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
                    MQTTPublish(&g_mqtt_client, PICO_ROLE.topicStat[i], &g_mqtt_message);
                    vTaskDelay(200);

                }
            }
        }
        vTaskDelay(100);
    }
}

/* Clock */
static void set_clock_khz(void)
{
    // set a system clock frequency in khz
    set_sys_clock_khz(PLL_SYS_KHZ, true);

    // configure the specified clock
    clock_configure(
        clk_peri,
        0,                                                // No glitchless mux
        CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS, // System PLL on AUX mux
        PLL_SYS_KHZ * 1000,                               // Input frequency
        PLL_SYS_KHZ * 1000                                // Output (must be same as no divider)
    );
}

/* Timer */
static void repeating_timer_callback(void)
{
    g_msec_cnt++;

    MilliTimer_Handler();
}

static time_t millis(void)
{
    return g_msec_cnt;
}