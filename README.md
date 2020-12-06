# SOS (Shitty OS) SOS, often stylized $0$, S0S, sos, SoS, and trashOS, is an
operating system built 100% from scratch without relying on anything at all,
except a compiler, and currently grub (but grub will be going away soon).

## Project status
Currently active development, exclusively as a hobby / learning exercise.
 - [x] Virtual & Physical memory management
 - [x] Memory allocation and freeing
 - [x] Keyboard handler and small "shell"
 - [x] Virtual Filesystem
 - [ ] Support for a real filesystem (FAT32)
 - [ ] A real shell (CLIT = Command Line Interface Tool)
 - [ ] Context switching environment / support for processes
 - [ ] Internet

## Building requirements
 - A 64 bit version of GCC, preferably the latest one. I can't guarantee that this works on any version of GCC prior to 6.3.1. This means it probably won't build on whatever older version of ubuntu you're using. If you're curious about a cross compiler, read the last section.
 - NASM 64 bit. No guarantee that this will work on a version older than 2.12.02. Again, it probably won't build if you dont live in at least 2017. 

## Building instructions
| Make Target   | Description  |
| ------------- | ------------ |
| make qemu     | Builds the whole OS and launches it in qemu  |
| make bochs    | Builds the whole OS and launches it in bochs |
| make debugq   | Builds the whole OS and launches it in qemu with debug flags |

## Contributing
Since this is here for me to learn, big features are only encouraged if you do a really clean job of it, and write nice fancy commit messages. small fixes are more than welcome. Don't use tabs in source files, and generally follow the same style guides used throughout the rest of the code.

## Cross Compiler?
When you normally compile something, your compiler *should* be configured to create binaries for only the architecture of your machine. Any time you have a compiler that generates machine code for some other architecture, that would be a cross compiler. Most operating systems use a cross compiler, and that's what OSDevWiki says to do too. However, we do not live in 2000 any more, and SOS is exclusively built on, and developed for the AMD64 architecture (x86_64). This means that there is no need to waste time on a cross compiler.

## License
SOS is licensed under the GPLv2. See LICENSE for details.
