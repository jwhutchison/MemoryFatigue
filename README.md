# MemoryFatigue

General purpose memory pattern and patch utility for Linux. Will work with any process that can be
read or modified via `/proc/pid`, but primarily was developed to make memory patching Windows games
running in Linux easier and more consistent.

Currently, you must build the tool that you want, but a general purpose cli tool is planned. More
documentation is incoming, but for now, have a look at demo.cpp or patchers/**/main.cpp

Note, this is meant for Linux, and for my use case in particular. It may or may not work in other
environments that support the `/proc/pid` interface and PTRACE. I welcome contributions.

Basic use:

1. Locate process PID, usually by executable name
2. Attach to process
3. Find the process memory map corresponding to the actual executable
4. Using memory segments, find offsets using patterns and patch as needed
5. Detach

## Building

See `CMakeLists.txt` and `demo.cpp` for a good example.

## TODO

- General purpose cli tool
- ELF helpers
- .so build
- Move handy utilities to their own repos (log, color)
- Tools for other games (?)

## Credits

Heavily based on the amazing work of others:
- [SekiroFpsUnlockAndMore](https://github.com/uberhalit/SekiroFpsUnlockAndMore)
- [sekirofpsunlock](https://github.com/Lahvuun/sekirofpsunlock)
- [KittyMemoryEx](https://github.com/MJx0/KittyMemoryEx)
