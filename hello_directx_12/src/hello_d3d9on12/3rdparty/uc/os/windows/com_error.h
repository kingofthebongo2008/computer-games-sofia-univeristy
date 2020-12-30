#pragma once

#include <exception>
#include <ole2.h>


namespace uc
{
    namespace os
    {
        namespace windows
        {
            class com_exception : public std::exception
            {
            public:
                com_exception(const HRESULT hr) : exception("com exception")
                    , m_hr(hr)
                {

                }

            private:
                const HRESULT m_hr;
                com_exception& operator=(const com_exception&);
            };

            class win32_exception : public std::exception
            {
            public:
                win32_exception() : exception("com exception")
                {

                }
            };

            template < typename exception > void throw_if_failed(HRESULT hr)
            {
                if (hr != S_OK)
                {
                    throw exception(hr);
                }
            }

            template < typename exception > void throw_if_failed(BOOL result)
            {
                if (!result)
                {
                    throw exception();
                }
            }
        }
    }
}

