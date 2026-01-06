# BLE Command Protocol

Transport: BLE Write to RX characteristic.

Encoding: ASCII text, integers separated by spaces.

## Frame rules
- One BLE write is treated as one command.
- The sketch expects at least one integer (command id).
- Command id is `0..99`.
- Out-of-range or malformed commands are ignored.

## Commands

### 0 - OFF
`0`

### 1 - RAINBOW_SCROLL
`1`

### 3 - RANGE_RGB
`3 start end r g b`

- Range is inclusive.
- If `start > end` the sketch swaps them.
- Range is clamped to `[0..LED_COUNT-1]`.

### 4 - STRIP_RGB
`4 r g b`

### 5 - BRIGHTNESS
`5 value`

`value` should be `0..255` (uint8).
