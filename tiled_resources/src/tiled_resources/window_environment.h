#pragma once

namespace sample
{
    struct window_environment
    {
		struct Size
		{
			float Width;
			float Height;

			Size() noexcept = default;

			constexpr Size(float Width, float Height) noexcept
				: Width(Width), Height(Height)
			{}
		};

		enum class DisplayOrientations : uint32_t
		{
			None = 0,
			Landscape = 0x1,
			Portrait = 0x2,
			LandscapeFlipped = 0x4,
			PortraitFlipped = 0x8,
		};


        // Cached device properties.
        Size                            m_back_buffer_size;     //back buffer
        Size                            m_output_size;          //window size in pixels
        Size                            m_logical_size;         //window logical size in dips
        DisplayOrientations			    m_native_orientation;   //native screen orientation
        DisplayOrientations				m_current_orientation;  //current orientation from the user
        float							m_dpi;                  //dpi from the display

        // This is the DPI that will be reported back to the app. It takes into account whether the app supports high resolution screens or not.
        float														m_effective_dpi;        //effective dpi, affected by scaling
    };
}