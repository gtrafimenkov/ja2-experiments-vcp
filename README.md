# Jagged Alliance 2 Cross-Platform in Rust

This is a rewrite of [ja2-vanilla](https://github.com/gtrafimenkov/ja2-vanilla-cp) project
in Rust.

Project goals:
- full rewrite in Rust

Non-goals:
- adding new functionality
- supporting game mods

## Project structure

```
ja2                - game sources
libs               - third-party libraries
rustlib            - Rust code compiled to a static library
```

## Build requirements

- CMake
- GCC or Clang for Linux
- Visual Studio Community 2022 for Windows
- Rust v1.67.0 or later


## How to build, test, and run

```
python xx.py build-debug copy-data run
```

## How to play the game

- install the original version of the game (from the original game CDs, Steam, gog.com, etc.)
- `python xx.py build-debug`
- `python xx.py copy-data run` or copy `build/Debug/ja2vcp.exe` to the game directory
   alongside the original ja2.exe and run `ja2vcp.exe`

The game is tested on Windows 10.

## License

This is not open source as defined by [The Open Source Definition](https://opensource.org/osd/).
The original JA2 sources were released under the terms of [SFI-SCLA](SFI-SCLA.txt) license.
Please read it carefully and make your own mind regarding this.
