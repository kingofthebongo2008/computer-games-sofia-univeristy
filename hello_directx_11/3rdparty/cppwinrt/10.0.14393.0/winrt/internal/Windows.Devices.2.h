// C++ for the Windows Runtime v1.0.170303.6
// Copyright (c) 2017 Microsoft Corporation. All rights reserved.

#pragma once

#include "Windows.Devices.1.h"

WINRT_EXPORT namespace winrt {

namespace Windows::Devices {

struct ILowLevelDevicesAggregateProvider :
    Windows::Foundation::IInspectable,
    impl::consume<ILowLevelDevicesAggregateProvider>
{
    ILowLevelDevicesAggregateProvider(std::nullptr_t = nullptr) noexcept {}
};

struct ILowLevelDevicesAggregateProviderFactory :
    Windows::Foundation::IInspectable,
    impl::consume<ILowLevelDevicesAggregateProviderFactory>
{
    ILowLevelDevicesAggregateProviderFactory(std::nullptr_t = nullptr) noexcept {}
};

struct ILowLevelDevicesController :
    Windows::Foundation::IInspectable,
    impl::consume<ILowLevelDevicesController>
{
    ILowLevelDevicesController(std::nullptr_t = nullptr) noexcept {}
};

struct ILowLevelDevicesControllerStatics :
    Windows::Foundation::IInspectable,
    impl::consume<ILowLevelDevicesControllerStatics>
{
    ILowLevelDevicesControllerStatics(std::nullptr_t = nullptr) noexcept {}
};

}

}
