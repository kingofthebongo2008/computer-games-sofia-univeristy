#include "pch.h"

#include <winrt/Windows.UI.Core.h>
#include <winrt/Windows.ApplicationModel.Core.h>
#include <winrt/Windows.ApplicationModel.Activation.h>
#include <winrt/Windows.Devices.Input.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Graphics.Display.h>
#include <winrt/Windows.UI.Input.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.Ui.Core.h>
#include <winrt/Windows.UI.ViewManagement.h>

#include "main_renderer_interface.h"

using namespace winrt::Windows::UI::Core;
using namespace winrt::Windows::ApplicationModel::Core;
using namespace winrt::Windows::ApplicationModel::Activation;
using namespace winrt::Windows::Devices::Input;

class ViewProvider : public winrt::implements<ViewProvider, IFrameworkView, IFrameworkViewSource>
{
    public:

    IFrameworkView CreateView()
    {
            return *this;
    }

	void Initialize(const CoreApplicationView& v);
	void Uninitialize();
	void Load(winrt::hstring h);
    void Run();

	//Window state
	void SetWindow(const CoreWindow& w);
	void OnWindowClosed(const CoreWindow& w, const CoreWindowEventArgs& a);
	void OnActivated(const CoreApplicationView&, const IActivatedEventArgs&);
	void OnWindowSizeChanged(const CoreWindow& window, const WindowSizeChangedEventArgs& a);


	// Input event handlers.
	void OnKeyDown(const CoreWindow& window, const KeyEventArgs& args);
	void OnKeyUp(const CoreWindow& window, const KeyEventArgs& args);
	void OnPointerPressed(const CoreWindow& window, const PointerEventArgs& args);
	void OnPointerReleased(const CoreWindow& window, const PointerEventArgs& args);
	void OnPointerMoved(const CoreWindow& window, const PointerEventArgs& args);
	void OnMouseMoved(const MouseDevice& sender, const MouseEventArgs& args);
	
	private:

	void SetMouseLook(const CoreWindow& w, bool value);

    bool m_window_running						= true;
	bool m_mouse_look_mode						= false;

	winrt::event_token							m_mouseMovedToken;

    CoreWindow::Closed_revoker					m_closed;
    CoreWindow::SizeChanged_revoker				m_size_changed;
    CoreApplicationView::Activated_revoker		m_activated;
	std::unique_ptr< IMainRenderer >			m_renderer;	//break connection between winrt and native code
};

