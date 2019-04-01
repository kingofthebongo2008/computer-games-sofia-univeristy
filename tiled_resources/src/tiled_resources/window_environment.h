#pragma once

#include <dxgi.h>

namespace sample
{
    struct window_environment
    {
        // Cached device properties.
        winrt::Windows::Foundation::Size                            m_back_buffer_size;     //back buffer
        winrt::Windows::Foundation::Size                            m_output_size;          //window size in pixels
        winrt::Windows::Foundation::Size                            m_logical_size;         //window logical size in dips
        winrt::Windows::Graphics::Display::DisplayOrientations      m_native_orientation;   //native screen orientation
        winrt::Windows::Graphics::Display::DisplayOrientations		m_current_orientation;  //current orientation from the user
        float														m_dpi;                  //dpi from the display

        // This is the DPI that will be reported back to the app. It takes into account whether the app supports high resolution screens or not.
        float														   m_effective_dpi;        //effective dpi, affected by scaling

        winrt::Windows::UI::Core::CoreWindow						   m_window		= nullptr;
    };

    inline DXGI_MODE_ROTATION compute_display_rotation(winrt::Windows::Graphics::Display::DisplayOrientations native_orientation, winrt::Windows::Graphics::Display::DisplayOrientations current_orientation)
    {
        using namespace winrt::Windows::Graphics::Display;

        DXGI_MODE_ROTATION rotation = DXGI_MODE_ROTATION_UNSPECIFIED;

        // Note: NativeOrientation can only be Landscape or Portrait even though
        // the DisplayOrientations enum has other values.
        switch (native_orientation)
        {
        case DisplayOrientations::Landscape:
            switch (current_orientation)
            {
            case DisplayOrientations::Landscape:
                rotation = DXGI_MODE_ROTATION_IDENTITY;
                break;

            case DisplayOrientations::Portrait:
                rotation = DXGI_MODE_ROTATION_ROTATE270;
                break;

            case DisplayOrientations::LandscapeFlipped:
                rotation = DXGI_MODE_ROTATION_ROTATE180;
                break;

            case DisplayOrientations::PortraitFlipped:
                rotation = DXGI_MODE_ROTATION_ROTATE90;
                break;
            }
            break;

        case DisplayOrientations::Portrait:
            switch (current_orientation)
            {
            case DisplayOrientations::Landscape:
                rotation = DXGI_MODE_ROTATION_ROTATE90;
                break;

            case DisplayOrientations::Portrait:
                rotation = DXGI_MODE_ROTATION_IDENTITY;
                break;

            case DisplayOrientations::LandscapeFlipped:
                rotation = DXGI_MODE_ROTATION_ROTATE270;
                break;

            case DisplayOrientations::PortraitFlipped:
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

    inline window_environment build_environment(const winrt::Windows::UI::Core::CoreWindow& window, const winrt::Windows::Graphics::Display::DisplayInformation& currentDisplayInformation )
    {
        using namespace winrt::Windows::Graphics::Display;

        window_environment result = {};

        winrt::Windows::Foundation::Rect bounds = window.Bounds();
        result.m_logical_size.Width  = bounds.Width;
        result.m_logical_size.Height = bounds.Height;

        float f;

        result.m_native_orientation = currentDisplayInformation.NativeOrientation();
        result.m_current_orientation = currentDisplayInformation.CurrentOrientation();
        result.m_dpi = currentDisplayInformation.LogicalDpi();

        f = currentDisplayInformation.LogicalDpi();

        result.m_effective_dpi = result.m_dpi;     //no scaling for now, scaling is used for phones to save power.
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

        result.m_window = window;

        return result;
    }
}