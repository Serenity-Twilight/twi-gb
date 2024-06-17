# twi-gb
This is my personal design and implementation of an emulator for the Nintendo
Game Boy portable system.

## Features & limitations
**NOTE:** This emulator is **NOT** feature complete, and is being developed
primarily for my own entertainment and practice as a developer. The intent
is to *eventually* emulate the Game Boy with total accuracy, but I only
work on this project when I enjoy doing so. As such, progress will be slow
and this emulator will is missing many features considered standard and/or
essential for casual use. If you just want to play Game Boy games with
maximum accuracy, search for a different emulator.

Implemented and planned features:
- [x] Near-Full Game Boy CPU interpreter
- [x] Live disassembly of running Game Boy software
- [x] Monochrome PPU rendering emulation
- [x] Input (hard-coded, see Usage section)
- [ ] Dynamic Game Pak parser and loader
- [ ] Configurable inputs
- [ ] Configuration for other user preferences
- [ ] Audio emulation
- [ ] Expansion of user preferences
- [ ] GUI
- [ ] Implement STOP instruction on CPU
  (not used by any commercial games, but might as well)

## Building
**NOTE:** I've developed and built this within an Arch Linux environment,
and has not been tested in other operating environments. It's very, very
likely that it's implicitly dependent on other software that I have yet
to formally identify.

### Dependencies:
- GNU make (to build)
- GCC (to compile)
- SDL2 (graphics+input)
- A terminal environment with access to `mkdir` and `rm`,
  behaviorally-identical to GNU variants

### How to build:
1. Clone the repository
2. Navigate to the repository's base directory (wherever you cloned it to)
3. Run `make`

This will build `dgb` in the current directory, which is a debug build of
the emulator. Release-version building is not currently available.

## Usage:
`dgb <ROM-filepath>`
At this time, this emulator doesn't support ROM or external RAM bank swapping,
so the emulation will only behave correctly if a 32KiB ROM with up to 8KiB
of external RAM is used. Data is not saved.

All testing has been done with a dump of the Game Boy version of Tetris,
which meets the above requirements.

Controls are currently hard-coded as:
- Arrow keys -> Directional pad
- Z key -> A button
- X key -> B button
- Enter/Return key -> Start button
- Right shift key -> Select button

