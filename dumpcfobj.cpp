#include <iostream>
#include "strcnv.h"
#include "cfhelper.h"

using namespace std;

struct DumpContext {
    DumpContext(ostream &stream, int indent_level)
	:os(stream), indent(indent_level) {}
    DumpContext next() { return DumpContext(os, indent + 4); }

    ostream &os;
    int indent;
};

static void dump_value(CFTypeRef value, DumpContext dc);
static void dump_array(CFArrayRef vec, DumpContext dc);

static
void dict_callback(const void *key, const void *value, void *ctx)
{
    DumpContext *dc = reinterpret_cast<DumpContext*>(ctx);
    string pad(dc->indent, ' ');
    CFTypeID keyID = CFGetTypeID(key);
    if (keyID == CFStringGetTypeID()) {
	wstring k = CF2W(reinterpret_cast<CFStringRef>(key));
	dc->os << format("%s%[%d]%ls: ", pad.c_str(),
			 CFGetRetainCount(key), k.c_str());
	dump_value(value, *dc);
    }
}

static
void dump_value(CFTypeRef value, DumpContext dc)
{
    CFTypeID valueID = CFGetTypeID(value);
    CFIndex nref = CFGetRetainCount(value);
    dc.os << format("[%d]", nref);
    if (valueID == CFStringGetTypeID()) {
	wstring v = CF2W(reinterpret_cast<CFStringRef>(value));
	dc.os << w2m(v) << endl;
    } else if (valueID == CFDictionaryGetTypeID()) {
	dc.os << "Dictionary" << endl;
	DumpContext ndc = dc.next();
	CFDictionaryApplyFunction(reinterpret_cast<CFDictionaryRef>(value),
		dict_callback, &ndc);
    } else if (valueID == CFArrayGetTypeID()) {
	dc.os << "Array" << endl;
	dump_array(reinterpret_cast<CFArrayRef>(value), dc.next());
    } else if (valueID == CFNumberGetTypeID()) {
	CFNumberRef ref = reinterpret_cast<CFNumberRef>(value);
	if (CFNumberIsFloatType(ref)) {
	    double x;
	    CFNumberGetValue(ref, kCFNumberDoubleType, &x);
	    dc.os << format("%g", x) << endl;
	} else {
	    int n;
	    CFNumberGetValue(ref, kCFNumberSInt32Type, &n);
	    dc.os << n << endl;
	}
    } else {
	dc.os << format("typeid[%d]\n", valueID) << endl;
    }
}

static
void dump_array(CFArrayRef vec, DumpContext dc)
{
    string pad(dc.indent, ' ');
    CFIndex count = CFArrayGetCount(vec);
    for (CFIndex i = 0; i < count; ++i) {
	CFTypeRef value = CFArrayGetValueAtIndex(vec, i);
	dc.os << format("%s%d: ", pad.c_str(), i);
	dump_value(value, dc);
    }
}

void dump_object(CFTypeRef ref, ostream &os)
{
    DumpContext dc(os, 0);
    dump_value(ref, dc);
}
