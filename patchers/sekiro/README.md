# Sekiro Patcher

Patch Sekiro running in Steam on Linux to unlock FPS, enable widescreen, etc.

To use, copy the executable somewhere convenient (like the game directory).
Then run the executable before running the game with the desired options, either
in a terminal (for testing), or in the Steam Launch Options like so:

`/home/path/to/sekiropatcher -f 120 -r 3440x1440 -ca &  %command%`

Or, for exameple, if using the Sekiro mod manager dll:

`/home/path/to/sekiropatcher -f 120 -r 3440x1440 -ca & WINEDLLOVERRIDES=dinput8=n,b %command%`

When starting the patcher, it will wait for Sekiro to start, make the required patches
and then exit. There is no program running in the background.

For a complete list of options, see `sekiropatcher --help`, but briefly:

- `-f` or `--fps` - Unlock max game FPS (between 30 and 300)
- `-r` or `--resolution` - Set game resolution (WxH), e.g. "3440x1440"
- `-c` or `--no-camera-reset` - Disable the camera reset "feature" on pressing the locl-on button when there is no visible target
- `-a` or `--autoloot` - Enable automatic looting (no need to hold the loot vacuum button)
