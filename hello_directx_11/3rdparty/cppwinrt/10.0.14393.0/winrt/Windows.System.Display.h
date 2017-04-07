// C++ for the Windows Runtime v1.0.170303.6
// Copyright (c) 2017 Microsoft Corporation. All rights reserved.

#pragma once

#include "base.h"
WINRT_WARNING_PUSH

#include "internal/Windows.System.Display.3.h"
#include "Windows.System.h"

WINRT_EXPORT namespace winrt {

namespace impl {

template <typename D>
struct produce<D, Windows::System::Display::IDisplayRequest> : produce_base<D, Windows::System::Display::IDisplayRequest>
{
    HRESULT __stdcall abi_RequestActive() noexcept override
    {
        try
        {
            typename D::abi_guard guard(this->shim());
            this->shim().RequestActive();
            return S_OK;
        }
        catch (...)
        {
            return impl::to_hresult();
        }
    }

    HRESULT __stdcall abi_RequestRelease() noexcept override
    {
        try
        {
            typename D::abi_guard guard(this->shim());
            this->shim().RequestRelease();
            return S_OK;
        }
        catch (...)
        {
            return impl::to_hresult();
        }
    }
};

}

namespace Windows::System::Display {

template <typename D> void impl_IDisplayRequest<D>::RequestActive() const
{
    check_hresult(WINRT_SHIM(IDisplayRequest)->abi_RequestActive());
}

template <typename D> void impl_IDisplayRequest<D>::RequestRelease() const
{
    check_hresult(WINRT_SHIM(IDisplayRequest)->abi_RequestRelease());
}

inline DisplayRequest::DisplayRequest() :
    DisplayRequest(activate_instance<DisplayRequest>())
{}

}

}

template<>
struct std::hash<winrt::Windows::System::Display::IDisplayRequest>
{
    size_t operator()(const winrt::Windows::System::Display::IDisplayRequest & value) const noexcept
    {
        return winrt::impl::hash_unknown(value);
    }
};

template<>
struct std::hash<winrt::Windows::System::Display::DisplayRequest>
{
    size_t operator()(const winrt::Windows::System::Display::DisplayRequest & value) const noexcept
    {
        return winrt::impl::hash_unknown(value);
    }
};

WINRT_WARNING_POP
