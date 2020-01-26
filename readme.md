Music Transcoder To MP3
=======================

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
* **Video formats**: mp4.

# Compilation requirements
## To build the tool:
* cross-platform build system: [CMake](http://www.cmake.org/cmake/resources/software.html).
* compiler: [Mingw64](http://sourceforge.net/projects/mingw-w64/) on Windows or [gcc](http://gcc.gnu.org/) on Linux.

## External dependencies:
The following libraries are required to build the tool:
* [Qt opensource framework](http://www.qt.io/).
* [Boost libraries](https://www.boost.org/).

The tool will need the following applications to play selected media:
* [WinAmp](http://www.winamp.com/).
* [SMPlayer](https://www.smplayer.info/).
* [Castnow](https://github.com/xat/castnow).

# Install

Binaries are not provided.

# Screenshots

Simple main dialog. Next nutton is only available while casting files to Chromecast as WinAmp and SMPlayer will load files as a playlist and play them sequentially. 

![Main dialog]()

Settings configuration dialog. If any of the applications is not present playing for it will be disabled by the tool.

![Configuration Dialog]()

# Repository information
**Version**: 2.0

**Status**: finished

**License**: GNU General Public License 3

**cloc statistics**

| Language                     |files          |blank        |comment           |code  |
|:-----------------------------|--------------:|------------:|-----------------:|-----:|
| C++                          |    5          |  237        |    140           | 861  |
| C/C++ Header                 |    5          |  105        |    274           | 194  |
| CMake                        |    2          |   20        |     18           |  65  |
| **Total**                    |   **12**      |  **362**    |   **432**        |**1120**|
