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
	    CFDictionaryCreateMutableCopy(0, 0, dictRef);
	std::vector<CFTypeRef> keys(CFDictionaryGetCount(dict));
	CFDictionaryGetKeysAndValues(dict, &keys[0], 0);
	for (size_t i = 0; i < keys.size(); ++i) {
	    CFDictionaryReplaceValue(dict, keys[i], 
		    CloneCFObject(CFDictionaryGetValue(dict, keys[i])));
	}
	return dict;
    } else if (typeID == CFArrayGetTypeID()) {
	CFArrayRef arrayRef = reinterpret_cast<CFArrayRef>(value);
	CFMutableArrayRef array = CFArrayCreateMutableCopy(0, 0, arrayRef);
	CFIndex count = CFArrayGetCount(array);
	for (CFIndex i = 0; i < count; ++i) {
	    CFArraySetValueAtIndex(array, i, 
		    CloneCFObject(CFArrayGetValueAtIndex(array, i)));
	}
	return array;
    } else {
	return CFRetain(value);
    }
}
