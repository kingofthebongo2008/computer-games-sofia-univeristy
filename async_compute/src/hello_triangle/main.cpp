#include "pch.h"
#include <cstdint>

#include "d3dx12.h"

using namespace winrt::Windows::UI::Core;
using namespace winrt::Windows::ApplicationModel::Core;
using namespace winrt::Windows::ApplicationModel::Activation;
using namespace Microsoft::WRL;

//There are many steps required for dx12 triangle to get on the screen
//1. Swap Chain is needed to bind dx12 backbuffer output to the window management system
//2. Device is needed
//3. Command Queue is needed to submit commands
//4. Memory management for the back buffers.
//5. Fence is needed to synchronize cpu submission of commands and waiting of the results.
//6. For shaders. Pipeline State is needed to be setup
//7. For commands submission allocator and command buffer is needed.


namespace sample
{
    template <typename to, typename from> to* copy_to_abi_private(const from& w)
    {
        void* v = nullptr;
        winrt::copy_to_abi(w, v);

        return reinterpret_cast<to*>(v);
    }

    template <typename to, typename from> winrt::com_ptr<to> copy_to_abi(const from& w)
    {
        winrt::com_ptr<to> v;
        v.attach(sample::copy_to_abi_private<IUnknown>(w));
        return v;
    }
}

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
static winrt::com_ptr<ID3D12Debug> CreateDebug()
{
    winrt::com_ptr<ID3D12Debug> r;
    //check if you have installed debug layer, from the option windows components
    if ( D3D12GetDebugInterface(__uuidof(ID3D12Debug), r.put_void() ) == S_OK)
    {
        r->EnableDebugLayer();
    }
    return r;
}

static winrt::com_ptr<ID3D12Device4> CreateDevice()
{
    winrt::com_ptr<ID3D12Device4> r;

    //One can use d3d12 rendering with d3d11 capable hardware. You will just be missing new functionality.
    //Example, d3d12 on a D3D_FEATURE_LEVEL_9_1 hardare (as some phone are ).
    D3D_FEATURE_LEVEL features = D3D_FEATURE_LEVEL_11_1;
    ThrowIfFailed(D3D12CreateDevice(nullptr, features, __uuidof(ID3D12Device4), r.put_void()));
    return r.as<ID3D12Device4>();
}

static winrt::com_ptr<ID3D12CommandQueue> CreateGraphicsQueue(ID3D12Device* d )
{
    winrt::com_ptr<ID3D12CommandQueue> r;
    D3D12_COMMAND_QUEUE_DESC q = {};

    q.Type = D3D12_COMMAND_LIST_TYPE_DIRECT; //submit copy, raster, compute payloads
    ThrowIfFailed(d->CreateCommandQueue(&q, __uuidof(ID3D12CommandQueue), r.put_void()));
    return r;
}

static winrt::com_ptr<ID3D12CommandQueue> CreateComputeQueue(ID3D12Device* d)
{
	winrt::com_ptr<ID3D12CommandQueue> r;
	D3D12_COMMAND_QUEUE_DESC q = {};

	q.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE; //submit copy, raster, compute payloads
	ThrowIfFailed(d->CreateCommandQueue(&q, __uuidof(ID3D12CommandQueue), r.put_void()));
	return r;
}

static winrt::com_ptr<IDXGISwapChain3> CreateSwapChain(const CoreWindow& w, ID3D12CommandQueue* d)
{
    winrt::com_ptr<IDXGIFactory2> f;
    winrt::com_ptr<IDXGISwapChain1> r;
    
    ThrowIfFailed(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG,__uuidof(IDXGIFactory2), f.put_void()));

    DXGI_SWAP_CHAIN_DESC1 desc = {};

    desc.BufferCount	= 2;
    desc.Format			= DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.Width			= static_cast<UINT>(w.Bounds().Width);
    desc.Height			= static_cast<UINT>(w.Bounds().Height);
    desc.SampleDesc.Count = 1;
    desc.SwapEffect		= DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.BufferUsage	= DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.AlphaMode		= DXGI_ALPHA_MODE_IGNORE;
    desc.Scaling		= DXGI_SCALING_NONE;

    ThrowIfFailed(f->CreateSwapChainForCoreWindow(d, sample::copy_to_abi<IUnknown>(w).get(), &desc, nullptr, r.put()));
    return r.as< IDXGISwapChain3>();
}

static winrt::com_ptr <ID3D12Fence> CreateFence(ID3D12Device1* device, uint64_t initialValue = 1)
{
    winrt::com_ptr<ID3D12Fence> r;
    ThrowIfFailed(device->CreateFence(initialValue, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), r.put_void()));
    return r;
}

static winrt::com_ptr <ID3D12DescriptorHeap> CreateDescriptorHeapTargets(ID3D12Device1* device)
{
    winrt::com_ptr<ID3D12DescriptorHeap> r;
    D3D12_DESCRIPTOR_HEAP_DESC d = {};

    d.NumDescriptors = 2;
    d.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    device->CreateDescriptorHeap(&d, __uuidof(ID3D12DescriptorHeap), r.put_void());
    return r;
}

static winrt::com_ptr <ID3D12DescriptorHeap> CreateDescriptorHeapDepth(ID3D12Device1* device)
{
	winrt::com_ptr<ID3D12DescriptorHeap> r;
	D3D12_DESCRIPTOR_HEAP_DESC d = {};

	d.NumDescriptors = 2;
	d.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	device->CreateDescriptorHeap(&d, __uuidof(ID3D12DescriptorHeap), r.put_void());
	return r;
}

static winrt::com_ptr <ID3D12DescriptorHeap> CreateDescriptorHeapShader(ID3D12Device1* device)
{
	winrt::com_ptr<ID3D12DescriptorHeap> r;
	D3D12_DESCRIPTOR_HEAP_DESC d = {};

	d.NumDescriptors = 2;
	d.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	device->CreateDescriptorHeap(&d, __uuidof(ID3D12DescriptorHeap), r.put_void());
	return r;
}

static winrt::com_ptr <ID3D12DescriptorHeap> CreateDescriptorHeapShaderGpu(ID3D12Device1* device)
{
	winrt::com_ptr<ID3D12DescriptorHeap> r;
	D3D12_DESCRIPTOR_HEAP_DESC d = {};

	d.NumDescriptors = 2;
	d.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	d.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	device->CreateDescriptorHeap(&d, __uuidof(ID3D12DescriptorHeap), r.put_void());
	return r;
}


static winrt::com_ptr <ID3D12DescriptorHeap> CreateDescriptorHeapRendering(ID3D12Device1* device)
{
    winrt::com_ptr<ID3D12DescriptorHeap> r;
    D3D12_DESCRIPTOR_HEAP_DESC d = {};

    d.NumDescriptors = 2;
    d.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    device->CreateDescriptorHeap(&d, __uuidof(ID3D12DescriptorHeap), r.put_void());
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

static winrt::com_ptr<ID3D12Resource1> CreateSwapChainResource1(ID3D12Device1* device, uint32_t width, uint32_t height)
{
    D3D12_RESOURCE_DESC d               = DescribeSwapChain( width, height );

    winrt::com_ptr<ID3D12Resource1>     r;
    D3D12_HEAP_PROPERTIES p             = {};
    p.Type                              = D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_STATES       state   = D3D12_RESOURCE_STATE_PRESENT;

    D3D12_CLEAR_VALUE v = {};
    v.Color[0] = 1.0f;
    v.Format = DXGI_FORMAT_B8G8R8A8_UNORM;

    ThrowIfFailed(device->CreateCommittedResource(&p, D3D12_HEAP_FLAG_NONE, &d, state, &v, __uuidof(ID3D12Resource1), r.put_void()));
    return r;
}

static winrt::com_ptr<ID3D12Resource1> CreateDepthResource(ID3D12Device1* device, uint32_t width, uint32_t height)
{
	D3D12_RESOURCE_DESC d = {};
	d.Alignment = 0;
	d.DepthOrArraySize = 1;
	d.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	d.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	d.Format = DXGI_FORMAT_D32_FLOAT;
	d.Height = height;
	d.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	d.MipLevels = 1;
	d.SampleDesc.Count = 1;
	d.SampleDesc.Quality = 0;
	d.Width = width;

	winrt::com_ptr<ID3D12Resource1>     r;
	D3D12_HEAP_PROPERTIES p = {};
	p.Type = D3D12_HEAP_TYPE_DEFAULT;
	D3D12_RESOURCE_STATES       state = D3D12_RESOURCE_STATE_DEPTH_WRITE;

	D3D12_CLEAR_VALUE v = {};
	v.DepthStencil.Depth = 1.0f;
	v.Format = DXGI_FORMAT_D32_FLOAT;

	ThrowIfFailed(device->CreateCommittedResource(&p, D3D12_HEAP_FLAG_NONE, &d, state, &v, __uuidof(ID3D12Resource1), r.put_void()));
	return r;
}

//Create a gpu metadata that describes the swap chain, type, format. it will be used by the gpu interpret the data in the swap chain(reading/writing).
static void CreateDepthWriteDescriptor(ID3D12Device1* device, ID3D12Resource1* resource, D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
	D3D12_DEPTH_STENCIL_VIEW_DESC d = {};
	d.ViewDimension					= D3D12_DSV_DIMENSION_TEXTURE2D;
	d.Format						= DXGI_FORMAT_D32_FLOAT;       //how we will view the resource during rendering
	device->CreateDepthStencilView(resource, &d, handle);
}

static void CreateDepthReadDescriptor(ID3D12Device1* device, ID3D12Resource1* resource, D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
	D3D12_DEPTH_STENCIL_VIEW_DESC d = {};
	d.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	d.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH;
	d.Format = DXGI_FORMAT_D32_FLOAT;				//how we will view the resource during rendering
	device->CreateDepthStencilView(resource, &d, handle);
}


//Get the buffer for the swap chain, this is the end result for the window
static winrt::com_ptr<ID3D12Resource1> CreateSwapChainResource(ID3D12Device1* device, IDXGISwapChain* chain, uint32_t buffer)
{
    winrt::com_ptr<ID3D12Resource1> r;

    chain->GetBuffer(buffer, __uuidof(ID3D12Resource1), r.put_void());
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
static winrt::com_ptr <ID3D12CommandAllocator> CreateGraphicsAllocator(ID3D12Device1* device)
{
    winrt::com_ptr<ID3D12CommandAllocator> r;
    ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), r.put_void()));
    return r;
}

//Create the memory manager for the gpu commands
static winrt::com_ptr <ID3D12CommandAllocator> CreateComputeAllocator(ID3D12Device1* device)
{
	winrt::com_ptr<ID3D12CommandAllocator> r;
	ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, __uuidof(ID3D12CommandAllocator), r.put_void()));
	return r;
}

//create an object that will record commands
static winrt::com_ptr <ID3D12GraphicsCommandList1> CreateGraphicsList(ID3D12Device1* device, ID3D12CommandAllocator* a)
{
    winrt::com_ptr<ID3D12GraphicsCommandList1> r;
    ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, a, nullptr, __uuidof(ID3D12GraphicsCommandList1), r.put_void()));

    r->Close();
    return r;
}

//create an object that will record commands
static winrt::com_ptr <ID3D12GraphicsCommandList1> CreateComputeList(ID3D12Device1* device, ID3D12CommandAllocator* a)
{
	winrt::com_ptr<ID3D12GraphicsCommandList1> r;
	ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COMPUTE, a, nullptr, __uuidof(ID3D12GraphicsCommandList1), r.put_void()));

	r->Close();
	return r;
}

//create an object which represents what types of external data the shaders will use. You can imagine f(int x, float y); Root Signature is that we have two parameters on locations 0 and 1 types int and float
static winrt::com_ptr< ID3D12RootSignature>	 CreateRootSignature(ID3D12Device1* device)
{
    static 
    #include <default_graphics_signature.h>

    winrt::com_ptr<ID3D12RootSignature> r;
    ThrowIfFailed(device->CreateRootSignature( 0, &g_default_graphics_signature[0], sizeof(g_default_graphics_signature), __uuidof(ID3D12RootSignature), r.put_void()));
    return r;
}

D3D12_GRAPHICS_PIPELINE_STATE_DESC CreateTriangleDescription()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC state = {};
	state.SampleMask = UINT_MAX;
	state.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

	state.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	state.RasterizerState.FrontCounterClockwise = TRUE;

	state.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	state.NumRenderTargets = 1;
	state.RTVFormats[0] = DXGI_FORMAT_B8G8R8A8_UNORM;
	state.SampleDesc.Count = 1;
	state.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	state.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	state.DepthStencilState.DepthEnable = FALSE;
	state.DepthStencilState.StencilEnable = FALSE;
	state.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	state.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;
	return state;
}

//create a state for the rasterizer. that will be set a whole big monolitic block. Below the driver optimizes it in the most compact form for it. 
//It can be something as 16 DWORDS that gpu will read and trigger its internal rasterizer state
static winrt::com_ptr< ID3D12PipelineState>	 CreateTrianglePipelineState(ID3D12Device1* device, ID3D12RootSignature* root)
{
	static
#include <triangle_pixel.h>

		static
#include <triangle_vertex.h>

	D3D12_GRAPHICS_PIPELINE_STATE_DESC state = CreateTriangleDescription();
    state.pRootSignature			= root;
    state.NumRenderTargets			= 1;
    state.RTVFormats[0]				= DXGI_FORMAT_B8G8R8A8_UNORM;

    state.DepthStencilState.DepthEnable = TRUE;
	state.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	state.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;

    state.VS = { &g_triangle_vertex[0], sizeof(g_triangle_vertex) };
    state.PS = { &g_triangle_pixel[0], sizeof(g_triangle_pixel) };

    winrt::com_ptr<ID3D12PipelineState> r;

    ThrowIfFailed(device->CreateGraphicsPipelineState(&state, __uuidof(ID3D12PipelineState), r.put_void()));
    return r;
}

//create a state for the rasterizer. that will be set a whole big monolitic block. Below the driver optimizes it in the most compact form for it. 
//It can be something as 16 DWORDS that gpu will read and trigger its internal rasterizer state
static winrt::com_ptr< ID3D12PipelineState>	 CreateTriangleDepthPipelineState(ID3D12Device1* device, ID3D12RootSignature* root)
{
		static
#include <triangle_vertex.h>

	D3D12_GRAPHICS_PIPELINE_STATE_DESC state = CreateTriangleDescription();
	state.pRootSignature = root;

	state.DepthStencilState.DepthEnable		= TRUE;
	state.DepthStencilState.StencilEnable	= FALSE;
	state.DepthStencilState.DepthWriteMask	= D3D12_DEPTH_WRITE_MASK_ALL;
	state.DepthStencilState.DepthFunc		= D3D12_COMPARISON_FUNC_LESS;

	state.VS = { &g_triangle_vertex[0], sizeof(g_triangle_vertex) };
	
	winrt::com_ptr<ID3D12PipelineState> r;

	ThrowIfFailed(device->CreateGraphicsPipelineState(&state, __uuidof(ID3D12PipelineState), r.put_void()));
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

        m_graphics_queue			= CreateGraphicsQueue(m_device.get());
		m_compute_queue				= CreateComputeQueue(m_device.get());

		m_descriptorHeapTargets		= CreateDescriptorHeapTargets(m_device.get());
		m_descriptorHeapDepth		= CreateDescriptorHeapDepth(m_device.get());

		m_descriptorHeapShader		= CreateDescriptorHeapShader(m_device.get());
		m_descriptorHeapShaderGpu	= CreateDescriptorHeapShaderGpu(m_device.get());

        //if you have many threads that generate commands. 1 per thread per frame
        m_graphics_allocator[0]		= CreateGraphicsAllocator(m_device.get());
        m_graphics_allocator[1]		= CreateGraphicsAllocator(m_device.get());

        m_graphics_list[0]			= CreateGraphicsList(m_device.get(), m_graphics_allocator[0].get());
		m_graphics_list[1]			= CreateGraphicsList(m_device.get(), m_graphics_allocator[1].get());

		m_compute_allocator[0]		= CreateComputeAllocator(m_device.get());
		m_compute_allocator[1]		= CreateComputeAllocator(m_device.get());

		m_compute_list[0]			= CreateComputeList(m_device.get(), m_compute_allocator[0].get());
		m_compute_list[1]			= CreateComputeList(m_device.get(), m_compute_allocator[1].get());

        //fence, sync from the gpu and cpu
        m_graphics_fence			= CreateFence(m_device.get());
		m_graphics_event			= CreateEvent(nullptr, false, false, nullptr);

		m_compute_fence				= CreateFence(m_device.get());
		m_compute_event				= CreateEvent(nullptr, false, false, nullptr);
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
            ID3D12CommandAllocator*     allocator       = m_graphics_allocator[m_frame_index].get();
            ID3D12GraphicsCommandList1* commandList     = m_graphics_list[m_frame_index].get();
            allocator->Reset();
            commandList->Reset(allocator, nullptr);

            // Set Descriptor heaps
            {
                //ID3D12DescriptorHeap* heaps[] = { m_descriptorHeap.get()};
                //commandList->SetDescriptorHeaps(1, heaps);
            }

            //get the pointer to the gpu memory
            D3D12_CPU_DESCRIPTOR_HANDLE back_buffer = CpuView(m_device.get(), m_descriptorHeapTargets.get()) + m_swap_chain_descriptors[m_frame_index];

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
                commandList->SetGraphicsRootSignature(m_graphics_signature.get());

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
                m_graphics_queue->ExecuteCommandLists(1, lists); //Execute what we have, submission of commands to the gpu
            }   

            m_swap_chain->Present(1, 0);    //present the swap chain

            //Tell the gpu to signal the cpu after it finishes executing the commands that we have just submitted
            ThrowIfFailed(m_graphics_queue->Signal(m_graphics_fence.get(), m_fence_value));

            //Now block the cpu until the gpu completes the previous frame
            if (m_graphics_fence->GetCompletedValue() < m_fence_value)
            {
                ThrowIfFailed(m_graphics_fence->SetEventOnCompletion(m_fence_value, m_graphics_event));
                WaitForSingleObject(m_graphics_event, INFINITE);
            }

            //prepare for the next frame
            m_fence_value = m_fence_value + 1;
            m_frame_index = m_swap_chain->GetCurrentBackBufferIndex();
        }
    }

    void Load(winrt::hstring h)
    {
        m_graphics_signature			= CreateRootSignature(m_device.get());
        m_triangle_state				= CreateTrianglePipelineState(m_device.get(), m_graphics_signature.get());
		m_triangle_state_depth_prepass	= CreateTriangleDepthPipelineState(m_device.get(), m_graphics_signature.get());
    }

    void SetWindow(const CoreWindow& w)
    {
        m_closed			    = w.Closed(winrt::auto_revoke, { this, &ViewProvider::OnWindowClosed });
        m_size_changed		    = w.SizeChanged(winrt::auto_revoke, { this, &ViewProvider::OnWindowSizeChanged });

        m_swap_chain		    = CreateSwapChain(w, m_graphics_queue.get());
        m_frame_index           = m_swap_chain->GetCurrentBackBufferIndex();

        m_back_buffer_width     = static_cast<UINT>(w.Bounds().Width);
        m_back_buffer_height    = static_cast<UINT>(w.Bounds().Height);

        //allocate memory for the view
        m_swap_chain_buffers[0] = CreateSwapChainResource(m_device.get(), m_swap_chain.get(), 0);
        m_swap_chain_buffers[1] = CreateSwapChainResource(m_device.get(), m_swap_chain.get(), 1);

        m_swap_chain_buffers[0]->SetName(L"Buffer 0");
        m_swap_chain_buffers[1]->SetName(L"Buffer 1");

        //create render target views, that will be used for rendering
        CreateSwapChainDescriptor(m_device.get(), m_swap_chain_buffers[0].get(), CpuView(m_device.get(), m_descriptorHeapTargets.get()) + 0);
        CreateSwapChainDescriptor(m_device.get(), m_swap_chain_buffers[1].get(), CpuView(m_device.get(), m_descriptorHeapTargets.get()) + 1);

		m_depth_buffer		= CreateDepthResource(m_device.get(), m_back_buffer_width, m_back_buffer_height);

		CreateDepthWriteDescriptor(m_device.get(), m_depth_buffer.get(), CpuView(m_device.get(), m_descriptorHeapDepth.get()) + 0);
		CreateDepthReadDescriptor(m_device.get(), m_depth_buffer.get(), CpuView(m_device.get(), m_descriptorHeapDepth.get()) + 1);

		m_depth_descriptor[0] = 0;
		m_depth_descriptor[1] = 1;

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
        ThrowIfFailed(m_graphics_queue->Signal(m_graphics_fence.get(), m_fence_value));

        //Wait for the gpu to notify us back that it had passed. Now it is idle
        ThrowIfFailed(m_graphics_fence->SetEventOnCompletion(m_fence_value, m_graphics_event));
        WaitForSingleObject(m_graphics_event, INFINITE);

        //Prepare to unblock the rendering
        m_fence_value = m_fence_value + 1;
        m_frame_index = m_swap_chain->GetCurrentBackBufferIndex();

        //Now recreate the swap chain with the new dimensions, we must have back buffer as the window size
        m_back_buffer_width		= static_cast<UINT>(a.Size().Width);
        m_back_buffer_height	= static_cast<UINT>(a.Size().Height);

        //allocate memory for the swap chain again
        m_swap_chain_buffers[0] = CreateSwapChainResource(m_device.get(), m_swap_chain.get(), 0);
        m_swap_chain_buffers[1] = CreateSwapChainResource(m_device.get(), m_swap_chain.get(), 1);

        //set names so we can see them in pix
        m_swap_chain_buffers[0]->SetName(L"Buffer 0");
        m_swap_chain_buffers[1]->SetName(L"Buffer 1");

        //create render target views, that will be used for rendering
        CreateSwapChainDescriptor(m_device.get(), m_swap_chain_buffers[0].get(), CpuView(m_device.get(), m_descriptorHeapTargets.get()) + 0);
        CreateSwapChainDescriptor(m_device.get(), m_swap_chain_buffers[1].get(), CpuView(m_device.get(), m_descriptorHeapTargets.get()) + 1);

        m_swap_chain_descriptors[0] = 0;
        m_swap_chain_descriptors[1] = 1;
    }

    bool m_window_running = true;

    CoreWindow::Closed_revoker					m_closed;
    CoreWindow::SizeChanged_revoker				m_size_changed;
    CoreApplicationView::Activated_revoker		m_activated;
    
    winrt::com_ptr <ID3D12Debug>                m_debug;
    winrt::com_ptr <ID3D12Device1>				m_device;						//device for gpu resources
    winrt::com_ptr <IDXGISwapChain3>			m_swap_chain;					//swap chain for 

    winrt::com_ptr <ID3D12Fence>        		m_graphics_fence;				//fence for cpu/gpu synchronization
	winrt::com_ptr <ID3D12Fence>        		m_compute_fence;				//fence for compute queue

	winrt::com_ptr <ID3D12CommandQueue>   		m_compute_queue;				//queue to the device
	winrt::com_ptr <ID3D12CommandQueue>   		m_graphics_queue;				//queue to the device
	
    winrt::com_ptr <ID3D12DescriptorHeap>   	m_descriptorHeapTargets;		//descriptor heap for the resources
    winrt::com_ptr <ID3D12DescriptorHeap>   	m_descriptorHeapDepth;			//descriptor heap for the resources
	winrt::com_ptr <ID3D12DescriptorHeap>   	m_descriptorHeapShader;			//descriptor heap for the resources
	winrt::com_ptr <ID3D12DescriptorHeap>   	m_descriptorHeapShaderGpu;		//descriptor heap for the resources

    std::mutex                                  m_blockRendering;				//block render thread for the swap chain resizes

	winrt::com_ptr<ID3D12Resource1>             m_swap_chain_buffers[2];		//back buffer resources
	uint64_t                                    m_swap_chain_descriptors[2];
	winrt::com_ptr<ID3D12Resource1>				m_depth_buffer;
	uint64_t									m_depth_descriptor[2];

	winrt::com_ptr<ID3D12Resource1>             m_lighting_buffer[2];			//lighting buffers ( ComputeGraphicsLatency + 1 )
	uint64_t                                    m_lighting_buffer_descriptors[2];

	winrt::com_ptr<ID3D12Resource1>             m_depth;						//depth buffer

    uint32_t									m_back_buffer_width = 0;
    uint32_t									m_back_buffer_height = 0;

	winrt::com_ptr <ID3D12CommandAllocator>   	m_graphics_allocator[2];		//one per frame
	winrt::com_ptr <ID3D12CommandAllocator>   	m_compute_allocator[2];			//one per frame
    winrt::com_ptr <ID3D12GraphicsCommandList1> m_graphics_list[2];				//one per frame 
	winrt::com_ptr <ID3D12GraphicsCommandList1>	m_compute_list[2];				//one per frame 

	uint64_t                                    m_frame_index	= 0;
    uint64_t									m_fence_value	= 1;
    HANDLE										m_graphics_event = {};
	HANDLE										m_compute_event = {};

	//Signatures
    winrt::com_ptr< ID3D12RootSignature>		m_compute_signature;
	winrt::com_ptr< ID3D12RootSignature>		m_graphics_signature;

	//States
	winrt::com_ptr< ID3D12PipelineState>		m_triangle_state;
	winrt::com_ptr< ID3D12PipelineState>		m_triangle_state_depth_prepass;
	winrt::com_ptr< ID3D12PipelineState>		m_gaussian_blur;
};

int32_t __stdcall wWinMain( HINSTANCE, HINSTANCE,PWSTR, int32_t )
{
    ThrowIfFailed(CoInitializeEx(nullptr, COINIT_MULTITHREADED));
    CoreApplication::Run(ViewProvider());
    CoUninitialize();
    return 0;
}
