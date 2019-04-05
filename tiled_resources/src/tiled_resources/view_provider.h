#include "pch.h"
#include <cstdint>

#include "device_resources.h"
#include "sampling_renderer.h"
#include "free_camera.h"

using namespace winrt::Windows::UI::Core;
using namespace winrt::Windows::ApplicationModel::Core;
using namespace winrt::Windows::ApplicationModel::Activation;

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
	void SetWindow(const CoreWindow& w);
	void OnWindowClosed(const CoreWindow& w, const CoreWindowEventArgs& a);
	void OnActivated(const CoreApplicationView&, const IActivatedEventArgs&);
	void OnWindowSizeChanged(const CoreWindow& window, const WindowSizeChangedEventArgs& a);

    bool m_window_running						= true;

    CoreWindow::Closed_revoker					m_closed;
    CoreWindow::SizeChanged_revoker				m_size_changed;
    CoreApplicationView::Activated_revoker		m_activated;

    std::unique_ptr<sample::DeviceResources>    m_deviceResources;
    std::unique_ptr<sample::SamplingRenderer>   m_samplingRenderer;

    std::mutex                                  m_blockRendering;           //block render thread for the swap chain resizes

    winrt::com_ptr <ID3D12CommandAllocator>   	m_command_allocator[2];		//one per frame
    winrt::com_ptr <ID3D12GraphicsCommandList1> m_command_list[2];			//one per frame

    uint32_t                                    m_frame_index	    = 0;
    uint64_t									m_fence_value[2]    = { 1, 1 };

    //Rendering
    winrt::com_ptr< ID3D12RootSignature>		m_root_signature;
    winrt::com_ptr< ID3D12PipelineState>		m_sampling_renderer_state;  //responsible for output of the parts that we want
    winrt::com_ptr< ID3D12PipelineState>		m_terrain_renderer_state;   //responsible to renderer the terrain


	winrt::com_ptr <ID3D12Resource1>   			m_geometry_vertex_buffer;	//planet geometry
	winrt::com_ptr <ID3D12Resource1>   			m_geometry_index_buffer;	//planet indices

    //view concepts
    D3D12_VERTEX_BUFFER_VIEW                    m_planet_vertex_view;       //vertices for render
    D3D12_INDEX_BUFFER_VIEW                     m_planet_index_view;		//indices for render

	sample::FreeCamera							m_camera;
};

