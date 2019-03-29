//
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

#include <wrl/client.h>

#include <d3d12.h>
#include <dxgi1_5.h>


#include <winrt/base.h>
#include <winrt/Windows.UI.Core.h>
#include <winrt/Windows.ApplicationModel.Core.h>
#include <winrt/Windows.ApplicationModel.Activation.h>
#include <winrt/Windows.UI.ViewManagement.h>




