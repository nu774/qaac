#include "win32util.h"
#include "util.h"

void throw_win32_error(const std::string &msg, DWORD code)
{
    LPSTR pszMsg;
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
		   0,
		   code,
		   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		   (LPSTR)&pszMsg,
		   0,
		   0);
    squeeze(pszMsg, "\r\n");
    std::runtime_error e(format("%s: %s", msg.c_str(), pszMsg));
    LocalFree(pszMsg);
    throw e;
}
