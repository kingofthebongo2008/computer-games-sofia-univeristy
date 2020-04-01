#include "pch.h"
#include "exception.h"
#include "view_provider.h"

int32_t __stdcall wWinMain( HINSTANCE, HINSTANCE,PWSTR, int32_t )
{
    using namespace winrt::Windows::ApplicationModel::Core;

    ThrowIfFailed(CoInitializeEx(nullptr, COINIT_MULTITHREADED));
    
    CoreApplication::Run(ViewProvider());

    CoUninitialize();
    return 0;
}
