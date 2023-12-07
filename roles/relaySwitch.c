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