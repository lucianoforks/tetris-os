This is a fork of TETRIS-OS originally by jdh

##### Current improvements (these have already been merged to upstream)
- Dynamically disabled SB16 so same binary works whether you have an SB16 or not.
- CHS reads as backup in case LBA reads fail
- Simple PC Speaker music playback in case SB16 is not found. Tested with QEMU and real hardware.
##### Planned
- Fixing SB16 compatibility
- High quality PWM-based speaker sound
##### Wishful thinking
- Refactor sound API to make writing different sound drivers possible
- AC97 or HD-audio, depending on what is onboard my old computer. Requires PCI setup among other things, so not the easiest thing to combat.

# TETRIS-OS: An operating system that only plays Tetris.

![screenshot](images/0.png)

[Video with an explanation of the development process.](https://www.youtube.com/watch?v=FaILnmUYS_U)

#### Features:
- It's Tetris.
- 32-bit (x86)
- Fully custom bootloader
- Soundblaster 16 driver
- Custom music track runner
- Fully hardcoded tetris theme
- Double-buffered 60 FPS graphics at 320x200 pixels with custom 8-bit RGB palette

#### Resources Used
- [osdev.org wiki](https://wiki.osdev.org/Main_Page)
- [Sortix](https://sortix.org)
- [ToaruOS](https://toaruos.org)
- [James Molloy's Kernel Development Tutorials](http://www.jamesmolloy.co.uk/tutorial_html/)

### Building & Running
<<<<<<< HEAD
Tested on real hardware as well as QEMU.

#### Mac OS
For the cross-compiler: `$ brew tap nativeos/i386-elf-toolchain && brew install i386-elf-binutils i386-elf-gcc`
```
$ make bin
$ qemu-system-i386 -drive format=raw,file=boot.bin -d cpu_reset -monitor stdio -device sb16 -audiodev coreaudio,id=coreaudio,out.frequency=48000,out.channels=2,out.format=s32
```
=======
~~**NOTE**: This has *only* been tested in an emulator. Real hardware might not like it.~~

 EDIT: this is not true anymore! [@parkerlreed has run this on a Thinkpad T510](https://github.com/jdah/tetris-os/issues/5#issuecomment-824507979).

#### Mac OS
For the cross-compiler: `$ brew tap nativeos/i386-elf-toolchain && brew install i386-elf-binutils i386-elf-gcc`

To run use `$ make qemu-mac`
>>>>>>> upstream/master

#### Unix-like
You should not need a cross-compiler in *most* cases as the `gcc` shipped in most linux distros will support `i386` targets.

[If this isn't the case for you, read here about getting a cross-compiler.](https://wiki.osdev.org/GCC_Cross-Compiler)

To run use `$ make qemu-pulse`

If you have sound device issues, try the SDL backend for QEMU with `$ make qemu-sdl` or disable any audio devices with `$make qemu-no-audio`

If you're having issues with no image showing up/QEMU freezing, this is a known bug with QEMU SB16 emulation under GTK. [Please read what @takaswie has written in #2 for a workaround](https://github.com/jdah/tetris-os/issues/2#issuecomment-824773889).

#### Windows

Good luck. Maybe try dual booting with Linux if this doesn't work out :)

- Follow the Unix-like instructions while using WSL 
- Using  [MSYS2](https://www.msys2.org/) and the [i386-elf-toolchain](https://github.com/nativeos/i386-elf-toolchain/releases)
  - Extract the binaries of the GCC and binutils releases to your mingw64 folder (likely at `C:\msys64\mingw64`)
  - `make`, `gcc`, etc. should now be in your `PATH`

```
> make img
> qemu-system-i386 -drive format=raw,file=boot.img -display sdl -audiodev id=dsound,driver=dsound -device sb16,audiodev=dsound
```

If sound is broken or choppy, try running with  `> qemu-system-i386 -display sdl -drive format=raw,file=boot.img -audiodev id=dsound,driver=dsound,out.fixed-settings=on,out.frequency=22050,out.buffer-length=80000,timer-period=100 -device sb16,audiodev=dsound`

#### Real hardware
You probably know what you're doing if you're going to try this. Just burn `boot.img` onto some bootable media and give it a go. The SB16 is dynamically disabled in case it's not found or it's reset procedure fails, but if things continue to break try removing all references to sound or music first.
