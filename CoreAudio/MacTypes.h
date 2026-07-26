#ifndef __MACTYPES__
#define __MACTYPES__

typedef unsigned char      UInt8;
typedef signed char        SInt8;
typedef unsigned short     UInt16;
typedef signed short       SInt16;
typedef unsigned int       UInt32;
typedef signed int         SInt32;
typedef signed long long   SInt64;
typedef unsigned long long UInt64;
typedef float              Float32;
typedef double             Float64;

typedef unsigned int       FourCharCode;
typedef SInt32             OSStatus;
typedef FourCharCode       OSType;
typedef unsigned char      Boolean;
typedef long               ComponentResult;
struct ComponentInstanceRecord {
  long                data[1];
};
typedef struct ComponentInstanceRecord  ComponentInstanceRecord;
typedef ComponentInstanceRecord *       ComponentInstance;

typedef UInt16             UniChar;

enum {
  kVariableLengthArray          = 1
};
#endif
