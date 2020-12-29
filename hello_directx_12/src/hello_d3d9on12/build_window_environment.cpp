#pragma once
#include "pch.h"
#include "build_window_environment.h"
#include <ShellScalingApi.h>


namespace sample
{
    inline DXGI_MODE_ROTATION compute_display_rotation(window_environment::DisplayOrientations native_orientation, window_environment::DisplayOrientations current_orientation)
    {
        DXGI_MODE_ROTATION rotation = DXGI_MODE_ROTATION_UNSPECIFIED;

        // Note: NativeOrientation can only be Landscape or Portrait even though
        // the DisplayOrientations enum has other values.
        switch (native_orientation)
        {
        case window_environment::DisplayOrientations::Landscape:
            switch (current_orientation)
            {
            case window_environment::DisplayOrientations::Landscape:
                rotation = DXGI_MODE_ROTATION_IDENTITY;
                break;

            case window_environment::DisplayOrientations::Portrait:
                rotation = DXGI_MODE_ROTATION_ROTATE270;
                break;

            case window_environment::DisplayOrientations::LandscapeFlipped:
                rotation = DXGI_MODE_ROTATION_ROTATE180;
                break;

            case window_environment::DisplayOrientations::PortraitFlipped:
                rotation = DXGI_MODE_ROTATION_ROTATE90;
                break;
            }
            break;

        case window_environment::DisplayOrientations::Portrait:
            switch (current_orientation)
            {
            case window_environment::DisplayOrientations::Landscape:
                rotation = DXGI_MODE_ROTATION_ROTATE90;
                break;

            case window_environment::DisplayOrientations::Portrait:
                rotation = DXGI_MODE_ROTATION_IDENTITY;
                break;

            case window_environment::DisplayOrientations::LandscapeFlipped:
                rotation = DXGI_MODE_ROTATION_ROTATE270;
                break;

            case window_environment::DisplayOrientations::PortraitFlipped:
                rotation = DXGI_MODE_ROTATION_ROTATE180;
                break;
            }
            break;
        }
        return rotation;
    }

    // Converts a length in device-independent pixels (DIPs) to a length in physical pixels.
    inline float convert_dips_to_pixels(float dips, float dpi)
    {
        static const float dipsPerInch = 96.0f;
        return floorf( dips * dpi / dipsPerInch + 0.5f ); // Round to nearest integer.
    }

    window_environment build_environment(const HWND window)
    {
        window_environment result = {};

        RECT rect;
        GetWindowRect(window, &rect);

        auto monitor = MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);

        UINT dpix;
        UINT dpiy;
        
        if (GetDpiForMonitor(monitor, MONITOR_DPI_TYPE::MDT_EFFECTIVE_DPI, &dpix, &dpiy)!= S_OK)
        {
            dpix = 96;
            dpiy = 96;
        }

        auto windowDpi = static_cast<float>(dpix);

        const uint32_t dpi = dpix;
        
        result.m_logical_size.Width  = static_cast<float>(rect.right  - rect.left) / (dpi / 96.0F);
        result.m_logical_size.Height = static_cast<float>(rect.bottom - rect.top) / (dpi / 96.0F);

        float f;

        result.m_dpi = dpi;


        result.m_effective_dpi              = result.m_dpi;		// no scaling for now, scaling is used for phones to save power.

        // Calculate the necessary render target size in pixels.
        result.m_output_size.Width          = convert_dips_to_pixels(result.m_logical_size.Width, result.m_effective_dpi);
        result.m_output_size.Height         = convert_dips_to_pixels(result.m_logical_size.Height, result.m_effective_dpi);

        // Prevent small sizes DirectX content from being created.
        result.m_output_size.Width          = result.m_output_size.Width  < 8 ? 8 : result.m_output_size.Width;
        result.m_output_size.Height         = result.m_output_size.Height < 8 ? 8 : result.m_output_size.Height;

        auto display_rotation               = compute_display_rotation(result.m_native_orientation, result.m_current_orientation);
        bool swap_dimensions                = display_rotation == DXGI_MODE_ROTATION_ROTATE90 || display_rotation == DXGI_MODE_ROTATION_ROTATE270;

        result.m_back_buffer_size.Width     = swap_dimensions ? result.m_output_size.Height : result.m_output_size.Width;
        result.m_back_buffer_size.Height    = swap_dimensions ? result.m_output_size.Width  : result.m_output_size.Height;

        return result;
    }
}