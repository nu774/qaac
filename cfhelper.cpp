#include <iostream>
#include "strcnv.h"
#include "cfhelper.h"

using namespace std;

CFDictionaryRef
SearchCFDictArray(CFArrayRef vec, CFStringRef key, CFStringRef value)
{
    if (CFGetTypeID(vec) != CFArrayGetTypeID())
	return 0;
    CFIndex length = CFArrayGetCount(vec);
    for (CFIndex i = 0; i < length; ++i) {
	CFTypeRef item = CFArrayGetValueAtIndex(vec, i);
	if (CFGetTypeID(item) != CFDictionaryGetTypeID())
	    continue;
	CFDictionaryRef dict = reinterpret_cast<CFDictionaryRef>(item);
	CFTypeRef v = CFDictionaryGetValue(dict, key);
	if (CFGetTypeID(v) != CFStringGetTypeID())
	    continue;
	if (!CFStringCompare(value, reinterpret_cast<CFStringRef>(v), 0))
	    return dict;
    }
    return 0;
}
