//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved

#include "pch.h"
#include "App.h"

using namespace TiledResources;

using namespace concurrency;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::Devices::Input;
using namespace Windows::UI::Core;
using namespace Windows::UI::Input;
using namespace Windows::System;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;

// The main function is only used to initialize our IFrameworkView class.
[Platform::MTAThread]
int main(Platform::Array<Platform::String^>^)
{
    auto directXApplicationSource = ref new DirectXApplicationSource();
    CoreApplication::Run(directXApplicationSource);
    return 0;
}

IFrameworkView^ DirectXApplicationSource::CreateView()
{
    return ref new App();
}

App::App() :
    m_windowClosed(false),
    m_windowVisible(true),
    m_mouseLookMode(false),
    m_lastPointerPositionValid(false)
{
}

// The first method called when the IFrameworkView is being created.
void App::Initialize(CoreApplicationView^ applicationView)
{
    // Register event handlers for app lifecycle. This example includes Activated, so that we
    // can make the CoreWindow active and start rendering on the window.
    applicationView->Activated +=
        ref new TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>(this, &App::OnActivated);

    CoreApplication::Suspending +=
        ref new EventHandler<SuspendingEventArgs^>(this, &App::OnSuspending);

    CoreApplication::Resuming +=
        ref new EventHandler<Platform::Object^>(this, &App::OnResuming);

    // At this point we have access to the device. 
    // We can create the device-dependent resources.
    m_deviceResources = std::make_shared<DeviceResources>();
}

// Called when the CoreWindow object is created (or re-created).
void App::SetWindow(CoreWindow^ window)
{
    window->SizeChanged += 
        ref new TypedEventHandler<CoreWindow^, WindowSizeChangedEventArgs^>(this, &App::OnWindowSizeChanged);

    window->VisibilityChanged +=
        ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(this, &App::OnVisibilityChanged);

    window->Closed += 
        ref new TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>(this, &App::OnWindowClosed);

    window->Activated +=
        ref new TypedEventHandler<CoreWindow^, WindowActivatedEventArgs^>(this, &App::OnWindowActivated);

    DisplayInformation::GetForCurrentView()->DpiChanged +=
        ref new TypedEventHandler<DisplayInformation^, Platform::Object^>(this, &App::OnDpiChanged);

    DisplayInformation::GetForCurrentView()->OrientationChanged +=
        ref new TypedEventHandler<DisplayInformation^, Platform::Object^>(this, &App::OnOrientationChanged);

    DisplayInformation::DisplayContentsInvalidated +=
        ref new TypedEventHandler<DisplayInformation^, Platform::Object^>(this, &App::OnDisplayContentsInvalidated);

    window->KeyDown +=
        ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(this, &App::OnKeyDown);

    window->KeyUp +=
        ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(this, &App::OnKeyUp);

    window->PointerPressed +=
        ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &App::OnPointerPressed);

    window->PointerMoved +=
        ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &App::OnPointerMoved);

    // Disable all pointer visual feedback for better performance when using touch.
    auto pointerVisualizationSettings = PointerVisualizationSettings::GetForCurrentView();
    pointerVisualizationSettings->IsContactFeedbackEnabled = false; 
    pointerVisualizationSettings->IsBarrelButtonFeedbackEnabled = false;

    m_deviceResources->SetWindow(window);
}

// Initializes scene resources, or loads a previously saved app state.
void App::Load(Platform::String^ entryPoint)
{
    m_main = std::shared_ptr<TiledResourcesMain>(new TiledResourcesMain(m_deviceResources));
    m_deviceResources->RegisterDeviceLostHandler(m_main->shared_from_this());
}

// This method is called after the window becomes active.
void App::Run()
{
    while (!m_windowClosed)
    {
        if (m_windowVisible)
        {
            CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);

            m_main->Update();

            if (m_main->Render())
            {
                m_deviceResources->Present();
            }
        }
        else
        {
            CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessOneAndAllPending);
        }
    }
}

// Required for IFrameworkView.
// Terminate events do not cause Uninitialize to be called. It will be called if your IFrameworkView
// class is torn down while the app is in the foreground.
void App::Uninitialize()
{
}

// Application lifecycle event handlers.

void App::OnActivated(CoreApplicationView^ applicationView, IActivatedEventArgs^ args)
{
    // Run() won't start until the CoreWindow is activated.
    CoreWindow::GetForCurrentThread()->Activate();
}

void App::OnSuspending(Platform::Object^ sender, SuspendingEventArgs^ args)
{
    // Save app state asynchronously after requesting a deferral. Holding a deferral
    // indicates that the application is busy performing suspending operations. Be
    // aware that a deferral may not be held indefinitely. After about five seconds,
    // the app will be forced to exit.
    SuspendingDeferral^ deferral = args->SuspendingOperation->GetDeferral();

    create_task([this, deferral]()
    {
        m_deviceResources->Trim();

        // Insert your code here.

        deferral->Complete();
    });
}

void App::OnResuming(Platform::Object^ sender, Platform::Object^ args)
{
    // Restore any data or state that was unloaded on suspend. By default, data
    // and state are persisted when resuming from suspend. Note that this event
    // does not occur if the app was previously terminated.

    // Insert your code here.
    m_lastPointerPositionValid = false;
}

// Window event handlers.

void App::OnWindowSizeChanged(CoreWindow^ sender, WindowSizeChangedEventArgs^ args)
{
    m_deviceResources->UpdateForWindowSizeChange();
    m_main->UpdateForWindowSizeChange();
}

void App::OnVisibilityChanged(CoreWindow^ sender, VisibilityChangedEventArgs^ args)
{
    m_windowVisible = args->Visible;
    SetMouseLook(false);
    m_lastPointerPositionValid = false;
}

void App::OnWindowClosed(CoreWindow^ sender, CoreWindowEventArgs^ args)
{
    m_windowClosed = true;
}

void App::OnWindowActivated(CoreWindow^ sender, WindowActivatedEventArgs^ args)
{
    SetMouseLook(false);
    m_lastPointerPositionValid = false;
}

// Display properties event handlers.

void App::OnDpiChanged(_In_ DisplayInformation^ sender, _In_ Platform::Object^ args)
{
    m_deviceResources->SetDpi(sender->LogicalDpi);
    m_lastPointerPositionValid = false;
}

void App::OnOrientationChanged(_In_ DisplayInformation^ sender, _In_ Platform::Object^ args)
{
    m_deviceResources->UpdateForWindowSizeChange();
    m_main->UpdateForWindowSizeChange();
    m_lastPointerPositionValid = false;
}

void App::OnDisplayContentsInvalidated(_In_ DisplayInformation^ sender, _In_ Platform::Object^ args)
{
    m_deviceResources->ValidateDevice();
}

// Input event handlers.

void App::OnKeyDown(CoreWindow^ sender, KeyEventArgs^ args)
{
    if (!args->KeyStatus.WasKeyDown)
    {
        // Filter out keyboard auto-repeat.
        m_main->OnKeyChanged(args->VirtualKey, true);
    }
}

void App::OnKeyUp(CoreWindow^ sender, KeyEventArgs^ args)
{
    m_main->OnKeyChanged(args->VirtualKey, false);
    if (args->VirtualKey == VirtualKey::Escape)
    {
        SetMouseLook(false);
    }
}

void App::OnPointerPressed(CoreWindow^ sender, PointerEventArgs^ args)
{
    if (args->CurrentPoint->PointerDevice->PointerDeviceType == PointerDeviceType::Mouse)
    {
        if (!m_mouseLookMode && args->CurrentPoint->Properties->IsRightButtonPressed)
        {
            m_main->OnRightClick(args->CurrentPoint->Position.X, args->CurrentPoint->Position.Y);
        }
        SetMouseLook(!args->CurrentPoint->Properties->IsRightButtonPressed);
    }
    else
    {
        SetMouseLook(false);
    }
}

void App::OnPointerMoved(CoreWindow^ sender, PointerEventArgs^ args)
{
    // Only use this for non-mouse pointer input.  Mouse input will only be handled in mouse-look.
    if (args->CurrentPoint->PointerDevice->PointerDeviceType != PointerDeviceType::Mouse)
    {
        if (m_lastPointerPositionValid)
        {
            m_main->OnPointerMoved(
                args->CurrentPoint->Position.X - m_lastPointerPosition.X,
                args->CurrentPoint->Position.Y - m_lastPointerPosition.Y
                );
        }
        m_lastPointerPosition = args->CurrentPoint->Position;
        m_lastPointerPositionValid = true;
    }
}

void App::OnMouseMoved(MouseDevice^ sender, MouseEventArgs^ args)
{
    m_main->OnPointerMoved(
        static_cast<float>(args->MouseDelta.X),
        static_cast<float>(args->MouseDelta.Y)
        );
}

void App::SetMouseLook(bool value)
{
    // Mouse-look enables a first-person style camera control,
    // ensuring the mouse cursor does not leave the window unintentionally.
    // Mouse-look mode starts when the main window is left-clicked.
    // Mouse-look mode stops under the following conditions:
    //   -A non-mouse pointer device is used (touch/pen/controller).
    //   -The window loses focus or visibility.
    //   -The right mouse button is pressed.
    //   -The escape key is pressed.
    if (m_mouseLookMode != value)
    {
        m_mouseLookMode = value;
        if (m_mouseLookMode)
        {
            CoreWindow::GetForCurrentThread()->PointerCursor = nullptr;
            m_mouseMoveEventToken = MouseDevice::GetForCurrentView()->MouseMoved +=
                ref new TypedEventHandler<MouseDevice^, MouseEventArgs^>(this, &App::OnMouseMoved);
        }
        else
        {
            CoreWindow::GetForCurrentThread()->PointerCursor = ref new CoreCursor(CoreCursorType::Arrow, 0);
            MouseDevice::GetForCurrentView()->MouseMoved -= m_mouseMoveEventToken;
            m_lastPointerPositionValid = false;
        }
    }
}
