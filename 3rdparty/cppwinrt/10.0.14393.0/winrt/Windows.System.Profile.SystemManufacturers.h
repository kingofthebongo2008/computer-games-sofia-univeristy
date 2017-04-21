// C++ for the Windows Runtime v1.0.170303.6
// Copyright (c) 2017 Microsoft Corporation. All rights reserved.

#pragma once

#include "base.h"
WINRT_WARNING_PUSH

#include "internal/Windows.System.Profile.SystemManufacturers.3.h"
#include "Windows.System.Profile.h"

WINRT_EXPORT namespace winrt {

namespace impl {

template <typename D>
struct produce<D, Windows::System::Profile::SystemManufacturers::ISmbiosInformationStatics> : produce_base<D, Windows::System::Profile::SystemManufacturers::ISmbiosInformationStatics>
{
    HRESULT __stdcall get_SerialNumber(impl::abi_arg_out<hstring> value) noexcept override
    {
        try
        {
            typename D::abi_guard guard(this->shim());
            *value = detach_abi(this->shim().SerialNumber());
            return S_OK;
        }
        catch (...)
        {
            *value = nullptr;
            return impl::to_hresult();
        }
    }
};

}

namespace Windows::System::Profile::SystemManufacturers {

template <typename D> hstring impl_ISmbiosInformationStatics<D>::SerialNumber() const
{
    hstring value;
    check_hresult(WINRT_SHIM(ISmbiosInformationStatics)->get_SerialNumber(put_abi(value)));
    return value;
}

inline hstring SmbiosInformation::SerialNumber()
{
    return get_activation_factory<SmbiosInformation, ISmbiosInformationStatics>().SerialNumber();
}

}

}

template<>
struct std::hash<winrt::Windows::System::Profile::SystemManufacturers::ISmbiosInformationStatics>
{
    size_t operator()(const winrt::Windows::System::Profile::SystemManufacturers::ISmbiosInformationStatics & value) const noexcept
    {
        return winrt::impl::hash_unknown(value);
    }
};

WINRT_WARNING_POP
