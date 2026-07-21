#ifndef RELAY_H
#define RELAY_H

#include <stdint.h>
#include <stdbool.h>

/* On-board relays wired to the RP2040 GPIOs (4CH PCB) */
#define RELAY_COUNT     4U
#define RELAY_1_PIN     18U
#define RELAY_2_PIN     19U
#define RELAY_3_PIN     20U
#define RELAY_4_PIN     21U

/* Configure the relay GPIOs as outputs and switch them all off */
void Relay_Init(void);

/* Drive a single relay (relay index is 1..RELAY_COUNT). Returns false on
 * an out-of-range index. */
bool Relay_Set(uint8_t relay, bool on);

/* Drive every relay to the same state */
void Relay_SetAll(bool on);

/* Read back the current state of a single relay (false if index invalid) */
bool Relay_Get(uint8_t relay);

/* Apply a LoRa/LoRaWAN payload of the form "<state><relay>" where the first
 * character is the target state ('0' = off, '1' = on) and the second is the
 * relay index ('1'..'4'). Example: "01" -> relay 1 off, "13" -> relay 3 on.
 * Returns true when a valid command was applied. */
bool Relay_ProcessLoRaPayload(const char *payload, uint16_t length);

#endif /* RELAY_H */
