#include "pch.h"
#include "view_provider.h"
#include "error.h"

#include <concrt.h>

int32_t __stdcall wWinMain( HINSTANCE, HINSTANCE,PWSTR, int32_t )
{
    //sample::ThrowIfFailed(CoInitializeEx(nullptr, COINIT_MULTITHREADED));
	CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	auto v = winrt::make<ViewProvider>();
    CoreApplication::Run(v.as<IFrameworkViewSource>());
    CoUninitialize();
    return 0;
}
