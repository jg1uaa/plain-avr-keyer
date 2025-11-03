# plain AVR keyer

---
## Description

A plain AVR CW keyer

## Requirements

- AVR based Arduino board (non-AVR CPU not supported)
- seeed studio relay shiled
- your favorite paddle

## Connection

As default,

- D15 (PC1): dot input
- D14 (PC0): dash input
- D4 (PD4): key output

Each inputs should be buffered with 10nF capacitor towards GND.

## Usage

Works as fixed-speed keyer.
If you want to change speed, modify `DOT_LEN` value with your favor.

## ToDo

- implement Iambic-A mode
- sidetone

## License

MIT License

## References

消費電流30μA〜送信機のキーイング極性を気にしないで使用できるCMOSエレキー (30uA current consumption - CMOS keyer that supports any transmitter keying polarity) JK1DLQ, CQ ham radio Jun/1980 p.283
