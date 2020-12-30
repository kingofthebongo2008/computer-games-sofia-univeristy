#pragma once

#include "com_error.h"

#include <uc/util/noncopyable.h>

namespace uc
{
    namespace os
    {
        namespace windows
        {
            class com_initializer : private util::noncopyable
            {
            public:
                com_initializer()
                {
                    HRESULT hr = ::CoInitializeEx(nullptr, COINIT_MULTITHREADED);

                    bool success = (hr == S_OK || hr == S_FALSE);

                    if (!success)
                    {
                        throw com_exception(hr);
                    }
                }


                ~com_initializer()
                {
                    ::CoUninitialize();
                }

            };
        }
    }
}



