monodrv
=======

Kernel and userland implementation of SIERRA MonoDrv debugging utility, used in EarthSiege 2.

The kernel driver opens a "pipe" which behaves to EarthSiege like the old monochrome GPU driver the developers used and takes the debug messages. It buffers them in kernel space and the userland component retrieves the messages and prints them.

Be warned, this driver is likely full of holes and leaks; it might as well also crash your system. Use a virtual machine, for gods sake. I tried to keep it stable, but I can't promise anything.

# Usage

1. Binaries will/should be on https://sites.google.com/site/es2reveng/downloads for x86 and x64, tested with/built against Win7. It may be possible to run them on Vista or even XP but I would not count on this.
2. Boot with the F8 option "Disable signature enforcement" in 64-bit Windows. Fuck MS.
3. Using srvman (google for it) create a "Device driver" called "monodrv" and the .sys as binary. Important: Start type MUST be manual.
4. Start the driver by running `net start monodrv` in an elevated console.
5. Run monodrv_userland.exe in the same console
6. Run ES.EXE in elevated mode
7. I can has debug message :)

# Compile
## What's needed (in order of install)
* Windows 7
* Visual Studio 2010
* Windows Driver Kit 7.1
* VisualDDK

# Compilation process
1. Open VS2010, load the solution
2. Set debug/release config
3. Compile
4. Output will be in \Release resp. \Debug (monodrv_userland.exe and, if 32bit, monodrv_userland.sys) or \x64\Release resp. \x64\Debug

Good luck.
