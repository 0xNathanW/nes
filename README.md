# NES Emulator

An NES emulator, in C, with support for mappers 1, 2 and 4.

<img src="assets/demo.gif" width="400">

## Install

Clone the respository and build the binary with Make.

## Usage

```bash
usage: ./nes [options] <rom.nes>

options:
  -s, --scale <n>   Window scale factor (default: 3)
  -o, --overscan    Crop 8 pixels from each edge (mimics original displays and hides PPUMASK left column clipping)
  -c, --speed <x>   Clock speed multiplier (default: 1.0)
  -h, --help        Show this help message
```

## Controls

| Input       | Key         |
| ----------- | ----------- |
| Up/Down/Left/Right   | Arrow keys  |
| A   | Z        |
| B   | X        |
| Start | Enter  |
| Select | Space |
