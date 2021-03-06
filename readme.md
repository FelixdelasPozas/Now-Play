Now Play!
=========

# Summary
- [Description](#description)
- [Compilation](#compilation-requirements)
- [Install](#install)
- [Screenshots](#screenshots)
- [Repository information](#repository-information)

# Description
Now Play! is a simple little tool to help play media files in various programs. The tool balloned from a simple command line application to a Qt 5 desktop one fairly quickly, just because I tend
to listen a lot of music while coding.

The tool will choose a random directory from the stablished 'base directory' and play the files in that sub-directory sequentially in the selected application (WinAmp, SMPlayer or Chromecast). If
the tool can't find a sub-directory from the base given one, then will search the base directory for playable files. 

Optionally, given a limit size and a destination will copy a random selection of the base subdirectories to destination up to the given limit (i.e. to fill a thumb drive with media files).

## Input file formats
The following file formats are detected and supported by the tool as input files:
* **Audio formats**: mp3, m3u playlist, m3u8 playlist.
* **Video formats**: mp4, mkv, webm.

# Compilation requirements
## To build the tool:
* cross-platform build system: [CMake](http://www.cmake.org/cmake/resources/software.html).
* compiler: [Mingw64](http://sourceforge.net/projects/mingw-w64/) on Windows or [gcc](http://gcc.gnu.org/) on Linux.

## External dependencies:
The following libraries are required to build the tool:
* [Qt opensource framework](http://www.qt.io/).

The tool will need the following applications to play selected media:
* [WinAmp](http://www.winamp.com/).
* [SMPlayer](https://www.smplayer.info/).
* [Castnow](https://github.com/xat/castnow).

# Install

Binaries are not provided.

# Screenshots

Simple main dialog. Next button is only available while casting files to Chromecast as WinAmp and SMPlayer will load files as a playlist and play them sequentially. 

![Main dialog](https://user-images.githubusercontent.com/12167134/73141718-7b342900-4087-11ea-8e3a-ff02dc8610eb.jpg)

Settings configuration dialog. If any of the applications is not present playing for it will be disabled by the tool.

![Configuration Dialog](https://user-images.githubusercontent.com/12167134/81506591-ae798b80-92f7-11ea-9a38-5d7a1b5b6a98.png)

# Repository information
**Version**: 2.5.1

**Status**: finished

**License**: GNU General Public License 3

**cloc statistics**

| Language                     |files          |blank        |comment           |code   |
|:-----------------------------|--------------:|------------:|-----------------:|------:|
| C++                          |    6          |  315        |    168           | 1197  |
| C/C++ Header                 |    6          |  130        |    337           |  250  |
| CMake                        |    2          |   21        |     17           |   72  |
| **Total**                    |   **14**      |  **466**    |   **522**        |**1519**|
