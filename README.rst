=====================================
qaac - CLI QuickTime AAC/ALAC encoder
=====================================

How to build
------------
You need QuickTime SDK 7.3 for Windows to build qaac. You can download QuickTime SDK from http://developer.apple.com/quicktime/ (registration is required).

SDK is installed under C:/Program Files/QuickTime SDK (or C:/Program Files (x86)/QuickTime SDK, if you are using 64bit OS).

You have to modify project files(vcxproj), if your SDK install directory is different from the project settings.


How to build with mingw
-----------------------
No so much tested, and not recommended.
You need working autotools to generate mp4v2 configure script.

::

$ pushd mp4v2
$ ./make_configure
$ ./configure --host=i686-w64-mingw32 --prefix=/usr/i686-w64-mingw32 --enable-shared=no --disable-util  # depends on your environment
$ make CPPFLAGS=-DMP4V2_USE_STATIC_LIB 
$ popd
$ . env.sh
$ make

Resulting binary will be VERY big.
You may want to strip qaac.exe with i686-w64-mingw32-strip or something.
