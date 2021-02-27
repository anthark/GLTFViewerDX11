#include "pch.h"

#include <fstream>

HRESULT ReadCompiledShader(const WCHAR* szFileName, BYTE** bytes, size_t& bufferSize)
{
    std::ifstream csoFile(szFileName, std::ios::in | std::ios::binary | std::ios::ate);

    if (csoFile.is_open())
    {
        bufferSize = (size_t)csoFile.tellg();
        *bytes = new BYTE[bufferSize];

        csoFile.seekg(0, std::ios::beg);
        csoFile.read(reinterpret_cast<char*>(*bytes), bufferSize);
        csoFile.close();

        return S_OK;
    }
    return HRESULT_FROM_WIN32(GetLastError());
}
