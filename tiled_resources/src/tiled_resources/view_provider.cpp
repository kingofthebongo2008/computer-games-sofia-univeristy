#include "pch.h"

#include "view_provider.h"

#include "build_window_environment.h"
#include "error.h"

using namespace winrt::Windows::ApplicationModel;
using namespace winrt::Windows::ApplicationModel::Core;
using namespace winrt::Windows::ApplicationModel::Activation;
using namespace winrt::Windows::Devices::Input;
using namespace winrt::Windows::UI::Core;
using namespace winrt::Windows::UI::Input;
using namespace winrt::Windows::System;
using namespace winrt::Windows::Graphics::Display;



void ViewProvider::Initialize(const CoreApplicationView& v)
{
    m_activated					= v.Activated(winrt::auto_revoke, { this, &ViewProvider::OnActivated });

	m_renderer					= std::unique_ptr<IMainRenderer>(MakeMainRenderer());
	m_renderer->Initialize();
}

void ViewProvider::Uninitialize()
{
	m_renderer->Uninitialize();
}

void ViewProvider::Load(winrt::hstring h)
{
	m_renderer->Load();
}

void ViewProvider::Run()
{
    while (m_window_running)
    {
        CoreWindow::GetForCurrentThread().Dispatcher().ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);
		m_renderer->Run();
    }
}

template <typename to, typename from> to* copy_to_abi_private(const from& w)
{
	void* v = nullptr;
	winrt::copy_to_abi(w, v);

	return reinterpret_cast<to*>(v);
}

template <typename to, typename from> winrt::com_ptr<to> copy_to_abi(const from& w)
{
	winrt::com_ptr<to> v;
	v.attach(copy_to_abi_private<::IUnknown>(w));
	return v;
}

void ViewProvider::SetWindow( const CoreWindow& w )
{
    m_closed			        = w.Closed(winrt::auto_revoke, { this, &ViewProvider::OnWindowClosed });
    m_size_changed		        = w.SizeChanged(winrt::auto_revoke, { this, &ViewProvider::OnWindowSizeChanged });
    auto e						= sample::build_environment(w, winrt::Windows::Graphics::Display::DisplayInformation::GetForCurrentView());
	m_renderer->SetWindow(copy_to_abi<::IUnknown>(w).get(), e );
}

void ViewProvider::OnWindowClosed(const CoreWindow&w, const CoreWindowEventArgs& a)
{
    m_window_running = false;
}

void ViewProvider::OnActivated(const CoreApplicationView&, const IActivatedEventArgs&)
{
    CoreWindow::GetForCurrentThread().Activate();
}

void ViewProvider::OnWindowSizeChanged(const CoreWindow& window, const WindowSizeChangedEventArgs& a)
{
	auto e = sample::build_environment(window, winrt::Windows::Graphics::Display::DisplayInformation::GetForCurrentView());
	m_renderer->OnWindowSizeChanged(e);
}

// Input event handlers.
void ViewProvider::OnKeyDown(const CoreWindow& window, const KeyEventArgs& args)
{
	/*
	if ( args.KeyStatus().WasKeyDown() == false)
	{
		// Filter out keyboard auto-repeat.
		//m_main->OnKeyChanged(args->VirtualKey, true);
	}
	*/
}

void ViewProvider::OnKeyUp(const CoreWindow& window, const KeyEventArgs& args)
{
	/*
	m_main->OnKeyChanged(args->VirtualKey, false);
	if (args.VirtualKey() == VirtualKey::Escape)
	{
		SetMouseLook(window, false);
	}
	*/
}

void ViewProvider::OnPointerPressed(const CoreWindow& window, const PointerEventArgs& args)
{
	/*
	if (args.CurrentPoint().PointerDevice().PointerDeviceType() == PointerDeviceType::Mouse)
	{
		if (!m_mouse_look_mode && args.CurrentPoint().Properties().IsRightButtonPressed())
		{
			//m_main->OnRightClick(args->CurrentPoint->Position.X, args->CurrentPoint->Position.Y);
		}
		SetMouseLook(window, !args.CurrentPoint().Properties().IsRightButtonPressed());
	}
	else
	{
		SetMouseLook(window, false);
		//m_main->OnPointerPressed(args->CurrentPoint->PointerId);
		//m_main->UpdatePointerPosition(args->CurrentPoint->PointerId, args->CurrentPoint->Position.X, args->CurrentPoint->Position.Y);
	}
	*/
}

void ViewProvider::OnPointerReleased(const CoreWindow& window, const PointerEventArgs& args)
{
	/*
	if (args->CurrentPoint->PointerDevice->PointerDeviceType != PointerDeviceType::Mouse)
	{
		m_main->OnPointerReleased(args->CurrentPoint->PointerId);
	}
	*/
}

void ViewProvider::OnPointerMoved(const CoreWindow& window, const PointerEventArgs& args)
{
	/*
	// Only use this for non-mouse pointer input.  Mouse input will only be handled in mouse-look.
	if (args->CurrentPoint->PointerDevice->PointerDeviceType != PointerDeviceType::Mouse)
	{
		m_main->UpdatePointerPosition(args->CurrentPoint->PointerId, args->CurrentPoint->Position.X, args->CurrentPoint->Position.Y);
	}
	*/
}

void ViewProvider::OnMouseMoved(const MouseDevice& sender, const MouseEventArgs& args)
{
	/*
	m_main->OnMouseMoved(
		static_cast<float>(args->MouseDelta.X),
		static_cast<float>(args->MouseDelta.Y)
	);
	*/
}

void ViewProvider::SetMouseLook(const CoreWindow& w, bool value)
{
	/*
	// Mouse-look enables a first-person style camera control,
	// ensuring the mouse cursor does not leave the window unintentionally.
	// Mouse-look mode starts when the main window is left-clicked.
	// Mouse-look mode stops under the following conditions:
	//   -A non-mouse pointer device is used (touch/pen/controller).
	//   -The window loses focus or visibility.
	//   -The right mouse button is pressed.
	//   -The escape key is pressed.
	if (m_mouse_look_mode != value)
	{
		m_mouse_look_mode = value;
		if (m_mouse_look_mode)
		{
			CoreWindow::GetForCurrentThread().PointerCursor() = nullptr;
			m_mouseMovedToken = MouseDevice::GetForCurrentView().MouseMoved({ this, &ViewProvider::OnMouseMoved });
		}
		else
		{
			CoreWindow::GetForCurrentThread().PointerCursor() = CoreCursor(CoreCursorType::Arrow, 0);
			MouseDevice::GetForCurrentView().MouseMoved(m_mouseMovedToken);
		}
	}
	*/
}


