#include "pch.h"

#include "view_provider.h"

#include "build_window_environment.h"
#include "error.h"

using namespace winrt::Windows::ApplicationModel;
using namespace winrt::Windows::ApplicationModel::Core;
using namespace winrt::Windows::ApplicationModel::Activation;
using namespace winrt::Windows::Devices::Input;
using namespace winrt::Windows::Devices;
using namespace winrt::Windows::UI::Core;
using namespace winrt::Windows::UI::Input;
using namespace winrt::Windows::System;
using namespace winrt::Windows::Graphics::Display;


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

void ViewProvider::SetWindow( const CoreWindow& w )
{
    m_closed			        = w.Closed(winrt::auto_revoke, { this, &ViewProvider::OnWindowClosed });
    m_size_changed		        = w.SizeChanged(winrt::auto_revoke, { this, &ViewProvider::OnWindowSizeChanged });
    auto e						= sample::build_environment(w, winrt::Windows::Graphics::Display::DisplayInformation::GetForCurrentView());
	m_renderer->SetWindow(copy_to_abi<::IUnknown>(w).get(), e );

	w.KeyDown({ this, &ViewProvider::OnKeyDown });
	w.KeyUp({ this, &ViewProvider::OnKeyUp });
	w.PointerPressed({ this, &ViewProvider::OnPointerPressed });
	w.PointerReleased({ this, &ViewProvider::OnPointerReleased });
	w.PointerMoved({ this, &ViewProvider::OnPointerMoved });
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

void ViewProvider::OnKeyDown(const CoreWindow& window, const KeyEventArgs& args)
{
}

void ViewProvider::OnKeyUp(const CoreWindow& window, const KeyEventArgs& args)
{
}

void ViewProvider::OnPointerPressed(const CoreWindow& window, const PointerEventArgs& args)
{
}

void ViewProvider::OnPointerReleased(const CoreWindow& window, const PointerEventArgs& args)
{
}

void ViewProvider::OnPointerMoved(const CoreWindow& window, const PointerEventArgs& args)
{
}

void ViewProvider::OnMouseMoved(const MouseDevice& sender, const MouseEventArgs& args)
{
}

void ViewProvider::SetMouseLook(const CoreWindow& w, bool value)
{
}
