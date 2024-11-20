# MemoryFatigue

General purpose memory pattern and patch utility for Linux. Will work with any process that can be
read or modified via `/proc/pid`, but primarily was developed to make memory patching Windows games
running in Linux easier and more consistent. The library has very rudimentary support for Elf executables,
but YMMV.

## Fatigue CLI

There is a simple CLI command to make one off memory patches or to dry-run finding processes by various
names. Download from releases and run `fatigue --help` for usage.

At least one of PID, status name, or cmdline must be specified, and you may supply a pattern and patch
to apply.

Examples:

- `build/bin/fatigue -s "sekiro.exe" --pattern "C6 86 ?? ?? 00 00 ?? F3 0F 10 8E ?? ?? 00 00" --offset 6 --patch "00" -v` -
  Find the sekiro exe by status name, locate pattern and apply patch at offset, with verbose output.
- `build/bin/fatigue -p 523340` - Get process by pid 523340, print basic information and exit.
- `build/bin/fatigue -c "games\\sekiro.exe" --pattern "C6 86 ?? ??0" --offset 6 --patch "00" -d` - Find a process
  by cmdline portion, find a pattern and show patch information, but do not apply it (dry run)
- `build/bin/fatigue -s "sekiro.exe" --pattern "C6 86 ?? ??"` - Find a process by status name, search for
  a pattern, show information and exit

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
