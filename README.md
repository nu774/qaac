# qaac - CLI QuickTime AAC/ALAC encoder

## Notice(2024-12-21)

It turned out that CoreAudioToolbox 7.9.8.x or greater (upto 7.10.9.0, the latest version at the moment) can produce glitches on the encoded result.  
The issue is only found on AAC **CBR** mode.

CoreAudioToolbox 7.9.7.x is OK, but you need very old iTunes installer for that version (it's released on 2012).

cf. https://hydrogenaud.io/index.php/topic,85135.msg1056191.html#msg1056191


## How to build

You need Microsoft Visual C++ 2010 to build qaac/refalac.
AMD64 build is only available for refalac.
