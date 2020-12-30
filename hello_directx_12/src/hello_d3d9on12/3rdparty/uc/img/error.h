#pragma once

#include <exception>

namespace uc
{
    namespace gx
    {
        namespace imaging
        {
            class exception : public std::exception
            {
                using base = std::exception;

                public:
                exception(const char* c) : base(c)
                {

                }
            };

            class com_exception : public exception
            {
                using base = exception;
                
                public:

                com_exception(const HRESULT hr) : base("com exception")
                    , m_hr(hr)
                {

                }

                private:
                HRESULT m_hr;
            };

            inline void throw_if_failed( HRESULT hr )
            {
                if (FAILED(hr))
                {
                    throw com_exception(hr);
                }
            }
        }
    }
}

