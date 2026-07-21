#include "relay.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"

static const uint8_t relayPins[RELAY_COUNT] =
{
    RELAY_1_PIN,
    RELAY_2_PIN,
    RELAY_3_PIN,
    RELAY_4_PIN
};

void Relay_Init(void)
{
    for(uint8_t i = 0U; i < RELAY_COUNT; i++)
    {
        gpio_init(relayPins[i]);
        gpio_set_dir(relayPins[i], GPIO_OUT);
        gpio_put(relayPins[i], 0);
    }
}

bool Relay_Set(uint8_t relay, bool on)
{
    if((relay < 1U) || (relay > RELAY_COUNT))
    {
        return false;
    }

    gpio_put(relayPins[relay - 1U], on ? 1 : 0);
    return true;
}

void Relay_SetAll(bool on)
{
    for(uint8_t i = 0U; i < RELAY_COUNT; i++)
    {
        gpio_put(relayPins[i], on ? 1 : 0);
    }
}

bool Relay_Get(uint8_t relay)
{
    if((relay < 1U) || (relay > RELAY_COUNT))
    {
        return false;
    }

    return gpio_get(relayPins[relay - 1U]) != 0;
}

bool Relay_ProcessLoRaPayload(const char *payload, uint16_t length)
{
    if((payload == NULL) || (length < 2U))
    {
        return false;
    }

    char stateChar = payload[0];
    char relayChar = payload[1];

    /* First character is the requested state ('0' = off, '1' = on) */
    if((stateChar != '0') && (stateChar != '1'))
    {
        return false;
    }

    /* Second character is the relay index ('1'..RELAY_COUNT) */
    if((relayChar < '1') || (relayChar > (char)('0' + RELAY_COUNT)))
    {
        return false;
    }

    uint8_t relay = (uint8_t)(relayChar - '0');
    bool on = (stateChar == '1');

    return Relay_Set(relay, on);
}
