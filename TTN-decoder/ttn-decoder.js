/**
 * TTN uplink payload decoder for the LoRaWAN RP2040 GatePi 4CH gateway.
 *
 * Relay status uplink (relay-report routine):
 *   - fPort 2
 *   - 1 byte, where bit i (0..3) reports relay (i + 1):
 *       bit set   -> relay ON
 *       bit clear -> relay OFF
 *
 * Example: 0x0A (0b0000_1010) -> relay 2 ON, relay 4 ON, relays 1 & 3 OFF.
 */

var RELAY_STATUS_PORT = 2;
var RELAY_COUNT = 4;

function toHexByte(value) {
  return "0x" + ("0" + (value & 0xff).toString(16).toUpperCase()).slice(-2);
}

function decodeUplink(input) {
  var bytes = input.bytes || [];
  var fPort = input.fPort;
  var warnings = [];
  var errors = [];

  if (bytes.length < 1) {
    errors.push("Empty payload: expected 1 relay-status byte");
    return { data: {}, warnings: warnings, errors: errors };
  }

  if (bytes.length > 1) {
    warnings.push(
      "Unexpected payload length: " + bytes.length +
      " bytes (expected 1); decoding the first byte only"
    );
  }

  if (fPort !== undefined && fPort !== null && fPort !== RELAY_STATUS_PORT) {
    warnings.push(
      "Unexpected fPort " + fPort +
      " (relay status is sent on port " + RELAY_STATUS_PORT + ")"
    );
  }

  var status = bytes[0] & 0xff;

  var data = {
    raw: toHexByte(status),
    relays: {}
  };

  var onCount = 0;
  for (var i = 0; i < RELAY_COUNT; i++) {
    var isOn = ((status >> i) & 0x01) === 1;
    data.relays["relay_" + (i + 1)] = isOn ? "on" : "off";
    if (isOn) {
      onCount++;
    }
  }

  data.relays_on = onCount;
  data.all_on = onCount === RELAY_COUNT;
  data.all_off = onCount === 0;

  return {
    data: data,
    warnings: warnings,
    errors: errors
  };
}

// Export for local testing (Node.js). TTN calls decodeUplink directly and
// ignores this block.
if (typeof module !== "undefined" && module.exports) {
  module.exports = { decodeUplink: decodeUplink };
}
