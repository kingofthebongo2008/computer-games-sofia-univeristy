#include "pch.h"
#include <cstdint>

#include <concrt.h>
#include <ppl.h>

#include "d3dx12.h"

#include "window_environment.h"
#include "device_resources.h"
#include "sampling_renderer.h"
#include "error.h"



using namespace winrt::Windows::UI::Core;
using namespace winrt::Windows::ApplicationModel::Core;
using namespace winrt::Windows::ApplicationModel::Activation;

//Create the memory manager for the gpu commands
static winrt::com_ptr <ID3D12CommandAllocator> CreateCommandAllocator(ID3D12Device1* device)
{
    winrt::com_ptr<ID3D12CommandAllocator> r;
    sample::ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), r.put_void()));
    return r;
}

//create an object that will record commands
static winrt::com_ptr <ID3D12GraphicsCommandList1> CreateCommandList(ID3D12Device1* device, ID3D12CommandAllocator* a)
{
    winrt::com_ptr<ID3D12GraphicsCommandList1> r;
    sample::ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, a, nullptr, __uuidof(ID3D12GraphicsCommandList1), r.put_void()));

    r->Close();
    return r;
}

//create an object which represents what types of external data the shaders will use. You can imagine f(int x, float y); Root Signature is that we have two parameters on locations 0 and 1 types int and float
static winrt::com_ptr< ID3D12RootSignature>	 CreateRootSignature(ID3D12Device1* device)
{
    static 
    #include <default_graphics_signature.h>

    winrt::com_ptr<ID3D12RootSignature> r;
    sample::ThrowIfFailed(device->CreateRootSignature( 0, &g_default_graphics_signature[0], sizeof(g_default_graphics_signature), __uuidof(ID3D12RootSignature), r.put_void()));
    return r;
}

//create a state for the rasterizer. that will be set a whole big monolitic block. Below the driver optimizes it in the most compact form for it. 
//It can be something as 16 DWORDS that gpu will read and trigger its internal rasterizer state
static winrt::com_ptr< ID3D12PipelineState>	 CreateTrianglePipelineState(ID3D12Device1* device, ID3D12RootSignature* root)
{
    static
    #include <triangle_pixel.h>

    static
    #include <triangle_vertex.h>

    D3D12_GRAPHICS_PIPELINE_STATE_DESC state = {};
    state.pRootSignature			        = root;
    state.SampleMask				        = UINT_MAX;
    state.RasterizerState			        = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

    state.PrimitiveTopologyType		        = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    state.NumRenderTargets			        = 1;
    state.RTVFormats[0]				        = DXGI_FORMAT_B8G8R8A8_UNORM;
    state.SampleDesc.Count			        = 1;
    state.BlendState				        = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    state.DSVFormat                         = DXGI_FORMAT_D24_UNORM_S8_UINT;

    state.DepthStencilState.DepthEnable     = TRUE;
    state.DepthStencilState.DepthWriteMask  = D3D12_DEPTH_WRITE_MASK_ALL;
    state.DepthStencilState.DepthFunc       = D3D12_COMPARISON_FUNC_LESS;
    state.DepthStencilState.StencilEnable   = FALSE;


    state.VS = { &g_triangle_vertex[0], sizeof(g_triangle_vertex) };
    state.PS = { &g_triangle_pixel[0], sizeof(g_triangle_pixel) };

    winrt::com_ptr<ID3D12PipelineState> r;

    sample::ThrowIfFailed(device->CreateGraphicsPipelineState(&state, __uuidof(ID3D12PipelineState), r.put_void()));
    return r;
}

//create a state for the rasterizer. that will be set a whole big monolitic block. Below the driver optimizes it in the most compact form for it. 
//It can be something as 16 DWORDS that gpu will read and trigger its internal rasterizer state
static winrt::com_ptr< ID3D12PipelineState>	 CreateSamplingRendererState(ID3D12Device1* device, ID3D12RootSignature* root)
{
    static
    #include <sampling_renderer_pixel.h>

    static
    #include <terrain_renderer_vertex.h>

    D3D12_GRAPHICS_PIPELINE_STATE_DESC state = {};
    state.pRootSignature = root;
    state.SampleMask = UINT_MAX;
    state.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

    state.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    state.RasterizerState.FrontCounterClockwise = TRUE;

    state.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    state.NumRenderTargets = 1;
    state.RTVFormats[0] = DXGI_FORMAT_B8G8R8A8_UNORM;
    state.SampleDesc.Count = 1;
    state.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

    state.DepthStencilState.DepthEnable = FALSE;
    state.DepthStencilState.StencilEnable = FALSE;

    //Describe the format of the vertices. In the gpu they are going to be unpacked into the registers
    //If you apply compression to then, you can always make them bytes
    D3D12_INPUT_ELEMENT_DESC inputLayoutDesc[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    state.InputLayout.NumElements = 1;
    state.InputLayout.pInputElementDescs = &inputLayoutDesc[0];

    state.VS = { &g_terrain_renderer_vertex[0], sizeof(g_terrain_renderer_vertex) };
    state.PS = { &g_sampling_renderer_pixel[0], sizeof(g_sampling_renderer_pixel) };

    winrt::com_ptr<ID3D12PipelineState> r;
    sample::ThrowIfFailed(device->CreateGraphicsPipelineState(&state, __uuidof(ID3D12PipelineState), r.put_void()));
    return r;
}

//create a state for the rasterizer. that will be set a whole big monolitic block. Below the driver optimizes it in the most compact form for it. 
//It can be something as 16 DWORDS that gpu will read and trigger its internal rasterizer state
static winrt::com_ptr< ID3D12PipelineState>	 CreateTerrainRendererState(ID3D12Device1* device, ID3D12RootSignature* root)
{
    static
    #include <terrain_renderer_tier2_pixel.h>

    static
    #include <terrain_renderer_vertex.h>

    D3D12_GRAPHICS_PIPELINE_STATE_DESC state = {};
    state.pRootSignature = root;
    state.SampleMask = UINT_MAX;
    state.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

    state.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    state.RasterizerState.FrontCounterClockwise = TRUE;

    state.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    state.NumRenderTargets = 1;
    state.RTVFormats[0] = DXGI_FORMAT_B8G8R8A8_UNORM;
    state.SampleDesc.Count = 1;
    state.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

    state.DepthStencilState.DepthEnable = FALSE;
    state.DepthStencilState.StencilEnable = FALSE;

    //Describe the format of the vertices. In the gpu they are going to be unpacked into the registers
    //If you apply compression to then, you can always make them bytes
    D3D12_INPUT_ELEMENT_DESC inputLayoutDesc[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    state.InputLayout.NumElements = 1;
    state.InputLayout.pInputElementDescs = &inputLayoutDesc[0];

    state.VS = { &g_terrain_renderer_vertex[0],      sizeof(g_terrain_renderer_vertex) };
    state.PS = { &g_terrain_renderer_tier2_pixel[0], sizeof(g_terrain_renderer_tier2_pixel) };

    winrt::com_ptr<ID3D12PipelineState> r;
    sample::ThrowIfFailed(device->CreateGraphicsPipelineState(&state, __uuidof(ID3D12PipelineState), r.put_void()));
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

        m_deviceResources           = std::make_unique<sample::DeviceResources>();
        m_samplingRenderer          = std::make_unique<sample::SamplingRenderer>();


        //if you have many threads that generate commands. 1 per thread per frame
        {
            ID3D12Device1* d = m_deviceResources->Device();

            m_command_allocator[0] = CreateCommandAllocator(d);
            m_command_allocator[1] = CreateCommandAllocator(d);

            m_command_list[0] = CreateCommandList(d, m_command_allocator[0].get());
            m_command_list[1] = CreateCommandList(d, m_command_allocator[1].get());
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
            D3D12_CPU_DESCRIPTOR_HANDLE back_buffer = m_deviceResources->SwapChainHandle(m_frame_index);

            //Transition resources for writing. flush caches
            {
                D3D12_RESOURCE_BARRIER barrier = {};

                barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                barrier.Transition.pResource = m_deviceResources->SwapChainBuffer(m_frame_index);
                barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
                barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;
                barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                commandList->ResourceBarrier(1, &barrier);
            }

            //Get Depth Buffer
            D3D12_CPU_DESCRIPTOR_HANDLE depth_buffer = m_deviceResources->SwapChainDepthHandle(m_frame_index);

            //Mark the resources in the rasterizer output
            {
                commandList->OMSetRenderTargets(1, &back_buffer, TRUE, &depth_buffer);
            }

            //do the clear, fill the memory with a value
            {
                FLOAT c[4] = { 1.0f, 0.f,0.f,0.f };
                commandList->ClearRenderTargetView(back_buffer, c, 0, nullptr);
                commandList->ClearDepthStencilView(depth_buffer, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
            }

            {
                //set the type of the parameters that we will use in the shader
                commandList->SetGraphicsRootSignature(m_root_signature.get());

                //set the raster pipeline state as a whole, it was prebuilt before
                commandList->SetPipelineState(m_triangle_state.get());

                uint32_t  w = m_deviceResources->SwapChainWidth();
                uint32_t  h = m_deviceResources->SwapChainHeight();
                
                //set the scissor test separately (which parts of the view port will survive)
                {
                    D3D12_RECT r = { 0, 0, static_cast<int32_t>(w), static_cast<int32_t>(h) };
                    commandList->RSSetScissorRects(1, &r);
                }

                //set the viewport. 
                {
                    D3D12_VIEWPORT v;
                    v.TopLeftX = 0;
                    v.TopLeftY = 0;
                    v.MinDepth = 0.0f;
                    v.MaxDepth = 1.0f;
                    v.Width = static_cast<float>(w);
                    v.Height = static_cast<float>(h);
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
                barrier.Transition.pResource = m_deviceResources->SwapChainBuffer(m_frame_index);
                barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
                barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
                barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                commandList->ResourceBarrier(1, &barrier);
            }
            
            commandList->Close();   //close the list

            {
                //form group of several command lists
                ID3D12CommandList* lists[] = { commandList };
                m_deviceResources->Queue()->ExecuteCommandLists(1, lists); //Execute what we have, submission of commands to the gpu
            }   

            const uint64_t fence_value = m_fence_value[m_frame_index];

            {
                //Tell the gpu to signal the cpu after it finishes executing the commands that we have just submitted
                m_deviceResources->SignalFenceValue(fence_value);
                m_deviceResources->WaitForFenceValue(fence_value);
                m_deviceResources->SwapChain()->Present(1, 0);    //present the swap chain
            }

            //prepare for the next frame
            m_frame_index = m_deviceResources->SwapChain()->GetCurrentBackBufferIndex();
            m_fence_value[m_frame_index] = fence_value + 1;
        }
    }

    void Load(winrt::hstring h)
    {
        ID3D12Device1* d = m_deviceResources->Device();
        m_root_signature = CreateRootSignature(d);

        //Compile many shader during the loading time of the app

        //use concurrency runtime 
        concurrency::task_group g;

        g.run( [this, d]
        {
            m_triangle_state = CreateTrianglePipelineState(d, m_root_signature.get());
        });

        g.run([this, d]
        {
            m_sampling_renderer_state = CreateSamplingRendererState(d, m_root_signature.get()); 
        });

        //let the waiting thread do some work also
        g.run_and_wait([this, d]
        {
            m_terrain_renderer_state = CreateTerrainRendererState(d, m_root_signature.get());  
        });
    }

    uint32_t align8(uint32_t value)
    {
        return (value + 7) & ~7;
    }

    void SetWindow(const CoreWindow& w)
    {
        m_closed			        = w.Closed(winrt::auto_revoke, { this, &ViewProvider::OnWindowClosed });
        m_size_changed		        = w.SizeChanged(winrt::auto_revoke, { this, &ViewProvider::OnWindowSizeChanged });

        auto envrionment            = sample::build_environment(w, winrt::Windows::Graphics::Display::DisplayInformation::GetForCurrentView());

        auto width                  = align8(static_cast<uint32_t>(envrionment.m_back_buffer_size.Width));
        auto height                 = align8(static_cast<uint32_t>(envrionment.m_back_buffer_size.Height));

        m_frame_index               = m_deviceResources->CreateSwapChain(w, width, height);

        //Create the sampling renderer
        sample::ResizeSamplingRendererContext ctx = {};

        ctx.m_device                = m_deviceResources->Device();
        ctx.m_width                 = width; 
        ctx.m_height                = height;
        ctx.m_depth_index           = 2;
        ctx.m_render_target_index   = 2;
        ctx.m_depth_heap            = m_deviceResources->DepthHeap();
        ctx.m_render_target_heap    = m_deviceResources->RenderTargetHeap();
        m_samplingRenderer->CreateSamplingRenderer(ctx);
    }

    void OnWindowClosed(const CoreWindow&w, const CoreWindowEventArgs& a)
    {
        m_window_running = false;
    }

    void OnActivated(const CoreApplicationView&, const IActivatedEventArgs&)
    {
        CoreWindow::GetForCurrentThread().Activate();
    }

    void OnWindowSizeChanged(const CoreWindow& window, const WindowSizeChangedEventArgs& a)
    {
        //wait for the render thread to finish and block it so we can submit a command
        std::lock_guard lock(m_blockRendering);

        //Now wait for the gpu to finish what it has from the main thread

        //Insert in the gpu a command after all submitted commands so far.
        const uint64_t fence_value = m_fence_value[m_frame_index];

        m_deviceResources->SignalFenceValue(fence_value);
        m_deviceResources->WaitForFenceValue(fence_value);

        auto envrionment = sample::build_environment(window, winrt::Windows::Graphics::Display::DisplayInformation::GetForCurrentView());

        auto w = align8(static_cast<uint32_t>(envrionment.m_back_buffer_size.Width));
        auto h = align8(static_cast<uint32_t>(envrionment.m_back_buffer_size.Height));

        {
            m_frame_index = m_deviceResources->ResizeBuffers(w, h);
        }

        {
            //Create the sampling renderer
            sample::ResizeSamplingRendererContext ctx = {};

            ctx.m_device = m_deviceResources->Device();
            ctx.m_width = w;
            ctx.m_height = h;
            ctx.m_depth_index = 2;
            ctx.m_render_target_index = 2;
            ctx.m_depth_heap = m_deviceResources->DepthHeap();
            ctx.m_render_target_heap = m_deviceResources->RenderTargetHeap();
            m_samplingRenderer->CreateSamplingRenderer(ctx);
        }

        //Prepare to unblock the rendering
        m_fence_value[m_frame_index] = fence_value + 1;


    }

    bool m_window_running = true;

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
    winrt::com_ptr< ID3D12PipelineState>		m_triangle_state;


    winrt::com_ptr< ID3D12PipelineState>		m_sampling_renderer_state;  //responsible for output of the parts that we want
    winrt::com_ptr< ID3D12PipelineState>		m_terrain_renderer_state;   //responsible to renderer the terrain
};

int32_t __stdcall wWinMain( HINSTANCE, HINSTANCE,PWSTR, int32_t )
{
    sample::ThrowIfFailed(CoInitializeEx(nullptr, COINIT_MULTITHREADED));
    CoreApplication::Run(ViewProvider());
    CoUninitialize();
    return 0;
}
