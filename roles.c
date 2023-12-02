

void mqtt_task(void *argument);
void yield_task(void *argument);

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
    g_mqtt_message.payload = MQTT_PUBLISH_PAYLOAD;
    g_mqtt_message.payloadlen = strlen(g_mqtt_message.payload);

    /* Subscribe */
    retval = MQTTSubscribe(&g_mqtt_client, MQTT_SUBSCRIBE_TOPIC, QOS0, message_arrived);

    if (retval < 0)
    {
        printf(" Subscribe failed : %d\n", retval);

        while (1)
        {
            vTaskDelay(1000 * 1000);
        }
    }

    printf(" Subscribed\n");

    g_mqtt_connect_flag = 1;

    while (1)
    {
        /* Publish */
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