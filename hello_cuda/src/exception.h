#include <exception>

struct exception : public std::exception
{
    exception(HRESULT h) : m_h(h)
    {

    }

    HRESULT m_h;
};

inline void ThrowIfFailed(HRESULT hr)
{
    if (hr != S_OK)
    {
        throw exception(hr);
    }
}