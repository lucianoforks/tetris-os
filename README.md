This is a fork of TETRIS-OS originally by jdh

##### Current improvements
- Dynamically disabled SB16 so same binary works whether you have an SB16 or not.
- CHS reads as backup in case LBA reads fail
- Simple PC Speaker music playback in case SB16 is not found. Tested with QEMU and real hardware.
##### Planned
- High quality PWM-based speaker sound
##### Wishful thinking
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
Tested on real hardware as well as QEMU.

#### Mac OS
For the cross-compiler: `$ brew tap nativeos/i386-elf-toolchain && brew install i386-elf-binutils i386-elf-gcc`
```
$ make bin
$ qemu-system-i386 -drive format=raw,file=boot.bin -d cpu_reset -monitor stdio -device sb16 -audiodev coreaudio,id=coreaudio,out.frequency=48000,out.channels=2,out.format=s32
```

#### Unix-like
You should not need a cross-compiler in *most* cases as the `gcc` shipped in most linux distros will support `i386` targets.

[If this isn't the case for you, read here about getting a cross-compiler.](https://wiki.osdev.org/GCC_Cross-Compiler)

To run:
```
$ make bin
$ qemu-system-i386 -drive format=raw,file=boot.bin -d cpu_reset -monitor stdio -device sb16 -audiodev pulseaudio,id=pulseaudio,out.frequency=48000,out.channels=2,out.format=s32
```

If you have sound device issues:
- To disable music entirely, try building without the `#define ENABLE_MUSIC` in `main.c` and running with  
`$ qemu-system-i386 -drive format=raw,file=boot.bin`.
- Try using the SDL backend for QEMU:  
`$ qemu-system-i386 -display sdl -drive format=raw,file=boot.bin -d cpu_reset -monitor stdio -audiodev sdl,id=sdl,out.frequency=48000,out.channels=2,out.format=s32 -device sb16,audiodev=sdl`

If you're having issues with no image showing up/QEMU freezing, this is a known bug with QEMU SB16 emulation under GTK. [Please read what @takaswie has written in #2 for a workaround](https://github.com/jdah/tetris-os/issues/2#issuecomment-824773889).

#### Windows
##### Running
Grab the image file from the releases, and run it with qemu with command `qemu-system-i386 -display sdl -drive format=raw,file=boot.bin -audiodev id=dsound,driver=dsound -device sb16,audiodev=dsound`. This combats the GTK rendering bug with SB16 enabled. If your sound is choppy, try to fiddle with the audio settings, for instance running with `qemu-system-i386 -display sdl -drive format=raw,file=boot.bin -audiodev id=dsound,driver=dsound,out.fixed-settings=on,out.frequency=22050,out.buffer-length=80000,timer-period=100 -device sb16,audiodev=dsound` to increase the audio buffer and set a different timer period for updating the audio. Lowering the sample quality from 44 kHz to 22kHz helps combat choppy audio and buffer underruns with not that much of an audio quality hit.

##### Compiling
Depending on your preference, you could setup a linux-like dev-env with WSL and follow the Linux instructions, or you can try the MINGW64 GCC environment for Windows. If you go the MINGW64 route, your best bet is to install MSYS2 and it should come packaged with that.

###### Instructions for MINGW64  
Grab and install [MSYS2](https://www.msys2.org/)  
Grab and install [i386-elf-toolchain](https://github.com/nativeos/i386-elf-toolchain/releases)    
* You need to extract the pre-compiled binary archive of both the GCC and Binutils releases (Windows x86_64 or i686) to your mingw64 folder. Mine was at C:\msys64\mingw64
* This is for cross-compiling the files to elf binary format, helps a lot to not need to battle with PE format binaries. 

If everything was installed correctly, you should be able to open MSYS2 MINGW64 terminal and from there just do a 
```
>make bin
>qemu-system-i386 boot.bin -display sdl -audiodev id=dsound,driver=dsound -device sb16,audiodev=dsound
```

#### Real hardware
You probably know what you're doing if you're going to try this. Just burn `boot.bin` onto some bootable media and give it a go. SB16 is dynamically disabled in case it's not found or it's reset procedure fails, but if things break, try disabling all of the music (remove `#define ENABLE_MUSIC` in `main.c`) since you *probably* don't have something with a SB16 in it.
