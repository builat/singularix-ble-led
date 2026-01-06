# Arduino BLE WS2811/NeoPixel Controller

Control a WS2811/WS2812 LED strip using BLE commands (ASCII text) sent from a client (e.g., a Rust service).

- Board: Arduino UNO R4 (or compatible with ArduinoBLE)
- LEDs: WS2811 / WS2812 ("NeoPixel-compatible")
- Library: Adafruit NeoPixel
- Transport: BLE Write characteristic (`rxChar`)

## Features

- Non-blocking moving rainbow animation (uses `millis()`)
- Commands are numeric `0..99`
- Set full strip color (RGB)
- Set a range of LEDs to an RGB color
- Set brightness

## Wiring

### LED strip (WS2811/WS2812)
- **DIN** -> Arduino `LED_PIN` (default: D6)
- **GND** -> Arduino GND
- **+5V** -> 5V power supply (recommended for long strips)

> Important: Use a suitable external 5V supply for 272 LEDs. Always connect **grounds together** (Arduino GND and LED PSU GND).

## BLE
### All of the next valus could or should be changed on you local setup.
Device name: `ArduinoBLE`

Service UUID:
- `f49842a0-def2-4ed6-8636-f48a4332b275` 

RX characteristic (write commands):
- `f4cbb481-052a-4080-8081-2e3f1437b3f3`

TX characteristic (notify/read; reserved):
- `6faa3298-1a04-4df0-9348-99492707d883`

## Command Protocol (ASCII)

One BLE write = one command string (no need for newline).

General format:

`cmd_id` must be `0..99`.

### Commands

#### 0 - Off
```0```
#### 1 - Moving rainbow
```1```
#### 3 - Set range to RGB

Inclusive range `[start..end]`:

```3 <start> <end> <r> <g> <b>```

Example:
```3 10 20 128 128 128```


#### 4 - Set entire strip to RGB
```4 <r> <g> <b>```

Example:
```4 255 0 0```

#### 5 - Set brightness
Brightness is `0..255`.

```5 <brightness>```

Example:

```5 128```

## Libraries

This project uses the following Arduino libraries:

### ArduinoBLE
- Purpose: BLE peripheral mode (advertising, services, characteristics).
- Used for:
  - `BLE.begin()`, `BLE.advertise()`
  - `BLEService`, `BLECharacteristic`
  - Receiving command strings via a write characteristic (`rxChar`)

Install:
- Arduino IDE → **Library Manager** → search **ArduinoBLE** → install

### Adafruit NeoPixel
- Purpose: Control WS2811/WS2812 LED strips (NeoPixel-compatible).
- Used for:
  - Setting pixel colors, brightness
  - `ColorHSV()` + `gamma32()` for smooth rainbow animation

Install:
- Arduino IDE → **Library Manager** → search **Adafruit NeoPixel** → install

## Notes / Troubleshooting

### Colors look wrong
In my code strip is configured as `NEO_BRG`. If colors are swapped, try:
- `NEO_GRB`
- `NEO_RGB`

### Rainbow not smooth
Tune in `src/main.ino`:
- `frameMs` (lower = smoother)
- `rainbowSpeed` (higher = faster movement)

### Power
274 LEDs can draw significant current (worst case at white). Use an appropriate power supply and add a large capacitor near the strip input if needed.

## License
MIT (see LICENSE)
