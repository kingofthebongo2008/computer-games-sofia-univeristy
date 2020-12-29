//
// pch.h
// Header for standard system include files.
//

#pragma once

#define NOMINMAX                        // Exclude windows header macro

#include <SDKDDKVer.h>

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#include <windows.h>
#include <mutex>
#include <wrl.h>

#include <d3d12.h>
#include <dxgi1_5.h>
#include <d3d9.h>
#include <d3d9on12.h>

#if WINAPI_FAMILY_DESKTOP_APP == WINAPI_FAMILY
#include <atlbase.h>
#include <atlwin.h>
#endif








