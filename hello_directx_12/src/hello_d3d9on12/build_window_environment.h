#pragma once

#include "window_environment.h"

namespace sample
{
    //Transfer data from cppwinrt to native.
    //Information about the swap chain and windows orientation and sizes
	window_environment build_environment(const HWND window);
}