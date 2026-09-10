# Piux Kernel v0.6.5 BETA

Run `fast-all.sh` for the optional interactive TUI used to build and run Piux.

Piux is a small 32-bit x86 experimental operating system and kernel environment bootable with GRUB Multiboot1. It is built using NASM, freestanding C, GNU `ld`, and a custom linker script.

## Features

* **Multiboot1 compatible** — Boots with GRUB
* **i386 32-bit architecture** — x86 protected-mode kernel
* **Framebuffer graphics** — Native 1280×720 graphics with 32-bit color support
* **Software rendering** — Text and UI are rendered directly into the framebuffer
* **Software double buffering** — Reduces visible flickering and tearing during pWM redraws
* **Bitmap font rendering** — Uppercase/lowercase letters, numbers, punctuation, and printable ASCII characters
* **PS/2 keyboard input** — QWERTY keyboard support with Shift modifiers
* **PS/2 mouse driver** — Pointer input, terminal focus, and wheel scrolling
* **Shell interface** — Command-based kernel interaction
* **Built-in commands** — Commands are compiled into the kernel from `bin/`
* **ATA PIO disk driver** — Primary IDE channel access
* **Ext2 filesystem** — Files, directories, paths, reading, creation, and writing
* **Persistent file editing** — `nano` can create and write files directly to ext2
* **First-boot installer TUI** — Creates the initial Unix-style filesystem tree and account configuration
* **User authentication** — SHA-256 password hashes, login, and sudoer checks
* **pWM text window manager** — Tiled terminal environment with keyboard and mouse focus
* **Resource monitor** — `top` reports RAM and ext2 storage usage
* **RAMFS fallback** — Provides basic filesystem functionality when no ext2 disk is available
* **Configuration files** — System metadata and pWM information are embedded from `etc/`

## Minimum Requirements

These are the current minimum target requirements for Piux:

* **RAM:** 16 MiB minimum
* **Disk:** 8 MiB minimum for a bootable installation and filesystem
* **CPU:** 32-bit x86 / i386-compatible processor
* **Boot:** BIOS/legacy boot with GRUB Multiboot1 support
* **Video:** Multiboot framebuffer capable of providing a supported graphics mode

The default QEMU configuration uses more resources than the minimum.

> Minimum requirements are intended as the project's supported target. Actual compatibility should be verified through testing on increasingly constrained QEMU configurations and real hardware.

## Prerequisites

### On Debian/Ubuntu

```bash
sudo apt-get install nasm gcc binutils grub-pc-bin xorriso qemu-system-x86 e2fsprogs
```

### On Fedora/RHEL

```bash
sudo dnf install nasm gcc binutils grub2-tools xorriso qemu-system-x86 e2fsprogs
```

### On macOS (via Homebrew)

```bash
brew install nasm gcc binutils grub xorriso qemu e2fsprogs
```

## Building

Clone and enter the repository:

```bash
git clone https://github.com/madebyanto/piux-kernel.git
cd piux-kernel-main
```

Build the ISO:

```bash
make clean
make
```

This generates `piux.iso`, a bootable GRUB ISO.

## Running

### With QEMU

```bash
make run
```

QEMU is configured to provide Piux with a framebuffer and an ext2 disk image.

The kernel can also operate using its RAMFS fallback when no usable ext2 disk is available.

To run the ISO without attaching an ext2 disk:

```bash
qemu-system-i386 \
  -cdrom piux.iso \
  -m 512M \
  -vga none \
    -device VGA,xres=1280,yres=720
```

### With the Interactive Build Script

```bash
./fast-all.sh
```

`fast-all.sh` provides an interactive TUI with options to:

1. Compile Piux from scratch
2. Boot an existing build
3. Create an ext2 disk image, build Piux, and boot it with QEMU
4. Exit

## Real Hardware

Piux can also be booted on compatible real x86 hardware.

Write the ISO to a USB device:

```bash
sudo dd if=piux.iso of=/dev/sdX bs=4M status=progress && sync
```

Replace `/dev/sdX` with the correct USB device.

**Warning:** `dd` will erase the selected device.

Then boot from the USB device through the system firmware/BIOS boot menu.

## Usage

After booting and completing the first-boot setup when required, Piux authenticates the user and starts the pWM environment.

A minimal shell is also available when pWM is stopped.

Example prompt:

```text
Welcome to Piux!

user@piux>
```

Use:

```text
help
```

to display the available commands.

## Available Commands

* **help** — Display available commands
* **about** — Show Piux version and system information
* **ls** — List files and directories
* **cd** — Change the current directory
* **cat** — Read a file
* **touch** — Create an empty file
* **mkdir** — Create a directory
* **nano** — Edit and persist files on ext2
* **echo** — Print text, with quoted argument support
* **fs** — Inspect and mount the ext2 filesystem
* **clear** — Clear the framebuffer display
* **top** — Show RAM and ext2 disk usage
* **pwm / pmw** — Start, stop, inspect, or reload pWM
* **reboot** — Reboot Piux
* **shutdown** — Shut down Piux

Command handlers are compiled into the kernel from `bin/`.

## Keyboard Support

Piux currently supports:

* QWERTY keyboard layout
* Shift modifiers
* Uppercase letters
* Common keyboard symbols
* Backspace
* Enter
* pWM keyboard shortcuts

AltGr and additional keyboard layouts are not currently implemented.

## Filesystem

Piux provides an ext2 filesystem implementation through the ATA primary IDE channel.

Current ext2 functionality includes:

* ext2 superblocks and group descriptors
* Variable-size inodes
* Direct data blocks
* Single-indirect data blocks
* Double-indirect data blocks
* Variable-length directory entries using `rec_len`
* Inode bitmap allocation
* Block bitmap allocation
* Directory traversal
* File creation
* Sequential file writing
* File reading
* Persistent file editing through `nano`

The filesystem is intentionally non-journaled.

The current implementation does not yet provide complete support for:

* File deletion
* File rename
* Truncation with block freeing
* Triple-indirect file growth
* Partition table management

When an ext2 disk is unavailable, Piux can fall back to its RAMFS implementation for basic filesystem operations.

## First-Boot Installer

On an ext2 filesystem without the first-boot marker, Piux launches a keyboard-driven installer TUI.

The installer can:

* Create standard Unix directories such as `/bin`, `/etc`, `/home`, `/usr`, and `/var`
* Configure the root device and filesystem
* Store system configuration in `/.config/system.conf` and `/.config/disk.conf`
* Create user home directories
* Store the user list in `/.config/users`
* Configure the initial sudo policy
* Store sudo configuration in `/.config/sudoers` and `/.config/sudo-user`
* Store account records in `/.config/passwd`
* Store passwords as SHA-256 hashes rather than plaintext
* Require authentication before entering the normal user environment

The installer currently records the mounted ext2 device as `/dev/hda`.

Partitioning is not implemented yet because Piux does not currently provide a partition table writer.

The installer runs as a fixed setup sequence and reboots after installation.

## Authentication and Users

Piux includes a basic authentication system.

Current functionality includes:

* User accounts
* Password authentication
* SHA-256 password hashing
* Login
* Sudoer configuration
* Sudoer checks
* Authenticated username display

The authentication system is still experimental and is not intended to provide the security guarantees of a mature Unix-like operating system.

## pWM

**pWM** is Piux's text-based window manager.

It provides a tiled terminal environment inspired by minimal tiled terminal/window managers.

After login, pWM starts automatically and creates the first terminal.

Each terminal has its own:

* Output
* Input
* Scroll state
* Command history

The layout automatically adapts to the framebuffer dimensions.

Supported layouts include:

```text
1 terminal

┌──────────────────────┐
│                      │
│       Terminal       │
│                      │
└──────────────────────┘


2 terminals

┌───────────┬───────────┐
│           │           │
│ Terminal  │ Terminal  │
│           │           │
└───────────┴───────────┘


3 terminals

┌───────────┬───────────┐
│           │ Terminal  │
│ Terminal  ├───────────┤
│           │ Terminal  │
└───────────┴───────────┘


4 terminals

┌───────────┬───────────┐
│ Terminal  │ Terminal  │
├───────────┼───────────┤
│ Terminal  │ Terminal  │
└───────────┴───────────┘
```

### pWM Controls

* `Ctrl+Q` — Open a new terminal, up to four
* `Ctrl+C` — Close the focused terminal
* `Ctrl+Left` — Focus the previous terminal
* `Ctrl+Right` — Focus the next terminal
* Mouse click — Focus a terminal
* Mouse wheel — Scroll the focused terminal
* Up/Down — Navigate command history

pWM uses a software double buffer for its redraw path.

The same command handlers used by the shell are reused inside pWM.

Stopping pWM returns the system to the minimal kernel shell.

## Video System

Piux uses the framebuffer supplied through the Multiboot information structure.

The current graphics target is:

```text
Resolution: 1280 × 720
Color:      32-bit
Rendering:  Software
```

The video subsystem is implemented in:

```text
kernel/video.c
kernel/video.h
```

Piux includes a software bitmap font used to render text directly into the framebuffer.

The text grid is calculated from the available framebuffer dimensions rather than assuming the old VGA 80×25 layout.

### Double Buffering

pWM uses a software back buffer.

Instead of continuously modifying the visible framebuffer during a redraw:

```text
Application
    ↓
Back buffer
    ↓
Complete frame
    ↓
Framebuffer
    ↓
Display
```

This reduces visible flickering while the interface is being redrawn.

Hardware-accelerated graphics are not currently supported.

## Resource Monitor

The `top` command provides basic system resource information.

Current information includes:

* Total RAM
* Used RAM
* Free RAM
* Total ext2 storage
* Used ext2 storage
* Free ext2 storage

`top` is currently a lightweight system monitor rather than a full process monitor.

Process-level monitoring will become more useful after Piux gains multitasking and process management.

## Architecture

### Boot Flow

1. **BIOS/Firmware** starts the boot process
2. **GRUB** loads the Piux kernel
3. GRUB recognizes the Multiboot1 header in the kernel ELF
4. GRUB provides the Multiboot information structure
5. **BootX** initializes the initial kernel environment
6. The kernel enters `kernel_main()`
7. Piux initializes its core subsystems
8. RAMFS is initialized
9. The ext2 filesystem is detected and mounted when available
10. The first-boot installer is launched when required
11. Authentication is performed
12. pWM starts after successful login

### Main Source Components

```text
boot/
    BootX bootloader/entry code

kernel/
    Core kernel
    ATA disk driver
    ext2 filesystem
    RAMFS
    authentication
    keyboard
    mouse
    video
    system functionality

bin/
    Built-in command handlers

tui/
    Installer
    pWM

etc/
    System and pWM configuration data

docs/
    Project documentation
```

## Memory Layout

The kernel is linked to the 1 MiB region traditionally used by the project:

```text
0x00000000 ┌─────────────────────────┐
           │       Low memory        │
           │   Firmware / reserved   │
0x00100000 ├─────────────────────────┤
           │       Piux kernel       │
           │  .multiboot / .text     │
           │  .rodata / .data / .bss │
           │                         │
           │       Kernel stack      │
           └─────────────────────────┘
```

The exact runtime memory usage depends on the kernel build, framebuffer configuration, filesystem state, and allocated buffers.

## Multiboot Protocol

Piux currently uses **GRUB Multiboot1**.

The Multiboot header uses the standard magic value:

```text
0x1BADB002
```

The bootloader provides the kernel with Multiboot information, including the framebuffer information used by Piux's video subsystem.

## Adding Commands

Commands are discovered automatically from `bin/*.c` by the Makefile, but each command must also be registered in `bin/commands.h`.

Create:

```text
bin/mycommand.c
```

with the common handler signature:

```c
#include <stdint.h>

void cmd_mycommand(const char *param,
                   void (*vga_puts)(const char *),
                   void (*vga_putc)(char)) {
    (void)param;
    (void)vga_putc;
    vga_puts("My command output\n");
}
```

Declare and register it in:

```text
bin/commands.h
```

For example:

```c
extern void cmd_mycommand(const char *,
                          void (*)(const char *),
                          void (*)(char));

{ "mycommand", cmd_mycommand },
```

Then rebuild:

```bash
make clean
make
```

## Configuration

### Kernel Information

System information is stored in:

```text
etc/os-infos
```

The file is embedded into the kernel during the build and displayed by the `about` command.

Example:

```text
Name: Piux
Version: 0.6.5 BETA
Bootloader: GRUB Multiboot1
Architecture: i386
Video: 1280x720x32
Developers: Anto
```

### pWM Information

pWM information is stored in:

```text
etc/pwm-info
```

and documented in:

```text
docs/PWM-COMMANDS.md
```

## Debugging

### With GDB

Start QEMU with the GDB stub:

```bash
qemu-system-i386 \
  -cdrom piux.iso \
  -drive file=build/ext2.img,format=raw,if=ide \
  -s -S
```

Then:

```bash
gdb build/kernel.elf
```

Inside GDB:

```gdb
target remote :1234
b kernel_main
c
```

### Serial Output

Serial logging is not currently implemented.

A future serial subsystem could use COM1:

```text
I/O port: 0x3F8
```

and QEMU's:

```bash
-serial stdio
```

## Troubleshooting

### GRUB Not Found

Ensure GRUB rescue tools are installed.

On Debian/Ubuntu:

```bash
sudo apt-get install grub-pc-bin xorriso
```

### QEMU Not Starting

Check:

```bash
qemu-system-i386 --version
```

If unavailable, install the QEMU x86 system package.

### No Framebuffer

Ensure QEMU is configured to provide a supported Multiboot framebuffer.

For example:

```bash
qemu-system-i386 \
  -cdrom piux.iso \
  -m 512M \
  -vga none \
    -device VGA,xres=1280,yres=720
```

### Keyboard Problems in QEMU

Check that QEMU is exposing a compatible PS/2 keyboard and that the Piux keyboard driver is receiving input.

## Known Limitations

* No FAT filesystem support
* No complete interrupt/exception subsystem
* No multitasking or process management
* No user/kernel process separation
* No general-purpose dynamic memory allocator
* No networking
* No hardware-accelerated graphics
* No complete partitioning subsystem
* No ext2 file deletion
* No ext2 rename
* No ext2 truncate with block freeing
* No triple-indirect ext2 file growth
* Limited keyboard layout support
* No AltGr support
* No shell pipes
* No shell redirection
* No shell variables
* `top` is not yet a process monitor
* pWM terminals are not independent user-space processes

## Future Improvements

Possible future development targets include:

* [ ] Interrupt descriptor table (IDT)
* [ ] Hardware interrupt handling
* [ ] Exception handling
* [ ] Protected-mode paging
* [ ] Physical/virtual memory management
* [ ] `malloc` / `free`
* [ ] User mode
* [ ] System calls
* [ ] Processes
* [ ] Task switching
* [ ] Scheduler
* [ ] IPC
* [ ] Pipes and shell redirection
* [ ] Improved ext2 deletion, rename, and truncate support
* [ ] Triple-indirect ext2 support
* [ ] More keyboard layouts
* [ ] Serial console
* [ ] Improved partition support
* [ ] Networking
* [ ] Loadable modules
* [ ] Graphical pWM
* [ ] Additional framebuffer resolutions
* [ ] DOOM port 💀

## Project Philosophy

Piux is designed as an experimental, lightweight, customizable Unix-like operating system project.

The project prioritizes:

* Small size
* Direct hardware interaction
* Simplicity
* Open development
* Customizability
* Learning by implementation

Piux is not intended to replace mature general-purpose operating systems at its current stage.

## License

GPLv3 License — See `LICENSE` for details.

## Credits

* NASM documentation
* OSDev.org
* GNU GRUB / Multiboot documentation
* The broader open-source operating-system development community

## Contributing

Contributions are welcome.

1. Fork the repository
2. Create a feature branch:

```bash
git checkout -b feature/amazing-feature
```

3. Commit your changes:

```bash
git commit -am "Add amazing feature"
```

4. Push the branch:

```bash
git push origin feature/amazing-feature
```

5. Open a Pull Request

When contributing kernel code, please keep hardware-specific functionality separated into appropriate modules where possible.

## Support

For issues, questions, or suggestions:

* Open an Issue on GitHub
* Check existing Issues
* Consult the OSDev wiki
* Include relevant build output, QEMU configuration, and hardware information when reporting bugs
