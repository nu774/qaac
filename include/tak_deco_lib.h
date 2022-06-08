/* unit tak_decoder_lib; */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#define TAK_API __declspec(dllimport)
#else
#error Port me!
#endif



/*=== TAK SDK ======================================================*/

/*
  Software Developement Kit for TAK, (T)om's lossless (A)udio (K)ompressor:
  Decoder library.

  Version:  2.3.2
  Date:     22-05-06
  Language: C

  Copyright 2007 by Thomas Becker, D-49080 Osnabrueck.
  All rights reserved.
*/



/*=== Elemental Types ===============================================*/

#ifdef _MSC_VER
typedef __int32      TtakInt32;
typedef unsigned int TtakUInt32;
typedef __int64      TtakInt64;
typedef __int32      TtakBool;
typedef char         TtakAnsiChar;
typedef wchar_t      TtakWideChar;
#else
#error Port me!
#endif

enum {
  tak_False = 0,
  tak_True  = 1,
};



/*=== Function results and errors ====================================*/

typedef TtakInt32 TtakResult;

enum tak_ErrorStringLimits {
  tak_ErrorStringLenMax  = 60,
  tak_ErrorStringSizeMax = 61,
};

enum tak_res {
  tak_res_Ok = 0,

  /*---    1 to 1023: Object results ---*/

  /*--- 1024 to 2047: System errors ---*/

  tak_res_InternalError       = 1024,
  tak_res_NotImplemented      = 1025,
  tak_res_IncompatibleVersion = 1026,
  tak_res_OutOfMemory         = 1027,

  /*--- 2048 to 3071: User errors ---*/

  tak_res_InvalidParameter    = 2048,
  tak_res_InvalidIoInterface  = 2049,
  tak_res_InvalidMode         = 2050,
  tak_res_BufferTooSmall      = 2051,
  tak_res_NotEnoughAudioData  = 2052,
  tak_res_TooMuchAudioData    = 2053,
};



/*=== System =========================================================*/

enum tak_FilePathStringLimits {
  tak_MaxPathLen  = 256,
  tak_MaxPathSize = 257,
};

typedef TtakInt32 TtakCpuOptions;

enum tak_Cpu {
  tak_Cpu_None  = 0x0000,
  tak_Cpu_Asm   = 0x0001,
  tak_Cpu_MMX   = 0x0002,
  tak_Cpu_SSE   = 0x0004,
  tak_Cpu_SSE2  = 0x0008,
  tak_Cpu_SSE3  = 0x0010,
  tak_Cpu_SSSE3 = 0x0020,
  tak_Cpu_SSE41 = 0x0040,
  tak_Cpu_SSE42 = 0x0080,
  tak_Cpu_AVX   = 0x0100,
  tak_Cpu_AVX2  = 0x0200,
  tak_Cpu_Any   =    tak_Cpu_Asm   | tak_Cpu_MMX   | tak_Cpu_SSE
                  | tak_Cpu_SSE2  | tak_Cpu_SSE3  | tak_Cpu_SSSE3
                  | tak_Cpu_SSE41 | tak_Cpu_SSE42 | tak_Cpu_AVX
                  | tak_Cpu_AVX2,
};



/*=== Library ========================================================*/

enum tak_Version {
  tak_InterfaceVersion = 0x00020302,
};

TAK_API TtakResult tak_GetLibraryVersion (TtakInt32 * AVersion,
                                          TtakInt32 * ACompatibility);



/*=== Audio Format ===================================================*/

enum tak_AudioFormat_DataType {
  tak_AudioFormat_DataType_PCM = 0,
};

enum tak_AudioFormat_Limits {
  tak_str_ChannelNumMax = 16,
};

enum tak_Speaker_Flags {
  tak_Speaker_Flag_None                  = 0x00000,
  tak_Speaker_Flag_Front_Left            = 0x00001,
  tak_Speaker_Flag_Front_Right           = 0x00002,
  tak_Speaker_Flag_Front_Center          = 0x00004,
  tak_Speaker_Flag_Low_Frequency         = 0x00008,
  tak_Speaker_Flag_Back_Left             = 0x00010,
  tak_Speaker_Flag_Back_Right            = 0x00020,
  tak_Speaker_Flag_Front_Left_Of_Center  = 0x00040,
  tak_Speaker_Flag_Front_Right_Of_Center = 0x00080,
  tak_Speaker_Flag_Back_Center           = 0x00100,
  tak_Speaker_Flag_Side_Left             = 0x00200,
  tak_Speaker_Flag_Side_Right            = 0x00400,
  tak_Speaker_Flag_Top_Center            = 0x00800,
  tak_Speaker_Flag_Top_Front_Left        = 0x01000,
  tak_Speaker_Flag_Top_Front_Center      = 0x02000,
  tak_Speaker_Flag_Top_Front_Right       = 0x04000,
  tak_Speaker_Flag_Top_Back_Left         = 0x08000,
  tak_Speaker_Flag_Top_Back_Center       = 0x10000,
  tak_Speaker_Flag_Top_Back_Right        = 0x20000,

  tak_Speaker_Flag_Max_Supported         = tak_Speaker_Flag_Top_Back_Right,
  tak_Speaker_Flag_Mask                  =    tak_Speaker_Flag_Max_Supported
                                           | (tak_Speaker_Flag_Max_Supported - 1),
  tak_Speaker_Flag_Max                   = 1 << 31,
};

enum tak_Speaker_Values {
  tak_Speaker_None                  =  0,
  tak_Speaker_Front_Left            =  1,
  tak_Speaker_Front_Right           =  2,
  tak_Speaker_Front_Center          =  3,
  tak_Speaker_Low_Frequency         =  4,
  tak_Speaker_Back_Left             =  5,
  tak_Speaker_Back_Right            =  6,
  tak_Speaker_Front_Left_Of_Center  =  7,
  tak_Speaker_Front_Right_Of_Center =  8,
  tak_Speaker_Back_Center           =  9,
  tak_Speaker_Side_Left             = 10,
  tak_Speaker_Side_Right            = 11,
  tak_Speaker_Top_Center            = 12,
  tak_Speaker_Top_Front_Left        = 13,
  tak_Speaker_Top_Front_Center      = 14,
  tak_Speaker_Top_Front_Right       = 15,
  tak_Speaker_Top_Back_Left         = 16,
  tak_Speaker_Top_Back_Center       = 17,
  tak_Speaker_Top_Back_Right        = 18,

  tak_Speaker_Max_Supported         = tak_Speaker_Top_Back_Right,
  tak_Speaker_Min                   = 0,
  tak_Speaker_Max                   = 63,
};

typedef struct TtakAudioFormat {
    TtakInt32 DataType;
    TtakInt32 SampleRate;
    TtakInt32 SampleBits;
    TtakInt32 ChannelNum;
    TtakInt32 BlockSize;
} TtakAudioFormat;

typedef char TtakSpeakerAssignment[tak_str_ChannelNumMax];

typedef struct TtakAudioFormatEx {
    TtakInt32             DataType;
    TtakInt32             SampleRate;
    TtakInt32             SampleBits;
    TtakInt32             ChannelNum;
    TtakInt32             BlockSize;

    TtakBool              HasExtension;
    TtakInt32             ValidBitsPerSample;
    TtakBool              HasSpeakerAssignment;
    TtakSpeakerAssignment SpeakerAssignment;
} TtakAudioFormatEx;


TAK_API TtakBool tak_GetWaveExtensibleSpeakerMask (TtakAudioFormatEx *  AFormat,
                                                   TtakUInt32 *         AMask);



/*=== Codecs =========================================================*/

enum tak_CodecNameStringLimits {
  tak_CodecNameLenMax  = 30,
  tak_CodecNameSizeMax = 31,
};

TAK_API TtakResult tak_GetCodecName (TtakInt32       ACodec,
                                     TtakAnsiChar *  AName,
                                     TtakInt32       ANameSize);



/*=== Presets ========================================================*/


  /*-- Presets/Profiles ---*/

typedef TtakInt32 TtakPresets;

  /*-- Evaluation ---*/

typedef TtakInt32 TtakPresetEvaluations;

enum tak_PresetEval {
  tak_PresetEval_Standard = 0,
  tak_PresetEval_Extra    = 1,
  tak_PresetEval_Max      = 2,
  tak_PresetEval_First    = tak_PresetEval_Standard,
  tak_PresetEval_Last     = tak_PresetEval_Max,
  tak_PresetEval_Num      = tak_PresetEval_Last - tak_PresetEval_First + 1,
};



/*=== Stream / Container =============================================*/

enum tak_str_FrameLimits {
  tak_FrameSizeMax     = 16384,
  tak_FrameDurationMax = 250,
};

enum tak_str_SimpleWaveDataSizeLimits {
  tak_str_SimpleWaveDataSizeMax = 1024 * 1024,
};

typedef struct Ttak_str_EncoderInfo {
    TtakInt32   Codec;
    TtakPresets Profile;
} Ttak_str_EncoderInfo;


typedef TtakInt32 Ttak_str_FrameSizeTypes;

enum tak_str_FrameSizeType {
  tak_str_FrameSizeType_94_ms  =  0,
  tak_str_FrameSizeType_125_ms =  1,
  tak_str_FrameSizeType_188_ms =  2,
  tak_str_FrameSizeType_250_ms =  3,
  tak_str_FrameSizeType_4096   =  4,
  tak_str_FrameSizeType_8192   =  5,
  tak_str_FrameSizeType_16384  =  6,
  tak_FrameSizeType_512        =  7,
  tak_FrameSizeType_1024       =  8,
  tak_FrameSizeType_2048       =  9,
};

typedef struct Ttak_str_SizeInfo {
    Ttak_str_FrameSizeTypes FrameSize;
    TtakInt32 FrameSizeInSamples;
    TtakInt64 SampleNum;
} Ttak_str_SizeInfo;

typedef struct Ttak_str_StreamInfo {
    Ttak_str_EncoderInfo Encoder;
    Ttak_str_SizeInfo    Sizes;
    TtakAudioFormat      Audio;
} Ttak_str_StreamInfo;

typedef struct Ttak_str_StreamInfo_V22 {
    Ttak_str_EncoderInfo Encoder;
    Ttak_str_SizeInfo    Sizes;
    TtakAudioFormatEx    Audio;
} Ttak_str_StreamInfo_V22;

typedef struct Ttak_str_SimpleWaveDataHeader {
    TtakInt32 HeadSize;
    TtakInt32 TailSize;
} Ttak_str_SimpleWaveDataHeader;

typedef struct Ttak_str_MetaEncoderInfo {
    TtakInt32             Version;
    TtakPresets           Preset;
    TtakPresetEvaluations Evaluation;
} Ttak_str_MetaEncoderInfo;

typedef char Ttak_str_MD5[16];



/*=== TtakStreamIoInterface ==========================================*/

typedef struct TtakStreamIoInterface {
    TtakBool (*CanRead)  (void * AUser);
    TtakBool (*CanWrite) (void * AUser);
    TtakBool (*CanSeek)  (void * AUser);

    TtakBool (*Read)     (void *      AUser,
                          void *      ABuf,
                          TtakInt32   ANum,
                          TtakInt32 * AReadNum );

    TtakBool (*Write)    (void *       AUser,
                          const void * ABuf,
                          TtakInt32    ANum);

    TtakBool (*Flush)    (void * AUser);

    TtakBool (*Truncate) (void * AUser);

    TtakBool (*Seek)     (void *    AUser,
                          TtakInt64 APos);

    TtakBool (*GetLength)(void *      AUser,
                          TtakInt64 * ALength);

} TtakStreamIoInterface;

typedef TtakStreamIoInterface* PtakStreamIoInterface;



/*=== APEv2-Tag (APE) ================================================*/

typedef void * TtakAPEv2Tag;

enum tak_ape_limits {
  tak_apev2_Version    = 2000,
  tak_apev2_ItemNumMax = 100,
  tak_apev2_TagSizeMax = 16 * 1024 * 1024,
};

typedef TtakInt32 TtakAPEv2ItemType;

enum tak_apev2_ItemType {
  tak_apev2_ItemType_Text     = 0,
  tak_apev2_ItemType_Binary   = 1,
  tak_apev2_ItemType_External = 2,
  tak_apev2_ItemType_Last     = tak_apev2_ItemType_External,
};

typedef struct TtakAPEv2ItemDesc {
    TtakAPEv2ItemType ItemType;
    TtakUInt32        Flags;
    TtakUInt32        KeySize;
    TtakUInt32        ValueSize;
    TtakInt32         ValueNum;
} TtakAPEv2ItemDesc;

typedef struct TtakAPEv2TagDesc {
    TtakUInt32 Version;
    TtakUInt32 Flags;
    TtakInt64  StreamPos;
    TtakInt64  TotSize;
} TtakAPEv2TagDesc;


enum tak_res_ape {

  /*--- Warnings ---*/

  tak_res_ape_NotAvail       = 1,
  tak_res_ape_InvalidType    = 2,
  tak_res_ape_BufferTooSmall = 3,

  /*--- Fatal errors ---*/

  tak_res_ape_None         = 4,
  tak_res_ape_Incompatible = 5,
  tak_res_ape_Invalid      = 6,
  tak_res_ape_IoErr        = 7,

  tak_res_ape_FatalErrorFirst = tak_res_ape_None,
};



/*--- Info ----------------------------------------------------------*/

TAK_API TtakBool tak_APE_Valid (TtakAPEv2Tag ATag);

TAK_API TtakResult tak_APE_State (TtakAPEv2Tag ATag);

TAK_API TtakResult tak_APE_GetErrorString (TtakResult     AError,
                                           TtakAnsiChar * AString,
                                           TtakInt32      AStringSize);

TAK_API TtakBool tak_APE_ReadOnly (TtakAPEv2Tag ATag);

TAK_API TtakResult tak_APE_GetDesc (TtakAPEv2Tag       ATag,
                                    TtakAPEv2TagDesc * ADesc);

TAK_API TtakInt32 tak_APE_GetItemNum (TtakAPEv2Tag ATag);



/*--- Items ---------------------------------------------------------*/

TAK_API TtakResult tak_APE_GetIndexOfKey (      TtakAPEv2Tag   ATag,
                                          const TtakAnsiChar * AKey,
                                                TtakInt32    * AIdx);

TAK_API TtakResult tak_APE_GetItemDesc (TtakAPEv2Tag        ATag,
                                        TtakInt32           AIdx,
                                        TtakAPEv2ItemDesc * ADesc);


TAK_API TtakResult tak_APE_GetItemKey (TtakAPEv2Tag   ATag,
                                       TtakInt32      AIdx,
                                       TtakAnsiChar * AKey,
                                       TtakInt32      AMaxSize,
                                       TtakInt32 *    ASize);

TAK_API TtakResult tak_APE_GetItemValue (TtakAPEv2Tag  ATag,
                                         TtakInt32     AIdx,
                                         void *        AValue,
                                         TtakInt32     AMaxSize,
                                         TtakInt32 *   ASize);

TAK_API TtakResult tak_APE_GetTextItemValueAsAnsi (TtakAPEv2Tag   ATag,
                                                   TtakInt32      AIdx,
                                                   TtakInt32      AValueIdx,
                                                   TtakAnsiChar   AValueSeparator,
                                                   TtakAnsiChar * AValue,
                                                   TtakInt32      AMaxSize,
                                                   TtakInt32 *    ASize);



/*=== Seekable Stream Decoder (SSD) ==================================*/

typedef void * TtakSeekableStreamDecoder;

typedef struct TtakSSDDamageItem {
    TtakInt64 SamplePosition;
    TtakInt64 SampleSize;
} TtakSSDDamageItem, *PtakSSDDamageItem;

typedef void (*TSSDDamageCallback)(void *            AUser,
                                   PtakSSDDamageItem ADamage);

enum tak_ssd_opt {
  tak_ssd_opt_OpenWriteable     = 0x00000001,
  tak_ssd_opt_BufferInput       = 0x00000002,
  tak_ssd_opt_SequentialRead    = 0x00000004,
  tak_ssd_opt_SkipDamagedFrames = 0x00000008,
  tak_ssd_opt_CheckMd5          = 0x00000010,
};

typedef struct TtakSSDOptions {
    TtakCpuOptions Cpu;
    TtakInt32 Flags;
} TtakSSDOptions;


enum tak_res_ssd {

  /*--- Warnings ---*/

  tak_res_ssd_MetaDataMissing = 1,

  /*--- Errors ---*/

  tak_res_ssd_MetaDataDamaged = 2,
  tak_res_ssd_FrameDamaged    = 3,
  tak_res_ssd_ErrorFirst      = tak_res_ssd_MetaDataDamaged,

  /*--- Fatal Errors ---*/

  tak_res_ssd_SourceIoError       = 4,
  tak_res_ssd_IncompatibleVersion = 5,
  tak_res_ssd_Undecodable         = 6,
  tak_res_ssd_FatalErrorFirst     = tak_res_ssd_SourceIoError,
};

typedef struct TtakSSDResult {
    TtakResult OpenResult;
    TtakResult SumResult;
    TtakInt64  StreamSampleNum;
    TtakInt64  ReadSampleNum;
    TtakInt64  DamagedSampleNum;
    TtakInt32  SkippedDataBlockNum;
} TtakSSDResult;



/*--- Create & Destroy -----------------------------------------------*/

TAK_API TtakSeekableStreamDecoder
  tak_SSD_Create_FromFile (const TtakAnsiChar *   ASourcePath,
                           const TtakSSDOptions * AOptions,
                           TSSDDamageCallback     ADamageCallback,
                           void *                 ACallbackUser);

TAK_API TtakSeekableStreamDecoder
  tak_SSD_Create_FromFileW (const TtakWideChar *   ASourcePath,
                            const TtakSSDOptions * AOptions,
                            TSSDDamageCallback     ADamageCallback,
                            void *                 ACallbackUser);

TAK_API TtakSeekableStreamDecoder
  tak_SSD_Create_FromStream (const TtakStreamIoInterface * ASourceStream,
                             void *                        ASourceStreamUser,
                             const TtakSSDOptions *        AOptions,
                             TSSDDamageCallback            ADamageCallback,
                             void *                        ACallbackUser);

TAK_API void tak_SSD_Destroy(TtakSeekableStreamDecoder ADecoder);



/*--- Info -----------------------------------------------------------*/

TAK_API TtakBool tak_SSD_Valid (TtakSeekableStreamDecoder ADecoder);

TAK_API TtakResult tak_SSD_State (TtakSeekableStreamDecoder ADecoder);

TAK_API TtakResult tak_SSD_GetStateInfo (TtakSeekableStreamDecoder ADecoder,
                                         TtakSSDResult *           AInfo);

TAK_API TtakResult tak_SSD_GetErrorString (TtakResult     AError,
                                           TtakAnsiChar * AString,
                                           TtakInt32      AStringSize);

TAK_API TtakResult tak_SSD_GetStreamInfo (TtakSeekableStreamDecoder ADecoder,
                                          Ttak_str_StreamInfo *     AInfo);

TAK_API TtakResult tak_SSD_GetStreamInfo_V22 (TtakSeekableStreamDecoder ADecoder,
                                              Ttak_str_StreamInfo_V22 * AInfo);

TAK_API TtakInt32 tak_SSD_GetFrameSize (TtakSeekableStreamDecoder ADecoder);

TAK_API TtakResult tak_SSD_GetMD5 (TtakSeekableStreamDecoder ADecoder,
                                   Ttak_str_MD5              AMD5);



/*--- IO -------------------------------------------------------------*/

TAK_API TtakResult tak_SSD_Seek (TtakSeekableStreamDecoder ADecoder,
                                 TtakInt64                 ASamplePos);

TAK_API TtakResult tak_SSD_ReadAudio (TtakSeekableStreamDecoder ADecoder,
                                      void *                    ASamples,
                                      TtakInt32                 ASampleNum,
                                      TtakInt32 *               AReadNum);

TAK_API TtakInt64 tak_SSD_GetReadPos (TtakSeekableStreamDecoder ADecoder);

TAK_API TtakInt32 tak_SSD_GetCurFrameBitRate (TtakSeekableStreamDecoder ADecoder);

TAK_API TtakResult tak_SSD_GetSimpleWaveDataDesc (TtakSeekableStreamDecoder       ADecoder,
                                                  Ttak_str_SimpleWaveDataHeader * ADesc);

TAK_API TtakResult tak_SSD_ReadSimpleWaveData (TtakSeekableStreamDecoder ADecoder,
		                               void *                    ABuf,
                                               TtakInt32                 ABufSize);

TAK_API TtakResult tak_SSD_GetEncoderInfo (TtakSeekableStreamDecoder   ADecoder,
                                           Ttak_str_MetaEncoderInfo  * AInfo);

TAK_API TtakAPEv2Tag tak_SSD_GetAPEv2Tag (TtakSeekableStreamDecoder ADecoder);



#ifdef __cplusplus
} /* extern "C" */
#endif

