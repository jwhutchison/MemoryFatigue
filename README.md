# MemoryFatigue

General purpose memory pattern and patch utility for Linux. Will work with any process that can be
read or modified via `/proc/pid`, but primarily was developed to make memory patching Windows games
running in Linux easier and more consistent. The library has rudimentary support for Elf executables,
but YMMV.

IMPORTANT: This software can be used to read and write memory of active processes. This could be dangerous.
Authors and maintainers of this software are not responsible for any damage you may do to your system or software.
SEE LICENSE

## Fatigue CLI

There is a simple CLI command to make one off memory patches or to dry-run finding processes by various
names. Download from releases and run `fatigue --help` for usage.

IMPORTANT: This CLI tool, in its current form at least, is meant to work with PE processes in Linux memory
(e.g. Windows games running in Wine). It has _rudimentary_, _experimental_ support for Elf processes, but
NO PARTICULAR RESULT IS GUARANTEED. Using this tool with an Elf process will force interactive mode.

When invoking the command you can specify the following:

- Process (must choose one)
  - `-p` or `--pid` - Use this exact process ID
  - `-c` or `--cmdline` - Find PID by whole or partial cmdline (/proc/{pid}/cmdline)
  - `-s` or `--status` - Find PID by Name field of status (/proc/{pid}/status)
  - `-m` or `--map` - If specified find the process map to use by whole or partial name
                      Useful for accessing dll or so files, or if the process name is wildly different
- Address & Pattern
  - `--section` - Memory section to use, usually '.text' or '.data' (defaults to '.text')
                  Note: 'map' or 'header' can be used here _for read only_ to read the process headers
  - `--address` - Use this exact address in the section for operations (overrides pattern)
  - `--pattern` - Search for a given memory pattern in the section (see examples)
  - `--offset` - Offset in bytes from the pattern to apply a patch (only applies with pattern)
- Actions (may choose one)
  - `--read` - Read and display this many bytes
  - `--patch` - Write the specified hex bytes (see warning)
- Flags
  - `-d` or `--dry-run` - Will display information about the address and patch to be applied without actually writing it
  - `-i` or `--interactive` - Will prompt you to continue before searching and patching (gives you an opportunity to abort)
  - `-v` or `--verbose` - Display extra information during patch (default on for read, dry-run, and interactive)
- Operational
  - `-P` or `--ptrace` - Use PTRACE for memory operations (do not use this)
  - `-T` or `--timeout` - Wait for n seconds for the process to start
  - `-D` or `--delay` - Wait for n milliseconds after finding the process before attaching to it (increase to avoid some errors)

Examples:

Find the sekiro exe by status name, locate pattern and apply patch at offset, with verbose output:

```fatigue -s "sekiro.exe" --pattern "C6 86 ?? ?? 00 00 ?? F3 0F 10 8E ?? ?? 00 00" --offset 6 --patch "00" -v```

Get process by pid 523340, print basic information and exit:

```fatigue -p 523340```

Find a process by cmdline portion, find a pattern and show patch information, but do not apply it (dry run):

```fatigue -c "games\\sekiro.exe" --pattern "C6 86 ?? ??0" --offset 6 --patch "00" -d```

Find a process by status name, search for a pattern, show information and exit:

```fatigue -s "sekiro.exe" --pattern "C6 86 ?? ??"```

Find a process by status name, search for a pattern, read 32 bytes from the address:

```fatigue -s "sekiro.exe" --pattern "C6 86 ?? ?? 00 00 ?? F3 0F 10 8E ?? ?? 00 00" --offset 6 --read 32```

Find a process, read 15 bytes from a static address (using hex):

```fatigue -s "sekiro.exe" --address $(( 0x73cece )) --read 16```

Read the process headers directly from the map:

```fatigue -p 17770 --section map --address 0 --read 64```

Read the ELF process headers from a loaded .so in the process:

```fatigue -p 17770 --map "steamoverlayvulkanlayer.so" --section header --address 0 --read 64```


## Sekiro: Shadows Die Twice Game Patcher

Also included in releases is a patcher specifically for patching Sekiro in Linux to unlock FPS and some
other handy features. See [patchers/sekiro/README.md] for details.

## Library

For more complex patch tasks, you will need to build the tool you want. The code is fairly well documented
and simple, and have a look at demo.cpp or patchers/**/main.cpp for more examples.

Note, this is meant for Linux, and for my use case in particular. It may or may not work in other
environments that support the `/proc/pid` interface and PTRACE. I welcome contributions.

Basic library use:

1. Locate process PID, usually by executable name
2. Attach to process
3. Find the process memory map corresponding to the actual executable
4. Using memory segments, find offsets using patterns and patch as needed
5. Detach

## Building

See `CMakeLists.txt` and `demo.cpp` for a good example.

## TODO

- Tools for other games (?)

## Credits

Heavily based on the amazing work of others:
- [SekiroFpsUnlockAndMore](https://github.com/uberhalit/SekiroFpsUnlockAndMore)
- [sekirofpsunlock](https://github.com/Lahvuun/sekirofpsunlock)
- [KittyMemoryEx](https://github.com/MJx0/KittyMemoryEx)
