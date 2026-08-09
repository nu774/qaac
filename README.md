# qaac - CLI QuickTime AAC/ALAC encoder

## Notice(2024-12-21)

It turned out that CoreAudioToolbox 7.9.8.x or greater (upto 7.10.9.0, the latest version at the moment) can produce glitches on the encoded result.  
The issue is only found on AAC **CBR** mode.

CoreAudioToolbox 7.9.7.x is OK, but you need very old iTunes installer for that version (it's released on 2012).

cf. https://hydrogenaud.io/index.php/topic,85135.msg1056191.html#msg1056191


## How to build

qaac/refalac are built with [CMake](https://cmake.org/) (`CMakeLists.txt` at
the repository root). The basic invocation is always the same:
```
cmake -B build -G Ninja .
cmake --build build
```
(drop `-G Ninja` to use the default generator instead -- e.g. `make` on
Linux/macOS, or a Visual Studio solution on Windows -- or pass `-G "Unix
Makefiles"` explicitly if you prefer `make` over `ninja`.)

### Windows

qaac needs the platform SDK (for the Win32 API) either way, so on Windows
you always build from a **Visual Studio Developer Command Prompt** (or a
shell that has run `vcvarsall.bat`/`Import-VisualStudioVars`), except when
using MinGW-w64. Several toolchains are known to work; pick whichever you
already have installed:

- **MSVC (`cl`)**, from a Developer Command Prompt:
  ```
  cmake -B build -G Ninja .
  cmake --build build
  ```
  Or, from Visual Studio 2017 or later: use "Open > Folder..." on this
  repository; Visual Studio's built-in CMake support will configure it and
  you can build/debug directly from the IDE.
- **clang-cl**, from a Developer Command Prompt:
  ```
  cmake -B build -G Ninja -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl .
  cmake --build build
  ```
- **clang** targeting the MSVC ABI, from a Developer Command Prompt:
  ```
  cmake -B build -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ ^
      -DCMAKE_C_COMPILER_TARGET=x86_64-pc-windows-msvc -DCMAKE_CXX_COMPILER_TARGET=x86_64-pc-windows-msvc .
  cmake --build build
  ```
  (use `i686-pc-windows-msvc` for a 32-bit build)
- **MinGW-w64 GCC** (e.g. from MSYS2), no Developer Command Prompt needed:
  ```
  cmake -B build -G Ninja .
  cmake --build build
  ```
  Both the UCRT (MSYS2's UCRT64 environment) and the legacy MSVCRT (MINGW64
  environment) runtime variants build and work.
- **Cross-compiling from Linux with mingw-w64** (e.g.
  [llvm-mingw](https://github.com/mstorsjo/llvm-mingw), or a distro's
  `mingw-w64-gcc`):
  ```
  cmake -B build -G Ninja -DCMAKE_SYSTEM_NAME=Windows \
      -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ .
  cmake --build build
  ```

### Linux

On Linux, only refalac is available.
Needs a C/C++ toolchain (gcc or clang), cmake, and ninja or make. refalac links
against `iconv`, so make sure your libc's iconv development headers are
installed -- on glibc-based distros this is normally already pulled in with
the rest of the toolchain (e.g. as part of `libc6-dev`/`glibc-devel`), but if
CMake's `find_package(Iconv REQUIRED)` fails to find it, install your
distro's iconv/glibc development package explicitly.

ALSA development headers (`libasound2-dev` on Debian/Ubuntu) are optional --
without them refalac still build, but `--play` loses its ALSA backend
(falling back to the silent dummy backend).

```
cmake -B build -G Ninja .
cmake --build build
```

Optional codec libraries (FLAC, libsndfile, Opus, WavPack, libsoxr, ...) are
loaded at runtime via `dlopen()`, not linked at build time, so they aren't
required to build -- only to actually use the corresponding `--flac`
etc. features.

### macOS

Needs the Xcode Command Line Tools (`clang`; also provides the SDK and
`iconv`), cmake, and ninja or make. As on Linux, optional codec libraries are
loaded at runtime via `dlopen()`, so install whichever ones you want to use
via Homebrew, e.g.:
```
brew install flac libsndfile opus wavpack libsoxr
```

```
cmake -B build -G Ninja .
cmake --build build
```
