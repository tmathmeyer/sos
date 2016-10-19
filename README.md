# sos, the shitty operating system ...---...

##Building:
sos ONLY targets x86_64. There may be arm support in the future, but there will never be support for 32 bit evil.
because sos only targets one architecture and it is the same architecture that I build on, there is no need for a cross compiler; this of course means that you need to build sos on another x64 machine (or go through xcomp hell). Despite OSDevWiki's insistence that kernels _NEED_ a cross compiler, that is total bullshit; as long as no system libraries are included or linked, there is no problem. If you've degenerated to the point of writing hobby kernels, you might as well just drink all the coolaid and write every single line of code yourself.

Right. Back to building instructions. There are three ways to build / test sos. 
 - Hardware: run ```make iso``` then plug in a flash drive and then run ```sudo dd bs=4M if=build/os.iso of=/dev/sdX status=progress && sync```, then reboot your computer and select your flash drive in the boot menu. do NOT flash this onto your hard drive
 - qemu: If you want to just play around with sos, run ```make```. this will altomatically build sos and launch qemu with 512 megs of ram. if you want to get a trace from qemu if youre getting crashes, run ```make debug```.
 - bochs: If you _actually_ want to debug things like an adult, run ```make kernel``` then run ```bochs -q```. Also without VERY good reason, I will not accept any pull requests to bochsrc.txt.
 
##Contributing:
Contributions, especially bug fixes, are welcome. Large features that implement significant chunks are also welcome, but I probably wont accept them unless you talk to me about it before hand. The point of this kernel is for my own edification; if other people write a bunch of mystery code then I need to go through it and read it, and I do not have that kind of time usually since I am at work. Like I mentioned above in the building instructions, I do have a few rules for contributing:
 - all contributions are GPL2
 - do not use tabs
 - do not use if/for/while/else, etc expressions without curly braces. there will be no goto fail; here, only much worse bugs.
 - comments are nice, but not super necessary. I might ask for some though.
 - all curly braces for if/for/while/else/functions are on the same line
 - do not use tabs
 - do not use tabs
 - do not change the bochsrc.txt file unless you have a very good reason

 ##TODO list:
 - better page allocation
     - use the AVL system for determining best fit rather than first fit
 - filesystem
 - text editor
 - compiler
