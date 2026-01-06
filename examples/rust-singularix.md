# Rust BLE: Command formatting

This repo uses ASCII commands over BLE. Your Rust service can build strings like:

- `format!("0")`
- `format!("1")`
- `format!("4 {} {} {}", r, g, b)`
- `format!("3 {} {} {} {} {}", start, end, r, g, b)`

Then write the bytes to the RX characteristic UUID:
`f4cbb481-052a-4080-8081-2e3f1437b3f3`

If you want, add an ACK channel via TX characteristic (notify) later.
