#ifndef TEXTFILE_H
#define TEXTFILE_H

#include <string>

std::wstring load_text_file(const std::wstring &path, uint32_t codepage=0);
#endif
