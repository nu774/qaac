# qaac - CLI QuickTime AAC/ALAC encoder

## Notice(2024-12-21)

It turned out that CoreAudioToolbox 7.9.8.x or greater (upto 7.10.9.0, the latest version at the moment) can produce glitches on the encoded result.  
The issue is only found on AAC **CBR** mode.

CoreAudioToolbox 7.9.7.x is OK, but you need very old iTunes installer for that version (it's released on 2012).

cf. https://hydrogenaud.io/index.php/topic,85135.msg1056191.html#msg1056191


## How to build

qaac/refalac are built with [CMake](https://cmake.org/) (`CMakeLists.txt` at
the repository root).

- From Visual Studio 2017 or later: use "Open > Folder..." on this
  repository; Visual Studio's built-in CMake support will configure it and
  you can build/debug directly from the IDE.
- From the command line:
  ```
  cmake -B build
  cmake --build build --config Release
  ```
