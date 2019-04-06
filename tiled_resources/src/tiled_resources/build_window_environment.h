#pragma once

#include "window_environment.h"

#include <winrt/Windows.UI.Core.h>
#include <winrt/Windows.Graphics.Display.h>

namespace sample
{
    //Transfer data from cppwinrt to native.
    //Information about the swap chain and windows orientation and sizes
	window_environment build_environment(const winrt::Windows::UI::Core::CoreWindow& window, const winrt::Windows::Graphics::Display::DisplayInformation& currentDisplayInformation);
}