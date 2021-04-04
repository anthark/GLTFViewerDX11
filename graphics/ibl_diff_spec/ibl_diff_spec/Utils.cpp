#include "pch.h"

#include <fstream>

#include "Utils.h"

HRESULT ReadCompiledShader(const WCHAR* szFileName, std::vector<BYTE>& bytes)
{
    std::ifstream csoFile(szFileName, std::ios::in | std::ios::binary | std::ios::ate);

    if (csoFile.is_open())
    {
        size_t bufferSize = static_cast<size_t>(csoFile.tellg());
        bytes.clear();
        bytes.resize(bufferSize);
        csoFile.seekg(0, std::ios::beg);
        csoFile.read(reinterpret_cast<char*>(bytes.data()), bufferSize);
        csoFile.close();

        return S_OK;
    }
    return HRESULT_FROM_WIN32(GetLastError());
}

HRESULT CreateVertexShader(ID3D11Device* device, const WCHAR* szFileName, std::vector<BYTE>& bytes, ID3D11VertexShader** vertexShader)
{
    HRESULT hr = S_OK;

    // Read the vertex shader
    hr = ReadCompiledShader(szFileName, bytes);
    if (FAILED(hr))
        return hr;

    // Create the vertex shader
    hr = device->CreateVertexShader(bytes.data(), bytes.size(), nullptr, vertexShader);
    
    return hr;
}

HRESULT CreatePixelShader(ID3D11Device* device, const WCHAR* szFileName, std::vector<BYTE>& bytes, ID3D11PixelShader** pixelShader)
{
    HRESULT hr = S_OK;

    // Read the pixel shader
    hr = ReadCompiledShader(szFileName, bytes);
    if (FAILED(hr))
        return hr;

    // Create the pixel shader
    hr = device->CreatePixelShader(bytes.data(), bytes.size(), nullptr, pixelShader);

    return hr;
}
