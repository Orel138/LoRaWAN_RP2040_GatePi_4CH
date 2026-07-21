#ifndef RELAY_REPORT_H
#define RELAY_REPORT_H

#include <stdint.h>
#include <stdbool.h>

/* Periodic LoRaWAN uplink routine that reports the state of the four on-board
 * relays. The relay states are packed into a single byte, bit i (0..3) being
 * set when relay (i + 1) is on. */

/* Create the background task. Call once at start-up. The routine starts
 * disabled. */
void RelayReport_Init(void);

/* Enable the periodic uplink routine. interval_s is the delay between frames
 * in seconds (clamped to a safe minimum). Returns false if the task has not
 * been initialised. */
bool RelayReport_Start(uint32_t interval_s);

/* Disable the periodic uplink routine. */
void RelayReport_Stop(void);

/* Whether the routine is currently enabled. */
bool RelayReport_IsRunning(void);

/* Current interval between frames, in seconds. */
uint32_t RelayReport_GetIntervalS(void);

/* Apply a downlink payload controlling the report routine. The payload is the
 * hex-string form of the downlink bytes and must start with 'F':
 *   "F0"             -> stop the routine
 *   "F1"             -> start with the current (or default) interval
 *   "F1" + 4 hex     -> start with a 16-bit interval in seconds (e.g. "F1003C")
 * Returns true when the payload was a report command (even if it just stopped
 * the routine), false when it is not addressed to this handler. */
bool RelayReport_ProcessLoRaPayload(const char *payload, uint16_t length);

#endif /* RELAY_REPORT_H */
