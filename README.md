# PS2Game

Minimal PlayStation 2 homebrew survival shooter built in C with:

- PS2SDK
- gsKit
- dmaKit
- audsrv

The project builds `ps2game.elf` and can also be packaged as a PS2 ISO image.

## Current game

The current build contains:

- title screen
- `PLAY` button prompt
- player movement with D-pad
- shooting with `CROSS`
- enemy spawning and chase logic
- level progression
- game over state
- background music playback during `STATE_PLAYING`

Rendering is rectangle-based for stability in PCSX2 and on PS2.

## Project layout

Important files:

- [Makefile](./Makefile)
- [SYSTEM.CNF](./SYSTEM.CNF)
- [src/main.c](./src/main.c)
- [src/game.c](./src/game.c)
- [src/input.c](./src/input.c)
- [src/render.c](./src/render.c)
- [src/audio.c](./src/audio.c)

Assets used by the ISO build:

- `assets/pixel.wav`
- `assets/audsrv.irx`

## Requirements

This project is built inside the Docker image:

- `ps2dev/ps2dev:latest`

The project does not rely on a local PS2SDK install on Windows. Everything important is built in the container.

## Docker workflow

Open PowerShell in the project root and start the PS2DEV container:

```powershell
docker run --rm -it -v ${PWD}:/src ps2dev/ps2dev:latest sh
```

Inside the container, export the PS2 toolchain environment:

```sh
export PS2DEV=/usr/local/ps2dev
export PS2SDK=$PS2DEV/ps2sdk
export PATH=$PATH:$PS2DEV/bin:$PS2DEV/ee/bin:$PS2DEV/iop/bin:$PS2SDK/bin
```

Install the tools used by this repo:

```sh
apk add --no-cache make xorriso
```

`make` is required for building the ELF.

`xorriso` is required for building the ISO.

## Build the ELF

From inside the container:

```sh
cd /src
make clean
make
```

Output:

- `ps2game.elf`

## Build the ISO

From inside the container:

```sh
cd /src
make clean
make
make iso
```

Output:

- `ps2game.iso`

What `make iso` does:

1. Builds `ps2game.elf`
2. Creates an ISO staging directory named `iso_root`
3. Copies these files into the disc image:
   - `SYSTEM.CNF`
   - `PS2GAME.ELF`
   - `PIXEL.WAV`
   - `AUDSRV.IRX`
4. Runs `xorriso -as mkisofs` to generate `ps2game.iso`

## Why ISO mode is used

Earlier builds used `host:` paths for loading runtime files such as:

- `pixel.wav`
- `audsrv.irx`

That approach can fail depending on how PCSX2 launches the ELF and whether `host:` is available.

The project is now prepared for ISO-based execution instead:

- music file is loaded from `cdrom0:\PIXEL.WAV;1`
- `audsrv.irx` is loaded from `cdrom0:\AUDSRV.IRX;1`
- `SYSTEM.CNF` boots `cdrom0:\PS2GAME.ELF;1`

This is closer to how a real PS2 disc behaves and is more reliable for packaged builds.

## Run in PCSX2

### ELF mode

The audio system is now configured for ISO/disc paths, so plain `Run ELF` is no longer the preferred way to test the game.

You can still use it for quick graphics testing, but disc-based audio loading will not work correctly there.

### ISO mode

Recommended method:

1. Open PCSX2
2. Choose the option to boot/open an ISO
3. Select `ps2game.iso`
4. Start the game

Expected result:

- the game boots through `SYSTEM.CNF`
- the ELF starts from the disc image
- `pixel.wav` is read from the ISO
- audio should play only while the game is in `STATE_PLAYING`

## Audio behavior

The audio code is intentionally simple:

- `audio_init()` loads `LIBSD`, then `AUDSRV.IRX`
- `pixel.wav` is opened from `cdrom0:`
- WAV format is read from the file header
- `audio_update()` streams small chunks every frame
- audio is enabled only when `game.state == STATE_PLAYING`
- leaving gameplay stops audio and rewinds the file

## Notes for Git

Before pushing:

```sh
make clean
```

This avoids committing generated object files or stale build outputs unless you intentionally want them in the repository.

Files that matter most to commit:

- source files under `src/`
- `Makefile`
- `SYSTEM.CNF`
- `README.md`
- runtime assets required by ISO mode:
  - `assets/pixel.wav`
  - `assets/audsrv.irx`

Generated outputs that usually should not be committed:

- `ps2game.elf`
- `ps2game.iso`
- `iso_root/`
- `src/*.o`

## Typical full session

From Windows PowerShell:

```powershell
docker run --rm -it -v ${PWD}:/src ps2dev/ps2dev:latest sh
```

Inside the container:

```sh
export PS2DEV=/usr/local/ps2dev
export PS2SDK=$PS2DEV/ps2sdk
export PATH=$PATH:$PS2DEV/bin:$PS2DEV/ee/bin:$PS2DEV/iop/bin:$PS2SDK/bin
apk add --no-cache make xorriso
cd /src
make clean
make
make iso
```

After that:

- test `ps2game.iso` in PCSX2
- if it works, commit and push to Git
