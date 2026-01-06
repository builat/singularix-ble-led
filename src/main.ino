#include <ArduinoBLE.h>
#include <Adafruit_NeoPixel.h>

/**
 * WS2811 strip wiring
 *  - LED data pin -> LED_PIN
 *  - LED powerd directly from LED power supply unit.
 *  - Common GND between Arduino and LED power supply.
 */
#define LED_PIN 6
#define LED_COUNT 274

/**
 * Color order note:
 * NEO_BRG - config tested for my particular led strip.
 * Many strips are GRB or RGB; keep this as you tested.
 * If colors look swapped, try NEO_GRB / NEO_RGB.
 */
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_BRG + NEO_KHZ800);

// Global feature flags
bool rainbowEnabled = false;

/**
 * Non-blocking animation timing.
 * frameMs controls "FPS", rainbowSpeed controls movement speed.
 */
unsigned long lastUpdate = 0;
const uint16_t frameMs = 45;        // ms per frame (~22 FPS)
const uint16_t rainbowSpeed = 180;  // hue offset delta per frame
uint16_t rainbowOffset = 0;

/**
 * BLE UUIDs.
 * BLE_ID = Service UUID
 * BLE_RX = Write characteristic (client -> Arduino)
 * BLE_TX = Notify characteristic (Arduino -> client), not used yet but ready for ACK/telemetry
 * BLE_NM = Name of device in bluetooth discovery
 */
static const char BLE_ID[] = "f49842a0-def2-4ed6-8636-f48a4332b275";
static const char BLE_RX[] = "f4cbb481-052a-4080-8081-2e3f1437b3f3";
static const char BLE_TX[] = "6faa3298-1a04-4df0-9348-99492707d883";
static const char BLE_NM[] = "ArduinoBLE";

BLEService service(BLE_ID);

// RX characteristic: accepts commands (ASCII text) up to 128 bytes
BLECharacteristic rxChar(
  BLE_RX,
  BLEWrite | BLEWriteWithoutResponse | BLERead, 128
);

// TX characteristic: reserved for future ACK/notifications
BLECharacteristic txChar(
  BLE_TX,
  BLENotify | BLERead, 128
);

void setup() {
  // Start BLE. If it fails, stop here.
  if (!BLE.begin()) {
    for (;;);
  }

  // Advertise with a stable device name so clients can find it
  BLE.setDeviceName(BLE_NM);
  BLE.setLocalName(BLE_NM);

  // Publish service and its characteristics
  BLE.setAdvertisedService(service);
  service.addCharacteristic(rxChar);
  service.addCharacteristic(txChar);
  BLE.addService(service);
  BLE.advertise();

  // Initialize LED strip
  strip.begin();
  strip.setBrightness(80);
  strip.show(); // ensure strip starts off
}

/**
 * Main loop: non-blocking BLE + animation
 * - No delay() is important here to change color in lifetime without delay
 */
void loop() {
  BLEDevice central = BLE.central();

  // No central device discovered
  if (!central) return;

  // Central exists but not connected
  if (!central.connected()) return;

  // Run "cooperative multitasking"
  processRainbowNonBlocking();
  processBleCommands();
}

/**
 * Set entire strip to a single RGB color.
 */
void setStripColor(uint8_t r, uint8_t g, uint8_t b) {
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(r, g, b));
  }
  strip.show();
}

/**
 * Turn all LEDs off.
 * Same could be done with set RGB to 0 0 0.
 * This function is just shortcut.
 */
void ledsOff() {
  setStripColor(0, 0, 0);
}

/**
 * Non-blocking moving rainbow ("scroll") animation.
 * Uses millis() timing, so it doesn't block BLE.
 */
void processRainbowNonBlocking() {
  if (!rainbowEnabled) return;

  unsigned long now = millis();
  if (now - lastUpdate < frameMs) return;
  lastUpdate = now;

  // Paint gradient + movement offset
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(
      i,
      strip.gamma32(
        strip.ColorHSV(
          rainbowOffset + (uint32_t)i * 65535 / strip.numPixels()
        )
      )
    );
  }

  strip.show();
  rainbowOffset += rainbowSpeed;
}

/**
 * Read BLE commands from rxChar.
 * Commands are ASCII strings, e.g.:
 *  <CMD-NUM> ARG0..ARG6
 *   "0" -> off
 *   "1" -> rainbow
 *   "3 10 20 128 128 128" -> set range 
 *        Set led in range from 10 to 20 with RGB colors (128, 128, 128)
 *        Order of range is not so important, coz it will be fixed in handler function.
 *   "4 255 0 0" -> set strip to specific RGB color, or switch off if RGB will be (0, 0, 0)
 */
void processBleCommands() {
  if (!rxChar.written()) return;

  uint8_t buf[128];
  int len = rxChar.valueLength();

  if (len > 127) len = 127;
  rxChar.readValue(buf, len);

  // Ensure we have a null-terminated string
  buf[len] = '\0';

  processCommand((char *)buf);
}

/**
 * Set a specific range [startLed..endLed] to an RGB color (inclusive).
 * - Swaps start/end if reversed
 * - Clamps indices to strip bounds
 */
void setRangeColor(int startLed, int endLed, uint8_t r, uint8_t g, uint8_t b) {
  // Normalize order
  if (startLed > endLed) {
    int tmp = startLed;
    startLed = endLed;
    endLed = tmp;
  }

  // Clamp
  if (startLed < 0) startLed = 0;
  if (endLed >= strip.numPixels()) endLed = strip.numPixels() - 1;

  uint32_t color = strip.Color(r, g, b);

  for (int i = startLed; i <= endLed; i++) {
    strip.setPixelColor(i, color);
  }
  strip.show();
}

/**
 * Command router.
 * Format: <cmd> [args...]
 * cmd must be 0..99.
 *
 * Parsed integers:
 *   cmd a b c d f
 *
 * Commands implemented:
 *  0                -> off
 *  1                -> enable moving rainbow
 *  3 start end r g b-> set range color
 *  4 r g b          -> set whole strip color
 *  5 brightness     -> set brightness (0..255)
 *  99               -> reserved
 */
void processCommand(String line) {
  int cmd = -1;

  // Generic parsed slots (reused by commands)
  int a = 0, b = 0, c = 0, d = 0, f = 0;

  int count = sscanf(line.c_str(), "%d %d %d %d %d %d", &cmd, &a, &b, &c, &d, &f);
  if (count < 1 || cmd < 0 || cmd > 99) return;

  // Default behavior: any command cancels rainbow unless command explicitly enables it
  rainbowEnabled = false;

  switch (cmd) {
    case 0: // OFF
      ledsOff();
      break;

    case 1: // RAINBOW (moving)
      rainbowEnabled = true;
      break;

    case 2: // reserved
      break;

    case 3: { // RANGE COLOR: 3 start end r g b
      int startLed, endLed, r, g, b2;
      if (sscanf(line.c_str(), "%d %d %d %d %d %d", &cmd, &startLed, &endLed, &r, &g, &b2) == 6) {
        setRangeColor(startLed, endLed, (uint8_t)r, (uint8_t)g, (uint8_t)b2);
      }
      break;
    }

    case 4: // WHOLE STRIP COLOR: 4 r g b
      if (count < 4) break;
      setStripColor((uint8_t)a, (uint8_t)b, (uint8_t)c);
      break;

    case 5: // BRIGHTNESS: 5 value
      // brightness is 0..255; you may clamp if desired
      strip.setBrightness((uint8_t)a);
      strip.show();
      break;

    case 99: // reserved
      break;

    default:
      break;
  }
}