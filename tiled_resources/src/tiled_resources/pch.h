﻿//
// pch.h
// Header for standard system include files.
//

#pragma once

#define NOMINMAX                        // Exclude windows header macro

#include <SDKDDKVer.h>

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

// Windows Header Files:
// cppwinrt takes too much time to compile, so it is good to precompile it
#include <windows.h>
#include <cstdint>

#include <Unknwn.h>
#include <winrt/base.h>








