# tf3-macos-hidpi-workaround

Enables native 4K rendering in Transport Fever 3's borderless mode on macOS, preserving desktop scaling and app switching.

## Requirements

- Apple Silicon Mac
- Transport Fever 3 installed through Steam
- Xcode or Apple Command Line Tools (`xcode-select --install`)

## Usage

Set the game to **Borderless** with **Resolution Scale: 100%**, then quit it.

With Steam running, execute from this repository:

```sh
sh hidpi/launch.sh
```

The library builds automatically on first launch. Alternatively, run `Launch TF3 HiDPI.command`.

For a non-default installation:

```sh
TPF3_GAME_DIR='/Volumes/Games/steamapps/common/Transport Fever 3' sh hidpi/launch.sh
```

To disable the workaround, quit the game and launch normally through Steam.

## Compatibility

Verified 4K output and Command–Tab with TF3 build 40408 on macOS 27.0, using a 4K display scaled to 1920 × 1080.

The resolution dropdown may retain the saved fullscreen value. Confirm output in the game's `crash_dump/stdout.txt`:

```text
Swapchain recreated: size = 3840x2160 (3840x2160)
```

## Implementation

The launcher loads an SDL2 compatibility library that enables HiDPI rendering and converts window dimensions and mouse coordinates to pixels. Game installation files are unchanged; settings and logs are backed up to `hidpi/test-backup.*`.

Rebuild after source changes:

```sh
sh hidpi/build.sh
```
