#pragma once

#ifndef UNICODE
#define UNICODE
#endif

#ifdef _WIN64
#define ENV64
#endif

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <wrl/client.h>

#include <d3d11.h>
#include <d3d11_1.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#include <directxmath.h>

#include <string>
#include <memory>
