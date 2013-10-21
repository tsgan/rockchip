== FreeBSD on Radxa Rock ==

[[http://www.radxa.com|Radxa Rock]] is a developer board that is based on the [[http://http://en.wikipedia.org/wiki/Rockchip|Rockchip RK3188 System-On-Chip (SoC)]].

The full technical details for the Radxa Rock are available at the [[http://radxa.com|Radxa site]]. 

=== Build FreeBSD ===

1. Get FreeBSD head

2. Compile kernel and world. Install world to USB Flash (replace da0 with your usb storage device)
 {{{
 $ truncate -s 1024M radxa.img
 $ sudo mdconfig -f radxa.img -u0
 $ sudo newfs /dev/md0
 $ sudo mount /dev/md0 /mnt

 $ sudo make TARGET_ARCH=armv6 kernel-toolchain
 $ sudo make TARGET_ARCH=armv6 KERNCONF=RADXA buildkernel
 $ sudo make TARGET_ARCH=armv6 buildworld
 $ sudo make TARGET_ARCH=armv6 DESTDIR=/mnt installworld distribution

 $ sudo umount /mnt
 $ sudo mdconfig -d -u0
 $ sudo sysctl kern.geom.debugflags=16
 $ sudo dd if=radxa.img of=/dev/da0 bs=4096k 
 }}}

=== Prepare rkcrc, rkflashtool and flash nand ===

1. Get rkcrc from [[https://github.com/naobsd/rkutils|here]]

2. Get rkflashtool from [[https://github.com/crewrktablets/rkflashtools|here]]

3. Replace Makefile with [[people.freebsd.org/~ganbold/rockchip/Makefile|this]] and rkflashtool.c with [[people.freebsd.org/~ganbold/rockchip/rkflashtool.c|this]].

4. Build rkcrc and rkflashtool (suppose you did git clone in your home directory )
 {{{
# cd ~/rkutils
# gcc rkcrc.c -o rkcrc
# cd ~/rkflashtool
# make
 }}}

5. Make kernel ready to flash
 {{{
# cd ~/rkutils
# ./rkcrc -k /usr/obj/arm.armv6/usr/src/sys/RADXA/kernel kernel.img
 }}}

6. Turn off the board. Then while pushing Recover button plug usb cable(connected to otg port) to PC and release Recover button.

7. Change load address just one time. Get parameters and edit KERNEL_IMG parameter and make it ready to flash and flash it back to nand. Again do this only one time.
 {{{
# cd ~/rkflashtool
# ./rkflashtool p > parm.txt             ---- Edit this file and change KERNEL_IMG address 0x60408000 to 0x60400000 and save it
# cd ~/rkutils
# ./rkcrc -p rkflashtool/parm.txt rkflashtool/parm.bin
# cd ~/rkflashtool
# ./rkflashtool w 0x0 0x2 < parm.bin
 }}}

8. Flash kernel image to nand and turn off the board.
 {{{
# ./rkflashtool w 0x4000 0x5000 < ../rkutils/kernel.img
 }}}

=== Boot ===

1. Connect by using serial console cable and speed 115200
 {{{
 # cu -l /dev/ttyu0 -s 115200
 }}}

2. Power ON your board


