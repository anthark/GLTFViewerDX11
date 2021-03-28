#pragma once

#include <vector>

HRESULT CreateVertexShader(ID3D11Device* device, const WCHAR* szFileName, std::vector<BYTE>& bytes, ID3D11VertexShader** vertexShader);

HRESULT CreatePixelShader(ID3D11Device* device, const WCHAR* szFileName, std::vector<BYTE>& bytes, ID3D11PixelShader** pixelShader);
