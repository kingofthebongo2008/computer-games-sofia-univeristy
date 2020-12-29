#include "pch.h"
#include <cstdint>

#include "d3dx12.h"
/*

//Helper class that assists us using the descriptors
struct DescriptorHeapCpuView
{
    DescriptorHeapCpuView( D3D12_CPU_DESCRIPTOR_HANDLE  base, uint64_t offset) : m_base(base), m_offset(offset)
    {

    }

    D3D12_CPU_DESCRIPTOR_HANDLE operator () ( size_t index) const
    {
        return { m_base.ptr + index * m_offset };
    }

    D3D12_CPU_DESCRIPTOR_HANDLE operator + (size_t index) const
    {
        return { m_base.ptr + index * m_offset };
    }

    D3D12_CPU_DESCRIPTOR_HANDLE m_base      = {};
    uint64_t                    m_offset;
};

DescriptorHeapCpuView CpuView( ID3D12Device* d, ID3D12DescriptorHeap* heap )
{
    D3D12_DESCRIPTOR_HEAP_DESC desc = heap->GetDesc();
    return DescriptorHeapCpuView(heap->GetCPUDescriptorHandleForHeapStart(), d->GetDescriptorHandleIncrementSize(desc.Type));
}

struct exception : public std::exception
{
    exception(HRESULT h) : m_h(h)
    {

    }

    HRESULT m_h;
};

inline void ThrowIfFailed(HRESULT hr)
{
    if (hr != S_OK)
    {
        throw exception(hr);
    }
}

//Debug layer, issues warnings if something broken. Use it when you develop stuff
static Microsoft::WRL::ComPtr <ID3D12Debug> CreateDebug()
{
    Microsoft::WRL::ComPtr<ID3D12Debug> r;
    
    //check if you have installed debug layer, from the option windows components
    if ( D3D12GetDebugInterface( IID_PPV_ARGS(r.GetAddressOf()) ) == S_OK)
    {
        r->EnableDebugLayer();
    }
    return r;
}

static Microsoft::WRL::ComPtr<ID3D12Device4> CreateDevice()
{
    Microsoft::WRL::ComPtr<ID3D12Device4> r;

    //One can use d3d12 rendering with d3d11 capable hardware. You will just be missing new functionality.
    //Example, d3d12 on a D3D_FEATURE_LEVEL_9_1 hardare (as some phone are ).
    D3D_FEATURE_LEVEL features = D3D_FEATURE_LEVEL_11_1;
    ThrowIfFailed(D3D12CreateDevice(nullptr, features, IID_PPV_ARGS(r.GetAddressOf())));
    return r;
}


static Microsoft::WRL::ComPtr<ID3D12CommandQueue> CreateCommandQueue(ID3D12Device* d )
{
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> r;
    D3D12_COMMAND_QUEUE_DESC q = {};

    q.Type = D3D12_COMMAND_LIST_TYPE_DIRECT; //submit copy, raster, compute payloads
    ThrowIfFailed(d->CreateCommandQueue(&q, IID_PPV_ARGS(r.GetAddressOf())));
    return r;
}

static Microsoft::WRL::ComPtr<IDXGISwapChain1> CreateSwapChain(const HWND w, ID3D12CommandQueue* d)
{
    Microsoft::WRL::ComPtr<IDXGIFactory2> f;
    Microsoft::WRL::ComPtr<IDXGISwapChain1> r;
    
    ThrowIfFailed(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(f.GetAddressOf())));

    DXGI_SWAP_CHAIN_DESC1 desc = {};


    desc.BufferCount	= 2;
    desc.Format			= DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.Width			= static_cast<UINT>(e.m_back_buffer_size.Width);
    desc.Height			= static_cast<UINT>(e.m_back_buffer_size.Height);
    desc.SampleDesc.Count = 1;
    desc.SwapEffect		= DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.BufferUsage	= DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.AlphaMode		= DXGI_ALPHA_MODE_IGNORE;
    desc.Scaling		= DXGI_SCALING_NONE;

    ThrowIfFailed(f->CreateSwapChainForHwnd(d, w, &desc, nullptr, nullptr, r.GetAddressOf()));
    return r;
}

static Microsoft::WRL::ComPtr <ID3D12Fence> CreateFence(ID3D12Device1* device, uint64_t initialValue = 1)
{
    Microsoft::WRL::ComPtr<ID3D12Fence> r;
    ThrowIfFailed(device->CreateFence(initialValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(r.GetAddressOf())));
    return r;
}

static Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> CreateDescriptorHeap(ID3D12Device1* device)
{
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> r;
    D3D12_DESCRIPTOR_HEAP_DESC d = {};

    d.NumDescriptors = 2;
    d.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    device->CreateDescriptorHeap(&d, IID_PPV_ARGS(r.GetAddressOf())));
    return r;
}

static Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> CreateDescriptorHeapRendering(ID3D12Device1* device)
{
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> r;
    D3D12_DESCRIPTOR_HEAP_DESC d = {};

    d.NumDescriptors = 2;
    d.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    device->CreateDescriptorHeap(&d, IID_PPV_ARGS(r.GetAddressOf())));
    return r;
}

//compute sizes
static D3D12_RESOURCE_DESC DescribeSwapChain ( uint32_t width, uint32_t height)
{
    D3D12_RESOURCE_DESC d   = {};
    d.Alignment             = 0;
    d.DepthOrArraySize      = 1;
    d.Dimension             = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    d.Flags                 = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    d.Format                = DXGI_FORMAT_B8G8R8A8_TYPELESS;     //important for computing the resource footprint
    d.Height                = height;
    d.Layout                = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    d.MipLevels             = 1;
    d.SampleDesc.Count      = 1;
    d.SampleDesc.Quality    = 0;
    d.Width                 = width;
    return                  d;
}

static Microsoft::WRL::ComPtr<ID3D12Resource1> CreateSwapChainResource1(ID3D12Device1* device, uint32_t width, uint32_t height)
{
    D3D12_RESOURCE_DESC d               = DescribeSwapChain( width, height );

    Microsoft::WRL::ComPtr<ID3D12Resource1>     r;
    D3D12_HEAP_PROPERTIES p             = {};
    p.Type                              = D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_STATES       state   = D3D12_RESOURCE_STATE_PRESENT;

    D3D12_CLEAR_VALUE v = {};
    v.Color[0] = 1.0f;
    v.Format = DXGI_FORMAT_B8G8R8A8_UNORM;

    ThrowIfFailed(device->CreateCommittedResource(&p, D3D12_HEAP_FLAG_NONE, &d, state, &v, IID_PPV_ARGS(r.GetAddressOf())));
    return r;
}

//Get the buffer for the swap chain, this is the end result for the window
static Microsoft::WRL::ComPtr<ID3D12Resource1> CreateSwapChainResource(ID3D12Device1* device, IDXGISwapChain* chain, uint32_t buffer)
{
    Microsoft::WRL::ComPtr<ID3D12Resource1> r;

    chain->GetBuffer(buffer, IID_PPV_ARGS(r.GetAddressOf()));
    return r;
}

//Create a gpu metadata that describes the swap chain, type, format. it will be used by the gpu interpret the data in the swap chain(reading/writing).
static void CreateSwapChainDescriptor(ID3D12Device1* device, ID3D12Resource1* resource, D3D12_CPU_DESCRIPTOR_HANDLE handle )
{
    D3D12_RENDER_TARGET_VIEW_DESC d = {};
    d.ViewDimension                 = D3D12_RTV_DIMENSION_TEXTURE2D;
    d.Format                        = DXGI_FORMAT_B8G8R8A8_UNORM;       //how we will view the resource during rendering
    device->CreateRenderTargetView(resource, &d, handle);
}

//Create the memory manager for the gpu commands
static Microsoft::WRL::ComPtr <ID3D12CommandAllocator> CreateCommandAllocator(ID3D12Device1* device)
{
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> r;
    ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(r.GetAddressOf())));
    return r;
}

//create an object that will record commands
static Microsoft::WRL::ComPtr <ID3D12GraphicsCommandList1> CreateCommandList(ID3D12Device1* device, ID3D12CommandAllocator* a)
{
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList1> r;
    ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, a, nullptr, IID_PPV_ARGS(r.GetAddressOf())));

    r->Close();
    return r;
}

//create an object which represents what types of external data the shaders will use. You can imagine f(int x, float y); Root Signature is that we have two parameters on locations 0 and 1 types int and float
static Microsoft::WRL::ComPtr< ID3D12RootSignature>	 CreateRootSignature(ID3D12Device1* device)
{
    static 
    #include <default_graphics_signature.h>

    Microsoft::WRL::ComPtr<ID3D12RootSignature> r;
    ThrowIfFailed(device->CreateRootSignature( 0, &g_default_graphics_signature[0], sizeof(g_default_graphics_signature), IID_PPV_ARGS(r.GetAddressOf())));
    return r;
}

//create a state for the rasterizer. that will be set a whole big monolitic block. Below the driver optimizes it in the most compact form for it. 
//It can be something as 16 DWORDS that gpu will read and trigger its internal rasterizer state
static Microsoft::WRL::ComPtr< ID3D12PipelineState>	 CreateTrianglePipelineState(ID3D12Device1* device, ID3D12RootSignature* root)
{
    static
    #include <triangle_pixel.h>

    static
    #include <triangle_vertex.h>

    D3D12_GRAPHICS_PIPELINE_STATE_DESC state = {};
    state.pRootSignature			= root;
    state.SampleMask				= UINT_MAX;
    state.RasterizerState			= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

    state.RasterizerState.CullMode	= D3D12_CULL_MODE_NONE;
    state.RasterizerState.FrontCounterClockwise = TRUE;

    state.PrimitiveTopologyType		= D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    state.NumRenderTargets			= 1;
    state.RTVFormats[0]				= DXGI_FORMAT_B8G8R8A8_UNORM;
    state.SampleDesc.Count			= 1;
    state.BlendState				= CD3DX12_BLEND_DESC(D3D12_DEFAULT);

    state.DepthStencilState.DepthEnable = FALSE;
    state.DepthStencilState.StencilEnable = FALSE;

    state.VS = { &g_triangle_vertex[0], sizeof(g_triangle_vertex) };
    state.PS = { &g_triangle_pixel[0], sizeof(g_triangle_pixel) };

    Microsoft::WRL::ComPtr<ID3D12PipelineState> r;

    ThrowIfFailed(device->CreateGraphicsPipelineState(&state, IID_PPV_ARGS(r.GetAddressOf())));
    return r;
}

//create a state for the rasterizer. that will be set a whole big monolitic block. Below the driver optimizes it in the most compact form for it. 
//It can be something as 16 DWORDS that gpu will read and trigger its internal rasterizer state
static Microsoft::WRL::ComPtr< IDirect3D9> CreateD3D9Device(ID3D12Device* device, ID3D12RootSignature* root)
{
    Microsoft::WRL::ComPtr<IDirect3D9> r;

    return r;
}


class ViewProvider : public winrt::implements<ViewProvider, IFrameworkView, IFrameworkViewSource>
{
    public:

    IFrameworkView CreateView()
    {
            return *this;
    }

    void Initialize(const CoreApplicationView& v)
    {
        m_activated					= v.Activated(winrt::auto_revoke, { this, &ViewProvider::OnActivated });
        m_debug						= CreateDebug();
        m_device					= CreateDevice();

        m_queue					    = CreateCommandQueue(m_device.get());

        m_descriptorHeap		    = CreateDescriptorHeap(m_device.get());

        m_descriptorHeapRendering   = CreateDescriptorHeapRendering(m_device.get());

        //if you have many threads that generate commands. 1 per thread per frame
        m_command_allocator[0]		= CreateCommandAllocator(m_device.get());
        m_command_allocator[1]		= CreateCommandAllocator(m_device.get());

        m_command_list[0]			= CreateCommandList(m_device.get(), m_command_allocator[0].get());
        m_command_list[1]			= CreateCommandList(m_device.get(), m_command_allocator[1].get());

        //fence, sync from the gpu and cpu
        m_fence						= CreateFence(m_device.get());
        m_fence_event				= CreateEvent(nullptr, false, false, nullptr);

        if (m_fence_event == nullptr)
        {
            ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }
    }

    void Uninitialize() 
    {

    }

    void Run()
    {
        while (m_window_running)
        {
            CoreWindow::GetForCurrentThread().Dispatcher().ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);

            std::lock_guard lock(m_blockRendering);
  

            //reset the command generators for this frame if they have data, which was already used by the gpu
            ID3D12CommandAllocator*     allocator       = m_command_allocator[m_frame_index].get();
            ID3D12GraphicsCommandList1* commandList     = m_command_list[m_frame_index].get();
            allocator->Reset();
            commandList->Reset(allocator, nullptr);

            // Set Descriptor heaps
            {
                //ID3D12DescriptorHeap* heaps[] = { m_descriptorHeap.get()};
                //commandList->SetDescriptorHeaps(1, heaps);
            }

            //get the pointer to the gpu memory
            D3D12_CPU_DESCRIPTOR_HANDLE back_buffer = CpuView(m_device.get(), m_descriptorHeap.get()) + m_swap_chain_descriptors[m_frame_index];

            //Transition resources for writing. flush caches
            {
                D3D12_RESOURCE_BARRIER barrier = {};

                barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                barrier.Transition.pResource = m_swap_chain_buffers[m_frame_index].get();
                barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
                barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;
                barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                commandList->ResourceBarrier(1, &barrier);
            }

            //Mark the resources in the rasterizer output
            {
                commandList->OMSetRenderTargets(1, &back_buffer, TRUE, nullptr);
            }

            //do the clear, fill the memory with a value
            {
                FLOAT c[4] = { 0.0f, 0.f,0.f,0.f };
                commandList->ClearRenderTargetView(back_buffer, c, 0, nullptr);
            }


            {
                //set the type of the parameters that we will use in the shader
                commandList->SetGraphicsRootSignature(m_root_signature.get());

                //set the raster pipeline state as a whole, it was prebuilt before
                commandList->SetPipelineState(m_triangle_state.get());
                
                //set the scissor test separately (which parts of the view port will survive)
                {
                    D3D12_RECT r = { 0, 0, static_cast<int32_t>(m_back_buffer_width), static_cast<int32_t>(m_back_buffer_height) };
                    commandList->RSSetScissorRects(1, &r);
                }

                //set the viewport. 
                {
                    D3D12_VIEWPORT v;
                    v.TopLeftX = 0;
                    v.TopLeftY = 0;
                    v.MinDepth = 0.0f;
                    v.MaxDepth = 1.0f;
                    v.Width = static_cast<float>(m_back_buffer_width);
                    v.Height = static_cast<float>(m_back_buffer_height);
                    commandList->RSSetViewports(1, &v);
                }

                //set the types of the triangles we will use
                {
                    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                }

                //draw the triangle
                commandList->DrawInstanced(3, 1, 0, 0);
            }
            

            //Transition resources for presenting, flush the gpu caches
            {
                D3D12_RESOURCE_BARRIER barrier = {};

                barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                barrier.Transition.pResource = m_swap_chain_buffers[m_frame_index].get();
                barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
                barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
                barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                commandList->ResourceBarrier(1, &barrier);
            }
            
            commandList->Close();   //close the list

            {
                //form group of several command lists
                ID3D12CommandList* lists[] = { commandList };
                m_queue->ExecuteCommandLists(1, lists); //Execute what we have, submission of commands to the gpu
            }   

            m_swap_chain->Present(1, 0);    //present the swap chain

            //Tell the gpu to signal the cpu after it finishes executing the commands that we have just submitted
            ThrowIfFailed(m_queue->Signal(m_fence.get(), m_fence_value));

            //Now block the cpu until the gpu completes the previous frame
            if (m_fence->GetCompletedValue() < m_fence_value)
            {
                ThrowIfFailed(m_fence->SetEventOnCompletion(m_fence_value, m_fence_event));
                WaitForSingleObject(m_fence_event, INFINITE);
            }

            //prepare for the next frame
            m_fence_value = m_fence_value + 1;
            m_frame_index = m_swap_chain->GetCurrentBackBufferIndex();
        }
    }

    void Load(winrt::hstring h)
    {
        m_root_signature = CreateRootSignature(m_device.get());
        m_triangle_state = CreateTrianglePipelineState(m_device.get(), m_root_signature.get());
    }

    void SetWindow(const CoreWindow& w)
    {
        m_closed			    = w.Closed(winrt::auto_revoke, { this, &ViewProvider::OnWindowClosed });
        m_size_changed		    = w.SizeChanged(winrt::auto_revoke, { this, &ViewProvider::OnWindowSizeChanged });

        m_swap_chain		    = CreateSwapChain(w, m_queue.get());
        m_frame_index           = m_swap_chain->GetCurrentBackBufferIndex();

		auto e = sample::build_environment(w, winrt::Windows::Graphics::Display::DisplayInformation::GetForCurrentView());

		//Now recreate the swap chain with the new dimensions, we must have back buffer as the window size
		m_back_buffer_width		= static_cast<UINT>(e.m_back_buffer_size.Width);
		m_back_buffer_height	= static_cast<UINT>(e.m_back_buffer_size.Height);

        //allocate memory for the view
        m_swap_chain_buffers[0] = CreateSwapChainResource(m_device.get(), m_swap_chain.get(), 0);
        m_swap_chain_buffers[1] = CreateSwapChainResource(m_device.get(), m_swap_chain.get(), 1);

        m_swap_chain_buffers[0]->SetName(L"Buffer 0");
        m_swap_chain_buffers[1]->SetName(L"Buffer 1");

        //create render target views, that will be used for rendering
        CreateSwapChainDescriptor(m_device.get(), m_swap_chain_buffers[0].get(), CpuView(m_device.get(), m_descriptorHeap.get()) + 0);
        CreateSwapChainDescriptor(m_device.get(), m_swap_chain_buffers[1].get(), CpuView(m_device.get(), m_descriptorHeap.get()) + 1);

        //Where are located the descriptors
        m_swap_chain_descriptors[0] = 0;
        m_swap_chain_descriptors[1] = 1;
    }

    void OnWindowClosed(const CoreWindow&w, const CoreWindowEventArgs& a)
    {
        m_window_running = false;
    }

    void OnActivated(const CoreApplicationView&, const IActivatedEventArgs&)
    {
        CoreWindow::GetForCurrentThread().Activate();
    }

    void OnWindowSizeChanged(const CoreWindow& w, const WindowSizeChangedEventArgs& a)
    {
        //wait for the render thread to finish and block it so we can submit a command
        std::lock_guard lock(m_blockRendering);

        //Now wait for the gpu to finish what it has from the main thread

        //Insert in the gpu a command after all submitted commands so far.
        ThrowIfFailed(m_queue->Signal(m_fence.get(), m_fence_value));

        //Wait for the gpu to notify us back that it had passed. Now it is idle
        ThrowIfFailed(m_fence->SetEventOnCompletion(m_fence_value, m_fence_event));
        WaitForSingleObject(m_fence_event, INFINITE);

        //Prepare to unblock the rendering
        m_fence_value = m_fence_value + 1;
        m_frame_index = 0;

		auto e = sample::build_environment(w, winrt::Windows::Graphics::Display::DisplayInformation::GetForCurrentView());

		//Now recreate the swap chain with the new dimensions, we must have back buffer as the window size
		m_back_buffer_width		= static_cast<UINT>(e.m_back_buffer_size.Width);
		m_back_buffer_height	= static_cast<UINT>(e.m_back_buffer_size.Height);

		m_swap_chain_buffers[0] = nullptr;
		m_swap_chain_buffers[1] = nullptr;

		ThrowIfFailed(m_swap_chain->ResizeBuffers(2, m_back_buffer_width, m_back_buffer_height, DXGI_FORMAT_B8G8R8A8_UNORM, 0));

        //allocate memory for the swap chain again
        m_swap_chain_buffers[0] = CreateSwapChainResource(m_device.get(), m_swap_chain.get(), 0);
        m_swap_chain_buffers[1] = CreateSwapChainResource(m_device.get(), m_swap_chain.get(), 1);

        //set names so we can see them in pix
        m_swap_chain_buffers[0]->SetName(L"Buffer 0");
        m_swap_chain_buffers[1]->SetName(L"Buffer 1");

        //create render target views, that will be used for rendering
        CreateSwapChainDescriptor(m_device.get(), m_swap_chain_buffers[0].get(), CpuView(m_device.get(), m_descriptorHeap.get()) + 0);
        CreateSwapChainDescriptor(m_device.get(), m_swap_chain_buffers[1].get(), CpuView(m_device.get(), m_descriptorHeap.get()) + 1);

        m_swap_chain_descriptors[0] = 0;
        m_swap_chain_descriptors[1] = 1;
    }

    bool m_window_running = true;

    CoreWindow::Closed_revoker					m_closed;
    CoreWindow::SizeChanged_revoker				m_size_changed;
    CoreApplicationView::Activated_revoker		m_activated;
    
    Microsoft::WRL::ComPtr <ID3D12Debug>                m_debug;
    Microsoft::WRL::ComPtr <ID3D12Device1>				m_device;           //device for gpu resources
    Microsoft::WRL::ComPtr <IDXGISwapChain3>			m_swap_chain;       //swap chain for 

    Microsoft::WRL::ComPtr <ID3D12Fence>        		m_fence;                     //fence for cpu/gpu synchronization
    Microsoft::WRL::ComPtr <ID3D12CommandQueue>   		m_queue;                     //queue to the device

    Microsoft::WRL::ComPtr <ID3D12DescriptorHeap>   	m_descriptorHeap;            //descriptor heap for the resources

    Microsoft::WRL::ComPtr <ID3D12DescriptorHeap>   	m_descriptorHeapRendering;   //descriptor heap for the resources

    std::mutex                                  m_blockRendering;   //block render thread for the swap chain resizes

    Microsoft::WRL::ComPtr<ID3D12Resource1>             m_swap_chain_buffers[2];
    uint64_t                                    m_swap_chain_descriptors[2];

    uint32_t									m_back_buffer_width = 0;
    uint32_t									m_back_buffer_height = 0;

    Microsoft::WRL::ComPtr <ID3D12CommandAllocator>   	m_command_allocator[2];		//one per frame
    Microsoft::WRL::ComPtr <ID3D12GraphicsCommandList1> m_command_list[2];			//one per frame

    uint64_t                                    m_frame_index	= 0;
    uint64_t									m_fence_value	= 1;
    HANDLE										m_fence_event = {};

    //Rendering
    Microsoft::WRL::ComPtr< ID3D12RootSignature>		m_root_signature;
    Microsoft::WRL::ComPtr< ID3D12PipelineState>		m_triangle_state;
};


int32_t __stdcall wWinMain( HINSTANCE, HINSTANCE,PWSTR, int32_t )
{
    ThrowIfFailed(CoInitializeEx(nullptr, COINIT_MULTITHREADED));
    CoUninitialize();
    return 0;
}
*/
