#include "configfile.h"
#include "strutil.h"
#include "win32util.h"
#include <cctype>
#include <shlobj.h>

std::shared_ptr<FILE> openConfigFile(const wchar_t *file)
{
    std::vector<std::wstring> search_paths;
    const wchar_t *home = _wgetenv(L"HOME");
    if (home)
        search_paths.push_back(strutil::format(L"%s\\%s", home, L".qaac"));
    wchar_t path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(0, CSIDL_APPDATA, 0, 0, path)))
        search_paths.push_back(strutil::format(L"%s\\%s", path, L"qaac"));
    search_paths.push_back(win32::get_module_directory());
    for (size_t i = 0; i < search_paths.size(); ++i) {
        try {
            std::wstring pathtry =
                strutil::format(L"%s\\%s", search_paths[i].c_str(), file);
            return win32::fopen(pathtry, L"r");
        } catch (...) {
            if (i == search_paths.size() - 1) throw;
        }
    }
    return 0;
}

static
std::vector<std::vector<complex_t>>
loadRemixerMatrix(std::shared_ptr<FILE> fileptr)
{
    FILE *fp = fileptr.get();
    int c;
    std::vector<std::vector<complex_t> > matrix;
    std::vector<complex_t> row;
    while ((c = std::getc(fp)) != EOF) {
        if (c == '\n') {
            if (row.size()) {
                matrix.push_back(row);
                row.clear();
            }
        } else if (std::isspace(c)) {
            while (c != '\n' && std::isspace(c = std::getc(fp)))
                ;
            std::ungetc(c, fp);
        } else if (std::isdigit(c) || c == '-') {
            std::ungetc(c, fp);
            double v;
            if (std::fscanf(fp, "%lf", &v) != 1)
                throw std::runtime_error("invalid matrix preset file");
            c = std::getc(fp);
            if (std::strchr("iIjJ", c))
                row.push_back(complex_t(0.0, v));
            else if (std::strchr("kK", c))
                row.push_back(complex_t(0.0, -v));
            else {
                std::ungetc(c, fp);
                row.push_back(complex_t(v, 0.0));
            }
        } else
            throw std::runtime_error("invalid char in matrix preset file");
    }
    if (row.size())
        matrix.push_back(row);
    return matrix;
}

std::vector<std::vector<complex_t>>
loadRemixerMatrixFromFile(const wchar_t *path)
{
    return loadRemixerMatrix(win32::fopen(path, L"r"));
}

std::vector<std::vector<complex_t>>
loadRemixerMatrixFromPreset(const wchar_t *preset_name)
{
    std::wstring path = strutil::format(L"matrix\\%s.txt", preset_name);
    return loadRemixerMatrix(openConfigFile(path.c_str()));
}

