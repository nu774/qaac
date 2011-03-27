QTSDKDir=C:/Program Files (x86)/QuickTime SDK

TOBJS=\
taglib/audioproperties.o \
taglib/mpeg/id3v1/id3v1genres.o \
taglib/mpeg/id3v2/frames/attachedpictureframe.o \
taglib/mpeg/id3v2/frames/commentsframe.o \
taglib/mpeg/id3v2/frames/generalencapsulatedobjectframe.o \
taglib/mpeg/id3v2/frames/popularimeterframe.o \
taglib/mpeg/id3v2/frames/privateframe.o \
taglib/mpeg/id3v2/frames/relativevolumeframe.o \
taglib/mpeg/id3v2/frames/textidentificationframe.o \
taglib/mpeg/id3v2/frames/uniquefileidentifierframe.o \
taglib/mpeg/id3v2/frames/unknownframe.o \
taglib/mpeg/id3v2/frames/unsynchronizedlyricsframe.o \
taglib/mpeg/id3v2/frames/urllinkframe.o \
taglib/mpeg/id3v2/id3v2extendedheader.o \
taglib/mpeg/id3v2/id3v2footer.o \
taglib/mpeg/id3v2/id3v2frame.o \
taglib/mpeg/id3v2/id3v2framefactory.o \
taglib/mpeg/id3v2/id3v2header.o \
taglib/mpeg/id3v2/id3v2synchdata.o \
taglib/mpeg/id3v2/id3v2tag.o \
taglib/riff/aiff/aifffile.o \
taglib/riff/aiff/aiffproperties.o \
taglib/riff/rifffile.o \
taglib/tag.o \
taglib/toolkit/tbytevector.o \
taglib/toolkit/tbytevectorlist.o \
taglib/toolkit/tdebug.o \
taglib/toolkit/tfile.o \
taglib/toolkit/tstring.o \
taglib/toolkit/tstringlist.o \
taglib/toolkit/unicode.o 

COBJS=\
cfhelper.o \
chanmap.o \
channel.o \
flacmodule.o \
getopt.o \
iff.o \
iointer.o \
itunetags.o \
mp4v2wrapper.o \
qtmoviesource.o \
strcnv.o \
utf8_codecvt_facet.o \
util.o \
win32util.o

QOBJS=\
aacencoder.o \
alacsink.o \
cuesheet.o \
encoderbase.o \
flacsrc.o \
libsndfilesrc.o \
main.o \
options.o \
riff.o \
sink.o \
srcsource.o \
version.o \
wavsource.o \
wvpacksrc.o

LIBS=mp4v2/.libs/libmp4v2.a "$(QTSDKDir)/Libraries/QTMLClient.lib"
INCLUDES=-Iinclude -I "$(QTSDKDir)/CIncludes" -Imp4v2 -Imp4v2/include \
	 -Imp4v2/src -Itaglib -Itaglib/toolkit -Itaglib/mpeg/id3v1 \
	 -Itaglib/mpeg/id3v2 -Itaglib/riff -Itaglib/riff/aiff

CPPFLAGS =-DMP4V2_USE_STATIC_LIB -DTAGLIB_STATIC $(INCLUDES)
CXXFLAGS = -O -Wno-multichar -Wall
CFLAGS = -O -Wall

all: qaac

qaac: $(TOBJS) $(COBJS) $(QOBJS)
	$(CXX) -static-libgcc -static-libstdc++ -o $@ $(TOBJS) $(COBJS) $(QOBJS) $(LIBS) -lshlwapi

clean:
	find . -name '*.o' -exec rm '{}' +
