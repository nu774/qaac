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

CFTypeRef CloneCFObject(CFTypeRef value)
{
    CFTypeID typeID = CFGetTypeID(value);
    if (typeID == CFDictionaryGetTypeID()) {
	CFDictionaryRef dictRef = reinterpret_cast<CFDictionaryRef>(value);
	CFMutableDictionaryRef dict =
	    CFDictionaryCreateMutable(0, 0, &kCFTypeDictionaryKeyCallBacks,
			&kCFTypeDictionaryValueCallBacks);
	CFIndex count = CFDictionaryGetCount(dictRef);
	std::vector<CFTypeRef> keys(count), values(count);
	CFDictionaryGetKeysAndValues(dictRef, &keys[0], &values[0]);
	for (CFIndex i = 0; i < count; ++i) {
	    CFTypeRef child = CloneCFObject(values[i]);
	    CFDictionarySetValue(dict, keys[i], child);
	    CFRelease(child);
	}
	return dict;
    } else if (typeID == CFArrayGetTypeID()) {
	CFArrayRef arrayRef = reinterpret_cast<CFArrayRef>(value);
	CFIndex count = CFArrayGetCount(arrayRef);
	CFMutableArrayRef array =
	    CFArrayCreateMutable(0, 0, &kCFTypeArrayCallBacks);
	for (CFIndex i = 0; i < count; ++i) {
	    CFTypeRef child = CloneCFObject(CFArrayGetValueAtIndex(arrayRef, i));
	    CFArrayAppendValue(array, child);
	    CFRelease(child);
	}
	return array;
    } else {
	return CFRetain(value);
    }
}
