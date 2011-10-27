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
qtimage.o \
qtmoviesource.o \
riff.o \
strcnv.o \
utf8_codecvt_facet.o \
util.o \
wavsource.o \
wicimage.o \
win32util.o

QOBJS=\
aacconfig.o \
aacencoder.o \
alacsink.o \
cuesheet.o \
encoderbase.o \
flacsrc.o \
libsndfilesrc.o \
logging.o \
main.o \
options.o \
reg.o \
resampler.o \
sink.o \
taksrc.o \
version.o \
wvpacksrc.o

AOBJS=\
alacdec.o \
flacsink.o \
wavsink.o

LIBS="$(QTSDKDir)/Libraries/QTMLClient.lib"
INCLUDES=-Iinclude -I "$(QTSDKDir)/CIncludes" -Imp4v2 -Imp4v2/include \
	 -Imp4v2/src -Itaglib -Itaglib/toolkit -Itaglib/mpeg/id3v1 \
	 -Itaglib/mpeg/id3v2 -Itaglib/riff -Itaglib/riff/aiff

CPPFLAGS =-DMP4V2_USE_STATIC_LIB -DTAGLIB_STATIC $(INCLUDES)
CFLAGS = -O2 -Wall -Wno-multichar
CXXFLAGS =$(CFLAGS)
LDFLAGS = -static-libgcc -static-libstdc++ -lshlwapi -luuid -lmp4v2 $(LIBPATH)

all: qaac alacdec

qaac: $(TOBJS) $(COBJS) $(QOBJS)
	$(CXX) -o $@ $(TOBJS) $(COBJS) $(QOBJS) $(LIBS) $(LDFLAGS)

alacdec: $(TOBJS) $(COBJS) $(AOBJS)
	$(CXX) -o $@ $(TOBJS) $(COBJS) $(AOBJS) $(LIBS) $(LDFLAGS)

clean:
	find . -name '*.o' -exec rm '{}' +
