#include "pch.h"
#include <cstdint>

#include "d3dx12.h"
#include "build_window_environment.h"
#include "cpu_view.h"


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

	constexpr uint32_t color(uint8_t r, uint8_t g, uint8_t b)
	{
		return (0xFF << 24) | (b << 16) | (g << 8) | r;
	}
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


static winrt::com_ptr<ID3D12CommandQueue> CreateCommandQueue(ID3D12Device* d )
{
    winrt::com_ptr<ID3D12CommandQueue> r;
    D3D12_COMMAND_QUEUE_DESC q = {};

    q.Type = D3D12_COMMAND_LIST_TYPE_DIRECT; //submit copy, raster, compute payloads
    ThrowIfFailed(d->CreateCommandQueue(&q, __uuidof(ID3D12CommandQueue), r.put_void()));
    return r;
}

static winrt::com_ptr<IDXGISwapChain3> CreateSwapChain(const CoreWindow& w, ID3D12CommandQueue* d)
{
    winrt::com_ptr<IDXGIFactory2> f;
    winrt::com_ptr<IDXGISwapChain1> r;
    
    ThrowIfFailed(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG,__uuidof(IDXGIFactory2), f.put_void()));

    DXGI_SWAP_CHAIN_DESC1 desc = {};

	auto e = sample::build_environment(w, winrt::Windows::Graphics::Display::DisplayInformation::GetForCurrentView());
	
    desc.BufferCount	= 2;
    desc.Format			= DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.Width			= static_cast<UINT>(e.m_back_buffer_size.Width);
    desc.Height			= static_cast<UINT>(e.m_back_buffer_size.Height);
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

static winrt::com_ptr <ID3D12DescriptorHeap> CreateDescriptorHeap(ID3D12Device1* device)
{
    winrt::com_ptr<ID3D12DescriptorHeap> r;
    D3D12_DESCRIPTOR_HEAP_DESC d = {};

    d.NumDescriptors = 2;
    d.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    device->CreateDescriptorHeap(&d, __uuidof(ID3D12DescriptorHeap), r.put_void());
    return r;
}

static winrt::com_ptr <ID3D12DescriptorHeap> CreateDescriptorHeapShaders(ID3D12Device1* device)
{
    winrt::com_ptr<ID3D12DescriptorHeap> r;
    D3D12_DESCRIPTOR_HEAP_DESC d = {};

    d.NumDescriptors = 2;
    d.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    device->CreateDescriptorHeap(&d, __uuidof(ID3D12DescriptorHeap), r.put_void());
    return r;
}

static winrt::com_ptr <ID3D12DescriptorHeap> CreateDescriptorHeapShadersGpu(ID3D12Device1* device)
{
	winrt::com_ptr<ID3D12DescriptorHeap> r;
	D3D12_DESCRIPTOR_HEAP_DESC d = {};

	d.NumDescriptors = 2;
	d.Type			 = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	d.Flags			 = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
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

static winrt::com_ptr<ID3D12Resource1> CreateCubeTexture(ID3D12Device1* device, uint32_t width, uint32_t height, D3D12_RESOURCE_STATES state)
{
	D3D12_RESOURCE_DESC d	= {};
	d.Alignment				= 0;
	d.DepthOrArraySize		= 6;
	d.Dimension				= D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	d.Flags					= D3D12_RESOURCE_FLAG_NONE;
	d.Format				= DXGI_FORMAT_R8G8B8A8_UNORM;
	d.Height				= height;
	d.Layout				= D3D12_TEXTURE_LAYOUT_UNKNOWN;
	d.MipLevels				= 1;
	d.SampleDesc.Count		= 1;
	d.SampleDesc.Quality	= 0;
	d.Width = width;

	winrt::com_ptr<ID3D12Resource1>     r;
	D3D12_HEAP_PROPERTIES p = {};
	p.Type = D3D12_HEAP_TYPE_DEFAULT;

	ThrowIfFailed(device->CreateCommittedResource(&p, D3D12_HEAP_FLAG_NONE, &d, state, nullptr, __uuidof(ID3D12Resource1), r.put_void()));
	return r;
}

static winrt::com_ptr<ID3D12Resource1> CreateUploadCubeTexture(ID3D12Device1* device, uint64_t size, D3D12_RESOURCE_STATES state)
{
	D3D12_RESOURCE_DESC d = {};
	d.Alignment = 0;
	d.DepthOrArraySize = 1;
	d.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	d.Flags = D3D12_RESOURCE_FLAG_NONE;
	d.Format = DXGI_FORMAT_UNKNOWN;
	d.Height = 1;
	d.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	d.MipLevels = 1;
	d.SampleDesc.Count = 1;
	d.SampleDesc.Quality = 0;
	d.Width = size;

	winrt::com_ptr<ID3D12Resource1>     r;
	D3D12_HEAP_PROPERTIES p = {};
	p.Type = D3D12_HEAP_TYPE_UPLOAD;

	ThrowIfFailed(device->CreateCommittedResource(&p, D3D12_HEAP_FLAG_NONE, &d, state, nullptr, __uuidof(ID3D12Resource1), r.put_void()));
	return r;
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
static winrt::com_ptr <ID3D12CommandAllocator> CreateCommandAllocator(ID3D12Device1* device)
{
    winrt::com_ptr<ID3D12CommandAllocator> r;
    ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), r.put_void()));
    return r;
}

//Create the memory manager for the gpu commands
static winrt::com_ptr <ID3D12CommandAllocator> CreateBundleCommandAllocator(ID3D12Device1* device)
{
    winrt::com_ptr<ID3D12CommandAllocator> r;
    ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_BUNDLE, __uuidof(ID3D12CommandAllocator), r.put_void()));
    return r;
}

//create an object that will record commands
static winrt::com_ptr <ID3D12GraphicsCommandList1> CreateCommandList(ID3D12Device1* device, ID3D12CommandAllocator* a)
{
    winrt::com_ptr<ID3D12GraphicsCommandList1> r;
    ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, a, nullptr, __uuidof(ID3D12GraphicsCommandList1), r.put_void()));

    r->Close();
    return r;
}

//create an object that will record commands
static winrt::com_ptr <ID3D12GraphicsCommandList1> CreateBundleCommandList(ID3D12Device1* device, ID3D12CommandAllocator* a)
{
    winrt::com_ptr<ID3D12GraphicsCommandList1> r;
    ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_BUNDLE, a, nullptr, __uuidof(ID3D12GraphicsCommandList1), r.put_void()));

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

//create a state for the rasterizer. that will be set a whole big monolitic block. Below the driver optimizes it in the most compact form for it. 
//It can be something as 16 DWORDS that gpu will read and trigger its internal rasterizer state
static winrt::com_ptr< ID3D12PipelineState>	 CreateTrianglePipelineState(ID3D12Device1* device, ID3D12RootSignature* root)
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

    winrt::com_ptr<ID3D12PipelineState> r;

    ThrowIfFailed(device->CreateGraphicsPipelineState(&state, __uuidof(ID3D12PipelineState), r.put_void()));
    return r;
}


class MyViewProvider : public winrt::implements<MyViewProvider, IFrameworkView, IFrameworkViewSource>
{
    public:

    IFrameworkView CreateView()
    {
            return *this;
    }

    void Initialize(const CoreApplicationView& v)
    {
        m_activated					= v.Activated(winrt::auto_revoke, { this, &MyViewProvider::OnActivated });
        m_debug						= CreateDebug();
        m_device					= CreateDevice();

        m_queue					    = CreateCommandQueue(m_device.get());

        m_descriptorHeap		    = CreateDescriptorHeap(m_device.get());

		m_descriptorHeapShaders		= CreateDescriptorHeapShaders(m_device.get());
		m_descriptorHeapShadersGpu  = CreateDescriptorHeapShadersGpu(m_device.get());

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
            }

			//set the type of the parameters that we will use in the shader
			commandList->SetGraphicsRootSignature(m_root_signature.get());

			//set the type of the parameters that we will use in the shader
			{
				ID3D12DescriptorHeap* heaps[1] = { m_descriptorHeapShadersGpu.get() };

				commandList->SetDescriptorHeaps(1, heaps);
			}

			{
				DescriptorHeapGpuView gpu = GpuView(m_device.get(), m_descriptorHeapShadersGpu.get());
				commandList->SetGraphicsRootDescriptorTable(0, gpu(0));
			}


			//set the raster pipeline state as a whole, it was prebuilt before
			commandList->SetPipelineState(m_triangle_state.get());

			float constants[4] = {};

			constants[0] = static_cast<float>(m_back_buffer_width);
			constants[1] = static_cast<float>(m_back_buffer_height);
			constants[2] = static_cast<float>(0);
			constants[3] = static_cast<float>(0);

			commandList->SetGraphicsRoot32BitConstants(1, 4, &constants[0], 0);

			//set the types of the triangles we will use
			commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			//draw the triangle
			commandList->DrawInstanced(3, 1, 0, 0);


            //insert pregenerated commands
            //commandList->ExecuteBundle(m_bundle_command_list.get());

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
        //Loading time
        m_root_signature = CreateRootSignature(m_device.get());
        m_triangle_state = CreateTrianglePipelineState(m_device.get(), m_root_signature.get());

        m_bundle_allocator = CreateBundleCommandAllocator(m_device.get());
        m_bundle_command_list = CreateBundleCommandList(m_device.get(), m_bundle_allocator.get());

		m_cube_texture = CreateCubeTexture(m_device.get(), 64, 64, D3D12_RESOURCE_STATE_COPY_DEST);

		{
			DescriptorHeapCpuView cpu = CpuView(m_device.get(), m_descriptorHeapShaders.get());

			D3D12_SHADER_RESOURCE_VIEW_DESC d = {};

			d.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			d.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
			d.TextureCube.MipLevels = 1;
			d.TextureCube.MostDetailedMip = 0;

			d.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			m_device->CreateShaderResourceView(m_cube_texture.get(), &d, cpu(0));
		}

		winrt::com_ptr <ID3D12CommandAllocator>   	upload_allocator	= CreateCommandAllocator(m_device.get());
		winrt::com_ptr <ID3D12GraphicsCommandList1> upload_list			= CreateCommandList(m_device.get(), upload_allocator.get());
		winrt::com_ptr <ID3D12Resource1 >			upload_cube_texture = CreateUploadCubeTexture(m_device.get(), GetRequiredIntermediateSize(m_cube_texture.get(), 0, 6), D3D12_RESOURCE_STATE_GENERIC_READ);

		upload_allocator->Reset();
		upload_list->Reset(upload_allocator.get(), nullptr);

		uint32_t colors[6] =
		{
			sample::color(255, 0,0),
			sample::color(0, 255,0),
			sample::color(0, 0,255),
			sample::color(127, 0,0),
			sample::color(0, 127,0),
			sample::color(0, 0,127)
		};

		std::vector<uint32_t> pixels(6 * 64 * 64, 0);

		D3D12_SUBRESOURCE_DATA d[6] = {};

		for (uint32_t i = 0; i < 6; ++i)
		{
			uint32_t* p = &pixels[64 * 64 * i];
			d[i].pData = p;
			d[i].RowPitch = 64 * 4;
			d[i].SlicePitch = 64 * 64 * 4;

			for (auto k = 0; k < 64 * 64; ++k)
			{
				*p++ = colors[i];
			}
		}

		UpdateSubresources(upload_list.get(), m_cube_texture.get(), upload_cube_texture.get(), 0, 0, 6U, &d[0]);

		{
			D3D12_RESOURCE_BARRIER b[6];

			for (uint32_t i = 0; i < 6; ++i)
			{
				b[i].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
				b[i].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				b[i].Transition.pResource = m_cube_texture.get();
				b[i].Transition.Subresource = i;
				b[i].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
				b[i].Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
			}
			upload_list->ResourceBarrier(6, &b[0]);
		}


		upload_list->Close();   //close the list

		{
			//form group of several command lists
			ID3D12CommandList* lists[] = { upload_list.get() };
			m_queue->ExecuteCommandLists(1, lists); //Execute what we have, submission of commands to the gpu
		}

		//Tell the gpu to signal the cpu after it finishes executing the commands that we have just submitted
		ThrowIfFailed(m_queue->Signal(m_fence.get(), m_fence_value + 1));

		//Now block the cpu until the gpu completes the previous frame
		if (m_fence->GetCompletedValue() < m_fence_value + 2)
		{
			ThrowIfFailed(m_fence->SetEventOnCompletion(m_fence_value + 1, m_fence_event));
			WaitForSingleObject(m_fence_event, INFINITE);
		}

		m_fence_value = m_fence_value + 2;
		{
			DescriptorHeapCpuView cpu = CpuView(m_device.get(), m_descriptorHeapShaders.get());
			DescriptorHeapCpuView gpu = CpuView(m_device.get(), m_descriptorHeapShadersGpu.get());
			
			m_device->CopyDescriptorsSimple(1, gpu(0), cpu(0), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}

		GenerateBundle();
	}

    void SetWindow(const CoreWindow& w)
    {
        m_closed			    = w.Closed(winrt::auto_revoke, { this, &MyViewProvider::OnWindowClosed });
        m_size_changed		    = w.SizeChanged(winrt::auto_revoke, { this, &MyViewProvider::OnWindowSizeChanged });

        m_swap_chain		    = CreateSwapChain(w, m_queue.get());
        m_frame_index           = m_swap_chain->GetCurrentBackBufferIndex();

		auto e					= sample::build_environment(w, winrt::Windows::Graphics::Display::DisplayInformation::GetForCurrentView());

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
        ThrowIfFailed(m_queue->Signal(m_fence.get(), m_fence_value + 1));

        //Wait for the gpu to notify us back that it had passed. Now it is idle
        ThrowIfFailed(m_fence->SetEventOnCompletion(m_fence_value + 1, m_fence_event));
        WaitForSingleObject(m_fence_event, INFINITE);

        //Prepare to unblock the rendering
        m_fence_value			= m_fence_value + 1;
        m_frame_index			= 0;

		auto e = sample::build_environment(w, winrt::Windows::Graphics::Display::DisplayInformation::GetForCurrentView());

        //Now recreate the swap chain with the new dimensions, we must have back buffer as the window size
        m_back_buffer_width		= static_cast<UINT>(e.m_back_buffer_size.Width);
		m_back_buffer_height	= static_cast<UINT>(e.m_back_buffer_size.Height);

		m_swap_chain_buffers[0] = nullptr;
		m_swap_chain_buffers[1] = nullptr;

		ThrowIfFailed(m_swap_chain->ResizeBuffers(2, m_back_buffer_width, m_back_buffer_height, DXGI_FORMAT_B8G8R8A8_UNORM, 0 ));

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

		GenerateBundle();
    }

	void GenerateBundle()
	{
		m_bundle_allocator->Reset();

		//Record the drawing
		//Usually up to ten draw calls does not make sense, but if you can do more than that will add up
		//This may involve drawing a character.
		{
			m_bundle_command_list->Reset(m_bundle_allocator.get(), nullptr);

			//set the type of the parameters that we will use in the shader
			m_bundle_command_list->SetGraphicsRootSignature(m_root_signature.get());

			//set the raster pipeline state as a whole, it was prebuilt before
			m_bundle_command_list->SetPipelineState(m_triangle_state.get());

			float constants[4] = {};

			constants[0] = static_cast<float>(m_back_buffer_width);
			constants[1] = static_cast<float>(m_back_buffer_height);
			constants[2] = static_cast<float>(0);
			constants[3] = static_cast<float>(0);

			m_bundle_command_list->SetGraphicsRoot32BitConstants(1, 4, &constants[0], 0);

			//set the types of the triangles we will use
			m_bundle_command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			//draw the triangle
			m_bundle_command_list->DrawInstanced(3, 1, 0, 0);

			m_bundle_command_list->Close();
		}


	}

    bool m_window_running = true;

    CoreWindow::Closed_revoker					m_closed;
    CoreWindow::SizeChanged_revoker				m_size_changed;
    CoreApplicationView::Activated_revoker		m_activated;
    
    winrt::com_ptr <ID3D12Debug>                m_debug;
    winrt::com_ptr <ID3D12Device1>				m_device;						//device for gpu resources
    winrt::com_ptr <IDXGISwapChain3>			m_swap_chain;					//swap chain for 

    winrt::com_ptr <ID3D12Fence>        		m_fence;						//fence for cpu/gpu synchronization
    winrt::com_ptr <ID3D12CommandQueue>   		m_queue;						//queue to the device

    winrt::com_ptr <ID3D12DescriptorHeap>   	m_descriptorHeap;				//descriptor heap for the resources

    winrt::com_ptr <ID3D12DescriptorHeap>   	m_descriptorHeapShaders;		//descriptor heap for the resources
	winrt::com_ptr <ID3D12DescriptorHeap>   	m_descriptorHeapShadersGpu;		//descriptor heap for the gpu resources

    std::mutex                                  m_blockRendering;				//block render thread for the swap chain resizes

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

    winrt::com_ptr <ID3D12CommandAllocator>   	m_bundle_allocator;		    //one per frame
    winrt::com_ptr <ID3D12GraphicsCommandList1> m_bundle_command_list;		//one per frame

	winrt::com_ptr< ID3D12Resource1 >			m_cube_texture;
};

int32_t __stdcall wWinMain( HINSTANCE, HINSTANCE,PWSTR, int32_t )
{
    ThrowIfFailed(CoInitializeEx(nullptr, COINIT_MULTITHREADED));
    CoreApplication::Run(MyViewProvider());
    CoUninitialize();
    return 0;
}
