#ifndef CoreFoundation_H__
#define CoreFoundation_H__

#include "MacTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef const void * CFTypeRef;
typedef SInt32 CFIndex;

typedef const struct __CFString * CFStringRef;
typedef struct __CFString * CFMutableStringRef;

typedef const struct __CFAllocator * CFAllocatorRef;

typedef struct {
    CFIndex location;
    CFIndex length;
} CFRange;

CFTypeRef CFRetain(CFTypeRef cf);
void CFRelease(CFTypeRef cf);

CFStringRef CFStringCreateWithCharacters(CFAllocatorRef alloc, const UniChar *chars, CFIndex numChars);
CFIndex CFStringGetLength(CFStringRef theString);
void CFStringGetCharacters(CFStringRef theString, CFRange range, UniChar *buffer);
#define CFSTR(cStr)  __CFStringMakeConstantString("" cStr "")
CFStringRef  __CFStringMakeConstantString(const char *cStr);

#ifdef __cplusplus
}
#endif

#endif
