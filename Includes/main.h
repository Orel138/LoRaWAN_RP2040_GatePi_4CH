#ifndef MAIN_H
#define MAIN_H

/* ------------------------------------------------------------------------- *
 * LoRaWAN OTAA credentials
 *
 * Replace the placeholders below with the identifiers provided by your
 * network server (e.g. The Things Network). All values are hex strings.
 * ------------------------------------------------------------------------- */
#define LORAWAN_DEVEUI      "0000000000000000"
#define LORAWAN_APPEUI      "0000000000000000"
#define LORAWAN_APPKEY      "00000000000000000000000000000000"

/* RUI3 band code (AT+BAND):
 *   0 = EU433   1 = CN470   2 = RU864   3 = IN865   4 = EU868
 *   5 = US915   6 = AU915   7 = KR920   8 = AS923-1
 * Default is EU868. Change to match your region. */
#define LORAWAN_BAND        "4"

/* LoRaWAN device class: use 'C' so downlink relay commands can be received
 * at any time (continuous receive), not only after an uplink. */
#define LORAWAN_CLASS       'C'

/* Number of join attempts performed at start-up before giving up */
#define LORAWAN_JOIN_ATTEMPTS   3U

#endif /* MAIN_H */
