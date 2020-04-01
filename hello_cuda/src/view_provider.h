#include "pch.h"
#include <cstdint>
#include <cuda_runtime.h>

using namespace winrt::Windows::UI::Core;
using namespace winrt::Windows::ApplicationModel::Core;
using namespace winrt::Windows::ApplicationModel::Activation;
using namespace Microsoft::WRL;


class ViewProvider : public winrt::implements<ViewProvider, IFrameworkView, IFrameworkViewSource>
{
    public:

    IFrameworkView CreateView();

    void Initialize(const CoreApplicationView& v);
    void Uninitialize();
    
    void Run();
    void Load(winrt::hstring h);

    void SetWindow(const CoreWindow& w);
    void OnActivated(const CoreApplicationView&, const IActivatedEventArgs&);
    void OnWindowClosed(const CoreWindow& w, const CoreWindowEventArgs& a);
    void OnWindowSizeChanged(const CoreWindow& w, const WindowSizeChangedEventArgs& a);


    bool m_window_running = true;

    CoreWindow::Closed_revoker					m_closed;
    CoreWindow::SizeChanged_revoker				m_size_changed;
    CoreApplicationView::Activated_revoker		m_activated;
    
    winrt::com_ptr <ID3D12Debug>                m_debug;
    winrt::com_ptr <ID3D12Device1>				m_device;                    //device for gpu resources
    winrt::com_ptr <IDXGISwapChain3>			m_swap_chain;                //swap chain for 

    winrt::com_ptr <ID3D12Fence>        		m_fence;                     //fence for cpu/gpu synchronization
    winrt::com_ptr <ID3D12CommandQueue>   		m_queue;                     //queue to the device

    winrt::com_ptr <ID3D12DescriptorHeap>   	m_descriptorHeap;            //descriptor heap for the resources
    winrt::com_ptr <ID3D12DescriptorHeap>   	m_descriptorHeapRendering;   //descriptor heap for the resources

    std::mutex                                  m_blockRendering;           //block render thread for the swap chain resizes

    winrt::com_ptr<ID3D12Resource1>             m_swap_chain_buffers[2];
    uint64_t                                    m_swap_chain_descriptors[2];

    uint32_t									m_back_buffer_width = 0;
    uint32_t									m_back_buffer_height = 0;

    winrt::com_ptr <ID3D12CommandAllocator>   	m_command_allocator[2];		//one per frame
    winrt::com_ptr <ID3D12GraphicsCommandList1> m_command_list[2];			//one per frame

    uint64_t                                    m_frame_index	= 0;
    uint64_t									m_fence_value	= 1;
    HANDLE										m_fence_event = {};

    //Rendering
    winrt::com_ptr< ID3D12RootSignature>		m_root_signature;
    winrt::com_ptr< ID3D12PipelineState>		m_triangle_state;
};

