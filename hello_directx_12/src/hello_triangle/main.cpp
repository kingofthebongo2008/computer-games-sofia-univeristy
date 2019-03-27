#include "pch.h"
#include <cstdint>

#include <triangle_vertex.h>
#include <triangle_pixel.h>

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

static winrt::com_ptr<IDXGISwapChain1> CreateSwapChain(const CoreWindow& w, ID3D12CommandQueue* d)
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
	return r;
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

static winrt::com_ptr<ID3D12Resource1> CreateSwapChainResource(ID3D12Device1* device, uint32_t width, uint32_t height)
{
    D3D12_RESOURCE_DESC d               = DescribeSwapChain( width, height );

    winrt::com_ptr<ID3D12Resource1>     r;
    D3D12_HEAP_PROPERTIES p             = {};
    p.Type                              = D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_STATES       state   = D3D12_RESOURCE_STATE_RENDER_TARGET;
    
    ThrowIfFailed(device->CreateCommittedResource(&p, D3D12_HEAP_FLAG_NONE, &d, state, 0, __uuidof(ID3D12Resource1), r.put_void()));
    return r;
}

static void CreateSwapChainDescriptor(ID3D12Device1* device, ID3D12Resource1* resource, D3D12_CPU_DESCRIPTOR_HANDLE handle )
{
    D3D12_RENDER_TARGET_VIEW_DESC d = {};
    d.ViewDimension                 = D3D12_RTV_DIMENSION_TEXTURE2D;
    d.Format                        = DXGI_FORMAT_B8G8R8A8_UNORM;       //how we will view the resource during rendering
    device->CreateRenderTargetView(resource, &d, handle);
}



/*
static winrt::com_ptr <ID3D11PixelShader> CreateTrianglePixelShader(ID3D11Device3* device)
{
    winrt::com_ptr <ID3D11PixelShader> r;
	ThrowIfFailed(device->CreatePixelShader(g_triangle_pixel, sizeof(g_triangle_pixel), nullptr, r.put()));
	return r;
}

static winrt::com_ptr<ID3D11RasterizerState2> CreateRasterizerState(ID3D11Device3* device)
{
    winrt::com_ptr<ID3D11RasterizerState2> r;

	D3D11_RASTERIZER_DESC2 state = {};

	state.FillMode				= D3D11_FILL_SOLID;
	state.CullMode				= D3D11_CULL_NONE;
	state.FrontCounterClockwise = TRUE;
	state.DepthClipEnable		= TRUE;
	state.ScissorEnable			= TRUE;

	ThrowIfFailed(device->CreateRasterizerState2(&state, r.put()));
	return r;
}

static winrt::com_ptr<ID3D11BlendState1> CreateBlendState(ID3D11Device3* device)
{
    winrt::com_ptr<ID3D11BlendState1> r;

	D3D11_BLEND_DESC1 state = {};
	state.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	ThrowIfFailed(device->CreateBlendState1(&state, r.put()));
	return r;
}

static winrt::com_ptr<ID3D11DepthStencilState> CreateDepthStencilState(ID3D11Device3* device)
{
    winrt::com_ptr <ID3D11DepthStencilState> r;

	D3D11_DEPTH_STENCIL_DESC state = {};
	state.DepthEnable = FALSE;
	state.DepthFunc = D3D11_COMPARISON_LESS;
	ThrowIfFailed(device->CreateDepthStencilState(&state, r.put()));
	return r;
}
*/
class ViewProvider : public winrt::implements<ViewProvider, IFrameworkView, IFrameworkViewSource>
{
	public:

	IFrameworkView CreateView()
	{
			return *this;
	}

	void Initialize(const CoreApplicationView& v)
	{
		m_activated			= v.Activated(winrt::auto_revoke, { this, &ViewProvider::OnActivated });
        m_debug             = CreateDebug();
		m_device			= CreateDevice();

        m_fence             = CreateFence(m_device.get());
        m_queue             = CreateCommandQueue(m_device.get());

        m_descriptorHeap    = CreateDescriptorHeap(m_device.get());
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

            /*
			{
                winrt::com_ptr<ID3D11RenderTargetView1> m_swap_chain_view = CreateSwapChainView(m_swap_chain.get(), m_device.get());

				m_device_context->ClearState();

				{
					float clear_value[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
					m_device_context->ClearRenderTargetView(m_swap_chain_view.get(), clear_value);
				}

				{
					ID3D11RenderTargetView* views[1] = { m_swap_chain_view.get() };
					m_device_context->OMSetRenderTargets(1, views, nullptr);
					m_device_context->OMSetDepthStencilState(m_depth_stencil_state.get(), 0);
				}

				{
					m_device_context->OMSetBlendState(m_blend_state.get(), nullptr, 0xFFFFFFFF);
				}

				{
					m_device_context->RSSetState(m_rasterizer_state.get());

					D3D11_RECT r = { 0, 0, m_back_buffer_width, m_back_buffer_height };
					m_device_context->RSSetScissorRects(1, &r);
					

					D3D11_VIEWPORT v;
					v.TopLeftX = 0;
					v.TopLeftY = 0;
					v.MinDepth = 0.0f;
					v.MaxDepth = 1.0f;
					v.Width = static_cast<float>(m_back_buffer_width);
					v.Height = static_cast<float>(m_back_buffer_height);

					m_device_context->RSSetViewports(1, &v);
				}

				{
					m_device_context->VSSetShader(m_triangle_vertex.get(), nullptr, 0);
				}

				{
					m_device_context->PSSetShader(m_triangle_pixel.get(), nullptr, 0);
				}
				{
					m_device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				}

				{
					m_device_context->Draw(3, 0);
				}
			}
            */
			m_swap_chain->Present(0, 0);
		}
	}

	void Load(winrt::hstring h)
	{
        /*
		m_triangle_vertex = CreateTriangleVertexShader(m_device.get());
		m_triangle_pixel = CreateTrianglePixelShader(m_device.get());

		m_blend_state = CreateBlendState(m_device.get());
		m_rasterizer_state = CreateRasterizerState(m_device.get());
		m_depth_stencil_state = CreateDepthStencilState(m_device.get());
        */
	}

	void SetWindow(const CoreWindow& w)
	{
		m_closed			    = w.Closed(winrt::auto_revoke, { this, &ViewProvider::OnWindowClosed });
		m_size_changed		    = w.SizeChanged(winrt::auto_revoke, { this, &ViewProvider::OnWindowSizeChanged });

		m_swap_chain		    = CreateSwapChain(w, m_queue.get());

		m_back_buffer_width     = static_cast<UINT>(w.Bounds().Width);
		m_back_buffer_height    = static_cast<UINT>(w.Bounds().Height);

        //allocate memory for the view
        m_swap_chain_buffers[0] = CreateSwapChainResource(m_device.get(), m_back_buffer_width, m_back_buffer_height);
        m_swap_chain_buffers[1] = CreateSwapChainResource(m_device.get(), m_back_buffer_width, m_back_buffer_height);

        //create render target views, that will be used for rendering
        CreateSwapChainDescriptor(m_device.get(), m_swap_chain_buffers[0].get(), CpuView(m_device.get(), m_descriptorHeap.get()) + 0);
        CreateSwapChainDescriptor(m_device.get(), m_swap_chain_buffers[1].get(), CpuView(m_device.get(), m_descriptorHeap.get()) + 1);
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
        std::lock_guard lock(m_blockRendering);

		m_back_buffer_width = static_cast<UINT>(a.Size().Width);
		m_back_buffer_height = static_cast<UINT>(a.Size().Height);

        //allocate memory for the swap chain again
        m_swap_chain_buffers[0] = CreateSwapChainResource(m_device.get(), m_back_buffer_width, m_back_buffer_height);
        m_swap_chain_buffers[1] = CreateSwapChainResource(m_device.get(), m_back_buffer_width, m_back_buffer_height);

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
	
    winrt::com_ptr<ID3D12Debug>                 m_debug;
    winrt::com_ptr <ID3D12Device1>				m_device;           //device for gpu resources
    winrt::com_ptr <IDXGISwapChain1>			m_swap_chain;       //swap chain for 
    winrt::com_ptr <ID3D12PipelineState>		m_pipeline_state;   //pipeline state

    winrt::com_ptr <ID3D12Fence>        		m_fence;            //fence for cpu/gpu synchronization
    winrt::com_ptr <ID3D12CommandQueue>   		m_queue;            //queue to the device

    winrt::com_ptr <ID3D12DescriptorHeap>   	m_descriptorHeap;   //descriptor heap for the resources

    std::mutex                                  m_blockRendering;   //block render thread for the swap chain resizes

    winrt::com_ptr<ID3D12Resource1>             m_swap_chain_buffers[2];
    uint64_t                                    m_swap_chain_descriptors[2];

    uint32_t									m_back_buffer_width = 0;
	uint32_t									m_back_buffer_height = 0;

    uint64_t                                    m_frame_index = 0;
};

int32_t __stdcall wWinMain( HINSTANCE, HINSTANCE,PWSTR, int32_t )
{
	ThrowIfFailed(CoInitializeEx(nullptr, COINIT_MULTITHREADED));
	CoreApplication::Run(ViewProvider());
	CoUninitialize();
	return 0;
}
