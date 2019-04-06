//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved

#pragma once

#include <ppltasks.h> // For create_task

namespace DX
{
    inline void ThrowIfFailed(HRESULT hr)
    {
        if (FAILED(hr))
        {
            // Set a breakpoint on this line to catch Win32 API errors.
            std::wcout << L"FAILED HRESULT (0x" << std::hex << hr << L")" << std::endl;
            RaiseException(hr, 0, 0, NULL);
            exit(hr);
        }
    }

    // Function that reads from a binary file asynchronously.
    inline Concurrency::task<std::vector<byte>> ReadDataAsync(const std::wstring& filename)
    {
        using namespace Concurrency;

        HANDLE file = CreateFile2(
            filename.c_str(),
            GENERIC_READ,
            FILE_SHARE_READ,
            OPEN_EXISTING,
            NULL
            );

        DX::ThrowIfFailed(file != INVALID_HANDLE_VALUE ? S_OK : HRESULT_FROM_WIN32(GetLastError()));

        return create_task([file](){
            BY_HANDLE_FILE_INFORMATION info;
            if (!GetFileInformationByHandle(file, &info)) DX::ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));

            std::vector<byte> ret(info.nFileSizeLow);
            DWORD read = 0;
            DX::ThrowIfFailed(ReadFile(file, ret.data(), info.nFileSizeLow, &read, NULL) ? S_OK : HRESULT_FROM_WIN32(GetLastError()));
            DX::ThrowIfFailed(read == info.nFileSizeLow ? S_OK : E_FAIL);

            CloseHandle(file);

            return ret;
        });
    }

    inline std::vector<byte> ReadData(const std::wstring& filename)
    {
        HANDLE file = CreateFile2(
            filename.c_str(),
            GENERIC_READ,
            FILE_SHARE_READ,
            OPEN_EXISTING,
            NULL
            );

        DX::ThrowIfFailed(file != INVALID_HANDLE_VALUE ? S_OK : HRESULT_FROM_WIN32(GetLastError()));

        BY_HANDLE_FILE_INFORMATION info;
        if (!GetFileInformationByHandle(file, &info)) DX::ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));

        std::vector<byte> ret(info.nFileSizeLow);
        DWORD read = 0;
        DX::ThrowIfFailed(ReadFile(file, ret.data(), info.nFileSizeLow, &read, NULL) ? S_OK : HRESULT_FROM_WIN32(GetLastError()));
        DX::ThrowIfFailed(read == info.nFileSizeLow ? S_OK : E_FAIL);

        CloseHandle(file);

        return ret;
    }

#if defined(_DEBUG)
    // Check for SDK Layer support.
    inline bool SdkLayersAvailable()
    {
        HRESULT hr = D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_NULL,       // There is no need to create a real hardware device.
            0,
            D3D11_CREATE_DEVICE_DEBUG,  // Check for the SDK layers.
            nullptr,                    // Any feature level will do.
            0,
            D3D11_SDK_VERSION,          // Always set this to D3D11_SDK_VERSION for Windows Store apps.
            nullptr,                    // No need to keep the D3D device reference.
            nullptr,                    // No need to know the feature level.
            nullptr                     // No need to keep the D3D device context reference.
            );

        return SUCCEEDED(hr);
    }
#endif
}

class HR
{
public:
    HR(HRESULT hr)
    {
        m_hr = hr;
    }
    HRESULT operator=(HRESULT hr)
    {
        m_hr = hr;
        if (FAILED(hr))
        {
            std::wcout << L"FAILED HRESULT (0x" << std::hex << hr << L")" << std::endl;
            RaiseException(hr, 0, 0, NULL);
            exit(hr);
        }
        return hr;
    }
private:
    HRESULT m_hr;
};
