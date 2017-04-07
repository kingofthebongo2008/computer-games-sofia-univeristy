// C++ for the Windows Runtime v1.0.170303.6
// Copyright (c) 2017 Microsoft Corporation. All rights reserved.

#pragma once

#include "base.h"
WINRT_WARNING_PUSH

#include "internal/Windows.Management.Orchestration.3.h"
#include "Windows.Foundation.h"

WINRT_EXPORT namespace winrt {

namespace impl {

template <typename D>
struct produce<D, Windows::Management::Orchestration::ICurrentAppOrchestration> : produce_base<D, Windows::Management::Orchestration::ICurrentAppOrchestration>
{
    HRESULT __stdcall abi_StartSingleAppMode(impl::abi_arg_out<Windows::Management::Orchestration::ISingleAppModeContext> result) noexcept override
    {
        try
        {
            typename D::abi_guard guard(this->shim());
            *result = detach_abi(this->shim().StartSingleAppMode());
            return S_OK;
        }
        catch (...)
        {
            *result = nullptr;
            return impl::to_hresult();
        }
    }
};

template <typename D>
struct produce<D, Windows::Management::Orchestration::ICurrentAppOrchestrationStatics> : produce_base<D, Windows::Management::Orchestration::ICurrentAppOrchestrationStatics>
{
    HRESULT __stdcall abi_GetForCurrentView(impl::abi_arg_out<Windows::Management::Orchestration::ICurrentAppOrchestration> result) noexcept override
    {
        try
        {
            typename D::abi_guard guard(this->shim());
            *result = detach_abi(this->shim().GetForCurrentView());
            return S_OK;
        }
        catch (...)
        {
            *result = nullptr;
            return impl::to_hresult();
        }
    }
};

template <typename D>
struct produce<D, Windows::Management::Orchestration::ISingleAppModeContext> : produce_base<D, Windows::Management::Orchestration::ISingleAppModeContext>
{};

}

namespace Windows::Management::Orchestration {

template <typename D> Windows::Management::Orchestration::CurrentAppOrchestration impl_ICurrentAppOrchestrationStatics<D>::GetForCurrentView() const
{
    Windows::Management::Orchestration::CurrentAppOrchestration result { nullptr };
    check_hresult(WINRT_SHIM(ICurrentAppOrchestrationStatics)->abi_GetForCurrentView(put_abi(result)));
    return result;
}

template <typename D> Windows::Management::Orchestration::SingleAppModeContext impl_ICurrentAppOrchestration<D>::StartSingleAppMode() const
{
    Windows::Management::Orchestration::SingleAppModeContext result { nullptr };
    check_hresult(WINRT_SHIM(ICurrentAppOrchestration)->abi_StartSingleAppMode(put_abi(result)));
    return result;
}

inline Windows::Management::Orchestration::CurrentAppOrchestration CurrentAppOrchestration::GetForCurrentView()
{
    return get_activation_factory<CurrentAppOrchestration, ICurrentAppOrchestrationStatics>().GetForCurrentView();
}

}

}

template<>
struct std::hash<winrt::Windows::Management::Orchestration::ICurrentAppOrchestration>
{
    size_t operator()(const winrt::Windows::Management::Orchestration::ICurrentAppOrchestration & value) const noexcept
    {
        return winrt::impl::hash_unknown(value);
    }
};

template<>
struct std::hash<winrt::Windows::Management::Orchestration::ICurrentAppOrchestrationStatics>
{
    size_t operator()(const winrt::Windows::Management::Orchestration::ICurrentAppOrchestrationStatics & value) const noexcept
    {
        return winrt::impl::hash_unknown(value);
    }
};

template<>
struct std::hash<winrt::Windows::Management::Orchestration::ISingleAppModeContext>
{
    size_t operator()(const winrt::Windows::Management::Orchestration::ISingleAppModeContext & value) const noexcept
    {
        return winrt::impl::hash_unknown(value);
    }
};

template<>
struct std::hash<winrt::Windows::Management::Orchestration::CurrentAppOrchestration>
{
    size_t operator()(const winrt::Windows::Management::Orchestration::CurrentAppOrchestration & value) const noexcept
    {
        return winrt::impl::hash_unknown(value);
    }
};

template<>
struct std::hash<winrt::Windows::Management::Orchestration::SingleAppModeContext>
{
    size_t operator()(const winrt::Windows::Management::Orchestration::SingleAppModeContext & value) const noexcept
    {
        return winrt::impl::hash_unknown(value);
    }
};

WINRT_WARNING_POP
