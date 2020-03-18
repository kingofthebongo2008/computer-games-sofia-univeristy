#include "pch.h"
#include <cstdint>
#include "build_window_environment.h"

#include "d3dx12.h"


using namespace winrt::Windows::UI::Input;
using namespace winrt::Windows::UI::Core;
using namespace winrt::Windows::ApplicationModel::Core;
using namespace winrt::Windows::ApplicationModel::Activation;
using namespace Microsoft::WRL;
using namespace DirectX;

//There are many steps required for dx12 triangle to get on the screen
//1. Swap Chain is needed to bind dx12 backbuffer output to the window management system
//2. Device is needed
//3. Command Queue is needed to submit commands
//4. Memory management for the back buffers.
//5. Fence is needed to synchronize cpu submission of commands and waiting of the results.
//6. For shaders. Pipeline State is needed to be setup
//7. For commands submission allocator and command buffer is needed.

struct AABB
{
    XMVECTOR m_min;
    XMVECTOR m_max;
};

struct FrameConstants
{
    XMMATRIX m_view;
    XMMATRIX m_projection;
    XMVECTOR m_position;
};

namespace lispsm
{
    float Pi()
    {
        return 3.14159265358979323846f;
    }

    float radians(float degrees)
    {
        return ((degrees) / 180.0f) * Pi();
    }
}

void triangulate_aabb(const AABB aabb, XMVECTOR* points);
void aabb_points(const AABB aabb, XMVECTOR* points);

namespace
{
    uint64_t align(uint64_t v, uint64_t a)
    {
        return (v + a - 1) & (~(a - 1));
    }
}

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

//Helper class that assists us using the descriptors
struct DescriptorHeapGpuView
{
    DescriptorHeapGpuView(D3D12_GPU_DESCRIPTOR_HANDLE  base, uint64_t offset) : m_base(base), m_offset(offset)
    {

    }

    D3D12_GPU_DESCRIPTOR_HANDLE operator () (size_t index) const
    {
        return { m_base.ptr + index * m_offset };
    }

    D3D12_GPU_DESCRIPTOR_HANDLE operator + (size_t index) const
    {
        return { m_base.ptr + index * m_offset };
    }

    D3D12_GPU_DESCRIPTOR_HANDLE m_base = {};
    uint64_t                    m_offset;
};


DescriptorHeapCpuView CpuView( ID3D12Device* d, ID3D12DescriptorHeap* heap )
{
    D3D12_DESCRIPTOR_HEAP_DESC desc = heap->GetDesc();
    return DescriptorHeapCpuView(heap->GetCPUDescriptorHandleForHeapStart(), d->GetDescriptorHandleIncrementSize(desc.Type));
}

DescriptorHeapGpuView GpuView(ID3D12Device* d, ID3D12DescriptorHeap* heap)
{
    D3D12_DESCRIPTOR_HEAP_DESC desc = heap->GetDesc();
    return DescriptorHeapGpuView(heap->GetGPUDescriptorHandleForHeapStart(), d->GetDescriptorHandleIncrementSize(desc.Type));
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
    desc.Format			= DXGI_FORMAT_R8G8B8A8_UNORM;
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

static winrt::com_ptr <ID3D12DescriptorHeap> CreateDescriptorHeapRTV(ID3D12Device1* device)
{
    winrt::com_ptr<ID3D12DescriptorHeap> r;
    D3D12_DESCRIPTOR_HEAP_DESC d = {};

    d.NumDescriptors = 3;
    d.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    device->CreateDescriptorHeap(&d, __uuidof(ID3D12DescriptorHeap), r.put_void());
    return r;
}

static winrt::com_ptr <ID3D12DescriptorHeap> CreateDescriptorHeapSRV(ID3D12Device1* device)
{
    winrt::com_ptr<ID3D12DescriptorHeap> r;
    D3D12_DESCRIPTOR_HEAP_DESC d = {};

    d.NumDescriptors = 3;
    d.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    device->CreateDescriptorHeap(&d, __uuidof(ID3D12DescriptorHeap), r.put_void());
    return r;
}

static winrt::com_ptr <ID3D12DescriptorHeap> CreateDescriptorHeapDSV(ID3D12Device1* device)
{
    winrt::com_ptr<ID3D12DescriptorHeap> r;
    D3D12_DESCRIPTOR_HEAP_DESC d = {};

    d.NumDescriptors = 2;
    d.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
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
    d.Format                = DXGI_FORMAT_R8G8B8A8_TYPELESS;     //important for computing the resource footprint
    d.Height                = height;
    d.Layout                = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    d.MipLevels             = 1;
    d.SampleDesc.Count      = 1;
    d.SampleDesc.Quality    = 0;
    d.Width                 = width;
    return                  d;
}


//compute sizes
static D3D12_RESOURCE_DESC DescribeFrameBuffer(uint32_t width, uint32_t height)
{
    D3D12_RESOURCE_DESC d   = {};
    d.Alignment             = 0;
    d.DepthOrArraySize      = 1;
    d.Dimension             = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    d.Flags                 = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    d.Format                = DXGI_FORMAT_R8G8B8A8_TYPELESS;     //important for computing the resource footprint
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
    v.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

    ThrowIfFailed(device->CreateCommittedResource(&p, D3D12_HEAP_FLAG_NONE, &d, state, &v, __uuidof(ID3D12Resource1), r.put_void()));
    return r;
}

static winrt::com_ptr<ID3D12Resource1> CreateFrameBuffer(ID3D12Device1* device, uint32_t width, uint32_t height)
{
    D3D12_RESOURCE_DESC d = DescribeFrameBuffer(width, height);

    winrt::com_ptr<ID3D12Resource1>     r;
    D3D12_HEAP_PROPERTIES p = {};
    p.Type = D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_STATES       state = D3D12_RESOURCE_STATE_RENDER_TARGET;

    D3D12_CLEAR_VALUE v = {};
    v.Color[0] = 1.0f;
    v.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

    ThrowIfFailed(device->CreateCommittedResource(&p, D3D12_HEAP_FLAG_NONE, &d, state, &v, __uuidof(ID3D12Resource1), r.put_void()));
    return r;
}

//Get the buffer for the swap chain, this is the end result for the window
static winrt::com_ptr<ID3D12Resource1> CreateSwapChainResource(ID3D12Device1* device, IDXGISwapChain* chain, uint32_t buffer)
{
    winrt::com_ptr<ID3D12Resource1> r;

    chain->GetBuffer(buffer, __uuidof(ID3D12Resource1), r.put_void());
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
    v.DepthStencil.Depth = 0.0f;
    v.Format = DXGI_FORMAT_D32_FLOAT;

    ThrowIfFailed(device->CreateCommittedResource(&p, D3D12_HEAP_FLAG_NONE, &d, state, &v, __uuidof(ID3D12Resource1), r.put_void()));
    return r;
}

//Create a gpu metadata that describes the swap chain, type, format. it will be used by the gpu interpret the data in the swap chain(reading/writing).
static void CreateDepthWriteDescriptor(ID3D12Device1* device, ID3D12Resource1* resource, D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
    D3D12_DEPTH_STENCIL_VIEW_DESC d = {};
    d.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    d.Format = DXGI_FORMAT_D32_FLOAT;       //how we will view the resource during rendering
    device->CreateDepthStencilView(resource, &d, handle);
}

static void CreateDepthReadDescriptor(ID3D12Device1* device, ID3D12Resource1* resource, D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
    D3D12_DEPTH_STENCIL_VIEW_DESC d = {};
    d.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    d.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH;
    d.Format = DXGI_FORMAT_D32_FLOAT;               //how we will view the resource during rendering
    device->CreateDepthStencilView(resource, &d, handle);
}

//compute sizes
static D3D12_RESOURCE_DESC DescribeDebugBuffer(uint32_t width, uint32_t height)
{
    D3D12_RESOURCE_DESC d = {};
    d.Alignment = 0;
    d.DepthOrArraySize = 1;
    d.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    d.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    d.Format = DXGI_FORMAT_R32G32B32A32_TYPELESS;     //important for computing the resource footprint
    d.Height = height;
    d.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    d.MipLevels = 1;
    d.SampleDesc.Count = 1;
    d.SampleDesc.Quality = 0;
    d.Width = width;
    return                  d;
}

static winrt::com_ptr<ID3D12Resource1> CreateDebugBuffer1(ID3D12Device1* device, uint32_t width, uint32_t height)
{
    D3D12_RESOURCE_DESC d = DescribeDebugBuffer(width, height);

    winrt::com_ptr<ID3D12Resource1>     r;
    D3D12_HEAP_PROPERTIES p = {};
    p.Type = D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_STATES       state = D3D12_RESOURCE_STATE_RENDER_TARGET;

    D3D12_CLEAR_VALUE v = {};
    v.Color[0] = 1.0f;
    v.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;

    ThrowIfFailed(device->CreateCommittedResource(&p, D3D12_HEAP_FLAG_NONE, &d, state, &v, __uuidof(ID3D12Resource1), r.put_void()));
    return r;
}

static winrt::com_ptr<ID3D12Heap> CreateUploadHeap(ID3D12Device1* device)
{
    winrt::com_ptr<ID3D12Heap>     r;
    D3D12_HEAP_DESC p = {};

    p.Properties.Type   = D3D12_HEAP_TYPE_UPLOAD;
    p.SizeInBytes       = 4 * 1024 * 1024;
    p.Flags             = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;

    ThrowIfFailed(device->CreateHeap(&p, __uuidof(ID3D12Heap), r.put_void()));
    return r;
}

static winrt::com_ptr<ID3D12Heap> CreateGeometryHeap(ID3D12Device1* device)
{
    winrt::com_ptr<ID3D12Heap>     r;
    D3D12_HEAP_DESC p = {};

    p.Properties.Type   = D3D12_HEAP_TYPE_DEFAULT;
    p.SizeInBytes       = 4 * 1024 * 1024;
    p.Flags             = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;

    ThrowIfFailed(device->CreateHeap(&p, __uuidof(ID3D12Heap), r.put_void()));
    return r;
}

static winrt::com_ptr<ID3D12Resource1> CreateGeometry(ID3D12Device1* device, ID3D12Heap* heap, size_t size)
{
    winrt::com_ptr<ID3D12Resource1>     r;
    D3D12_RESOURCE_DESC d = {};

    d.Width = (size + 3) & (~3);
    d.Height = 1;
    d.DepthOrArraySize = 1;
    d.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    d.Format = DXGI_FORMAT_UNKNOWN;
    d.MipLevels = 1;
    d.SampleDesc = { 1,0 };
    d.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    ThrowIfFailed(device->CreatePlacedResource(heap, 0, &d, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, __uuidof(ID3D12Resource1), r.put_void()));
    return r;
}

static winrt::com_ptr<ID3D12Resource1> CreateUploadGeometry(ID3D12Device1* device, ID3D12Heap* heap, size_t size)
{
    winrt::com_ptr<ID3D12Resource1>     r;
    D3D12_RESOURCE_DESC d = {};

    d.Width = (size + 3) & (~3);
    d.Height = 1;
    d.DepthOrArraySize = 1;
    d.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    d.Format = DXGI_FORMAT_UNKNOWN;
    d.MipLevels = 1;
    d.SampleDesc = { 1,0 };
    d.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    ThrowIfFailed(device->CreatePlacedResource(heap, 0, &d, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, __uuidof(ID3D12Resource1), r.put_void()));
    return r;
}

//Create a gpu metadata that describes the swap chain, type, format. it will be used by the gpu interpret the data in the swap chain(reading/writing).
static void CreateDebugBuffer1Descriptior(ID3D12Device1* device, ID3D12Resource1* resource, D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
    D3D12_RENDER_TARGET_VIEW_DESC d = {};
    d.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    d.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;       //how we will view the resource during rendering
    device->CreateRenderTargetView(resource, &d, handle);
}

//Create a gpu metadata that describes the swap chain, type, format. it will be used by the gpu interpret the data in the swap chain(reading/writing).
static void CreateSwapChainDescriptor(ID3D12Device1* device, ID3D12Resource1* resource, D3D12_CPU_DESCRIPTOR_HANDLE handle )
{
    D3D12_RENDER_TARGET_VIEW_DESC d = {};
    d.ViewDimension                 = D3D12_RTV_DIMENSION_TEXTURE2D;
    d.Format                        = DXGI_FORMAT_R8G8B8A8_UNORM;       //how we will view the resource during rendering
    device->CreateRenderTargetView(resource, &d, handle);
}

//Create the memory manager for the gpu commands
static winrt::com_ptr <ID3D12CommandAllocator> CreateCommandAllocator(ID3D12Device1* device)
{
    winrt::com_ptr<ID3D12CommandAllocator> r;
    ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), r.put_void()));
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

    state.RasterizerState.DepthClipEnable = FALSE;
    state.RasterizerState.CullMode	= D3D12_CULL_MODE_NONE;
    state.RasterizerState.FrontCounterClockwise = TRUE;

    state.PrimitiveTopologyType		= D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    state.NumRenderTargets			= 1;
    state.RTVFormats[0]				= DXGI_FORMAT_R32G32B32A32_FLOAT;
    state.SampleDesc.Count			= 1;
    state.BlendState				= CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    

    state.DSVFormat                         = DXGI_FORMAT_D32_FLOAT;
    state.DepthStencilState.DepthEnable     = TRUE;
    state.DepthStencilState.StencilEnable   = FALSE;
    state.DepthStencilState.DepthWriteMask  = D3D12_DEPTH_WRITE_MASK_ALL;
    state.DepthStencilState.DepthFunc       = D3D12_COMPARISON_FUNC_GREATER;

    state.VS = { &g_triangle_vertex[0], sizeof(g_triangle_vertex) };
    state.PS = { &g_triangle_pixel[0], sizeof(g_triangle_pixel) };

    winrt::com_ptr<ID3D12PipelineState> r;

    ThrowIfFailed(device->CreateGraphicsPipelineState(&state, __uuidof(ID3D12PipelineState), r.put_void()));
    return r;
}

//create a state for the rasterizer. that will be set a whole big monolitic block. Below the driver optimizes it in the most compact form for it. 
//It can be something as 16 DWORDS that gpu will read and trigger its internal rasterizer state
static winrt::com_ptr< ID3D12PipelineState>	 CreateAabbPipelineState(ID3D12Device1* device, ID3D12RootSignature* root)
{
    static
    #include <aabb_pixel.h>

    static
    #include <aabb_vertex.h>

    D3D12_GRAPHICS_PIPELINE_STATE_DESC state = {};
    state.pRootSignature = root;
    state.SampleMask = UINT_MAX;
    state.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

    state.RasterizerState.DepthClipEnable = FALSE;
    state.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    state.RasterizerState.FrontCounterClockwise = TRUE;

    state.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    state.NumRenderTargets = 1;
    state.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    state.SampleDesc.Count = 1;

    state.BlendState.AlphaToCoverageEnable          = FALSE;
    state.BlendState.IndependentBlendEnable         = FALSE;
    
    state.BlendState.RenderTarget[0].BlendEnable    = TRUE;
    state.BlendState.RenderTarget[0].LogicOpEnable  = FALSE;
    state.BlendState.RenderTarget[0].SrcBlend       = D3D12_BLEND_ONE;
    state.BlendState.RenderTarget[0].DestBlend      = D3D12_BLEND_INV_SRC_ALPHA;
    state.BlendState.RenderTarget[0].BlendOp        = D3D12_BLEND_OP_ADD;
    state.BlendState.RenderTarget[0].SrcBlendAlpha  = D3D12_BLEND_ONE;
    state.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
    state.BlendState.RenderTarget[0].BlendOpAlpha   = D3D12_BLEND_OP_ADD;
    state.BlendState.RenderTarget[0].LogicOp        = D3D12_LOGIC_OP_NOOP;
    state.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;


    state.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    state.DepthStencilState.DepthEnable = FALSE;
    state.DepthStencilState.StencilEnable = FALSE;
    state.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    state.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER;

    state.VS = { &g_aabb_vertex[0], sizeof(g_aabb_vertex) };
    state.PS = { &g_aabb_pixel[0], sizeof(g_aabb_pixel) };

    // Vertex struct holding position and color information.
    const D3D12_INPUT_ELEMENT_DESC d[] =
    {
        { "POSITION",    0, DXGI_FORMAT_R32G32B32A32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };


    state.InputLayout.NumElements = 1;
    state.InputLayout.pInputElementDescs = &d[0];

    winrt::com_ptr<ID3D12PipelineState> r;
    ThrowIfFailed(device->CreateGraphicsPipelineState(&state, __uuidof(ID3D12PipelineState), r.put_void()));
    return r;
}


//create a state for the rasterizer. that will be set a whole big monolitic block. Below the driver optimizes it in the most compact form for it. 
//It can be something as 16 DWORDS that gpu will read and trigger its internal rasterizer state
static winrt::com_ptr< ID3D12PipelineState>	 CreateSpheresPipelineState(ID3D12Device1* device, ID3D12RootSignature* root)
{
    static
    #include <spheres_pixel.h>

    static
    #include <spheres_vertex.h>

    static
    #include <spheres_geometry.h>

    D3D12_GRAPHICS_PIPELINE_STATE_DESC state = {};
    state.pRootSignature = root;
    state.SampleMask = UINT_MAX;
    state.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

    state.RasterizerState.DepthClipEnable = FALSE;
    state.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    state.RasterizerState.FrontCounterClockwise = TRUE;

    state.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
    state.NumRenderTargets = 1;
    state.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    state.SampleDesc.Count = 1;

    state.BlendState.AlphaToCoverageEnable = FALSE;
    state.BlendState.IndependentBlendEnable = FALSE;

    state.BlendState.RenderTarget[0].BlendEnable = FALSE;
    state.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
    state.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
    state.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    state.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    state.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    state.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
    state.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    state.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
    state.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    state.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    state.DepthStencilState.DepthEnable = FALSE;
    state.DepthStencilState.StencilEnable = FALSE;
    state.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    state.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER;

    state.VS = { &g_spheres_vertex[0],      sizeof(g_spheres_vertex) };
    state.PS = { &g_spheres_pixel[0],       sizeof(g_spheres_pixel)   };
    state.GS = { &g_spheres_geometry[0],    sizeof(g_spheres_geometry) };

    // Vertex struct holding position and color information.
    const D3D12_INPUT_ELEMENT_DESC d[] =
    {
        { "POSITION",    0, DXGI_FORMAT_R32G32B32A32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };


    state.InputLayout.NumElements = 1;
    state.InputLayout.pInputElementDescs = &d[0];

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

        m_queue					    = CreateCommandQueue(m_device.get());

        m_descriptorHeapRTV		    = CreateDescriptorHeapRTV(m_device.get());
        m_descriptorHeapDSV         = CreateDescriptorHeapDSV(m_device.get());
        m_descriptorHeapSRV         = CreateDescriptorHeapRTV(m_device.get());

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

        m_uploadHeap[0]             = CreateUploadHeap(m_device.get());
        m_uploadHeap[1]             = CreateUploadHeap(m_device.get());
        m_geometryHeap              = CreateGeometryHeap(m_device.get());

        m_uploadResource[0]         = CreateUploadGeometry(m_device.get(), m_uploadHeap[0].get(), 4 * 1024 * 1024);
        m_uploadResource[1]         = CreateUploadGeometry(m_device.get(), m_uploadHeap[0].get(), 4 * 1024 * 1024);
        m_geometry                  = CreateGeometry(m_device.get(), m_geometryHeap.get(), 4 * 1024 * 1024);


        m_uploadResource[0]->Map(0, nullptr, reinterpret_cast<void**>(&m_upload[0]));
        m_uploadResource[1]->Map(0, nullptr, reinterpret_cast<void**>(&m_upload[1]));

        {
            DirectX::XMVECTOR up = XMVectorSet(0, 1, 0, 0);
            DirectX::XMVECTOR forward = XMVectorSet(0, 0, 1, 0);
            DirectX::XMVECTOR position = XMVectorSet(0, 0, -10, 0);

            m_view = XMMatrixLookToLH(position, forward, up);
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

            uint8_t* upload_buffer = m_upload[m_frame_index];
            uint64_t upload_position = 0;
            
            AABB     aabb;
            aabb.m_max = XMVectorSet(1, 1, 1, 1);
            aabb.m_min = XMVectorSet(-1, -1, -1, 1);

            //Upload
            //AABB
            {
                
                XMVECTOR points[36];
                triangulate_aabb(aabb, &points[0]);
                std::memcpy(upload_buffer + upload_position, &points[0], sizeof(points));
                upload_position += sizeof(points);
            }

            //Frame constants
            uint64_t frame_offset = 0;
            {
                using namespace DirectX;

                FrameConstants frame;

                XMMATRIX p          = XMMatrixPerspectiveFovLH(lispsm::radians(75.0f), (float) m_back_buffer_width / (float)m_back_buffer_height, 64000.0f, 1.f);
                XMMATRIX horizontal = XMMatrixIdentity();
                XMMATRIX vertical   = XMMatrixIdentity();

                XMVECTOR left                = m_view.r[0];
                XMVECTOR up                  = m_view.r[1];
                XMVECTOR forward             = m_view.r[2];
                XMVECTOR translation         = m_view.r[3];
                XMVECTOR position            = XMVectorNegate(m_view.r[3]);
                XMVECTOR position_negative   = XMVectorNegate(m_view.r[3]);
                XMMATRIX translate_negative  = XMMatrixTranslationFromVector(position_negative);
                XMMATRIX translate           = XMMatrixTranslationFromVector(position);

                if (m_mode == 1)
                {
                    if (std::max( abs(m_x - m_begin_x), abs(m_y - m_begin_y) ) > 0)
                    {
                        float radians_x = m_x - m_begin_x < 0 ? 2.0f : -2.0f;
                        if (m_x - m_begin_x == 0.0f)
                        {
                            radians_x = 0.0f;
                        }

                        //radians_x = 0.0f;
                        
                        float radians_y = m_y - m_begin_y > 0 ? 2.0f : -2.0f;
                        if (m_y - m_begin_y == 0.0f)
                        {
                            radians_y = 0.0f;
                        }

                        //horizontal  = XMMatrixRotationAxis(up, lispsm::radians(radians_x));
                        //vertical    = XMMatrixRotationAxis(XMVector3Cross(forward, up), lispsm::radians(radians_y));
                        //vertical    = XMMatrixRotationAxis(XMVectorNegate(left), lispsm::radians(radians_y));

                        XMVECTOR rotation = XMVectorSet(-m_y + m_begin_y,  -m_x + m_begin_x, 0, 0);
                        XMVECTOR n        = XMVector4Normalize(rotation);
                        
                        horizontal        = XMMatrixRotationAxis(n, lispsm::radians(2));


                        m_begin_x = m_x;
                        m_begin_y = m_y;
                    }
                }
                else
                {
                    //horizontal                 = XMMatrixRotationAxis(up, lispsm::radians(0.f));
                    //vertical                   = XMMatrixRotationAxis(XMVector3Cross(up, forward), lispsm::radians(0.f));
                }

                XMMATRIX view;

                view.r[0] = left;
                view.r[1] = up;
                view.r[2] = forward;
                view.r[3] = XMVectorSet(0, 0, 0, 1);

                view = XMMatrixMultiply(horizontal, view);

                view.r[3] = translation;

                m_view = view;// XMMatrixMultiply(vertical, XMMatrixMultiply(horizontal, m_view));
                frame.m_view                = XMMatrixTranspose(m_view);
                frame.m_projection          = XMMatrixTranspose(p);
                frame.m_position            = position;

                upload_position             = align(upload_position, 256);
                frame_offset                = upload_position;

                std::memcpy(upload_buffer + upload_position, &frame, sizeof(frame));
                upload_position += sizeof(frame);
            }


            uint64_t spheres_offset = 0;
            uint64_t frustum_offset = 0;

            {
                XMVECTOR points[8];
                aabb_points(aabb, &points[0]);
                
                upload_position = align(upload_position, 16);
                spheres_offset  = upload_position;

                std::memcpy(upload_buffer + upload_position, &points[0], sizeof(points));
                upload_position += sizeof(points);
            }


            

            {
                XMMATRIX p = XMMatrixPerspectiveFovLH(lispsm::radians(75.0f), (float)m_back_buffer_width / (float)m_back_buffer_height, 10.0f, 1.f);

                BoundingFrustum f;
                BoundingFrustum::CreateFromMatrix(f, p);
                XMFLOAT3   corners[8];

                f.GetCorners(corners);

                XMVECTOR points[8];
                
                for (int i = 0; i < 8; ++i)
                {
                    points[i] = XMLoadFloat3(&corners[i]);
                }

                upload_position = align(upload_position, 16);
                frustum_offset = upload_position;

                std::memcpy(upload_buffer + upload_position, &points[0], sizeof(points));
                upload_position += sizeof(points);
            }

            commandList->CopyResource(m_geometry.get(), m_uploadResource[m_frame_index].get());

            //get the pointer to the gpu memory
            D3D12_CPU_DESCRIPTOR_HANDLE back_buffer  = CpuView(m_device.get(), m_descriptorHeapRTV.get()) + m_swap_chain_descriptors[m_frame_index];
            D3D12_CPU_DESCRIPTOR_HANDLE depth_buffer = CpuView(m_device.get(), m_descriptorHeapDSV.get()) + 0;

            //Transition resources for writing. flush caches
            {
                D3D12_RESOURCE_BARRIER barrier[2] = {};

                barrier[0].Type                     = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                barrier[0].Transition.pResource     = m_swap_chain_buffers[m_frame_index].get();
                barrier[0].Transition.StateBefore   = D3D12_RESOURCE_STATE_PRESENT;
                barrier[0].Transition.StateAfter    = D3D12_RESOURCE_STATE_RENDER_TARGET;
                barrier[0].Transition.Subresource   = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

                barrier[1].Type                     = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                barrier[1].Transition.pResource     = m_geometry.get();
                barrier[1].Transition.StateBefore   = D3D12_RESOURCE_STATE_COPY_DEST;
                barrier[1].Transition.StateAfter    = D3D12_RESOURCE_STATE_GENERIC_READ;
                barrier[1].Transition.Subresource   = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

                commandList->ResourceBarrier(2, &barrier[0]);
            }

            {
                D3D12_VERTEX_BUFFER_VIEW v = {};
                v.BufferLocation = m_geometry->GetGPUVirtualAddress();
                v.SizeInBytes   = 4 * 1024 * 1024;
                v.StrideInBytes = 16;
                commandList->IASetVertexBuffers(0, 1, &v);
            }

            //Mark the resources in the rasterizer output
            {
                commandList->OMSetRenderTargets(1, &back_buffer, TRUE, &depth_buffer);
            }

            //do the clear, fill the memory with a value
            {
                FLOAT c[4] = { 0.0f, 0.f,0.f,0.f };
                commandList->ClearRenderTargetView(back_buffer, c, 0, nullptr);
            }

            {
                commandList->ClearDepthStencilView(depth_buffer, D3D12_CLEAR_FLAG_DEPTH, 0.0f, 0, 0, nullptr);
            }

            {
                //set the type of the parameters that we will use in the shader
                commandList->SetGraphicsRootSignature(m_root_signature.get());

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

                //commandList->SetGraphicsRootConstantBufferView(0, m_uploadResource[m_frame_index]->GetGPUVirtualAddress() + frame_offset);
                commandList->SetGraphicsRootConstantBufferView(0, m_geometry->GetGPUVirtualAddress() + frame_offset);
                commandList->SetGraphicsRootShaderResourceView(1, m_geometry->GetGPUVirtualAddress());

                //draw aabb
                {
                    //set the raster pipeline state as a whole, it was prebuilt before
                    commandList->SetPipelineState(m_aabb_state.get());
                    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                    commandList->DrawInstanced(36, 1, 0, 0);
                }

                //set the types of the triangles we will use
                {
                    int subdivision_count       = 5 + 1;// subdivision_count + 1;
                    int vertical_segments       = subdivision_count;
                    int horizontal_segments     = subdivision_count * 2;
                    int vertex_count            = (horizontal_segments + 1) * (vertical_segments + 1);

                    commandList->SetPipelineState(m_spheres_state.get());
                    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

                    for (auto i = 0; i < 8; ++i)
                    {
                        commandList->SetGraphicsRoot32BitConstant(2, spheres_offset + i * sizeof(XMVECTOR), 0);
                        commandList->SetGraphicsRoot32BitConstant(2, 0xFF0000FF, 1);
                        commandList->SetGraphicsRoot32BitConstant(2, subdivision_count, 2);
                        commandList->DrawInstanced(vertex_count, 1, 0, 0);
                    }
                }


                {
                    int subdivision_count = 5 + 1;// subdivision_count + 1;
                    int vertical_segments = subdivision_count;
                    int horizontal_segments = subdivision_count * 2;
                    int vertex_count = (horizontal_segments + 1) * (vertical_segments + 1);

                    commandList->SetPipelineState(m_spheres_state.get());
                    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

                    for (auto i = 0; i < 8; ++i)
                    {
                        commandList->SetGraphicsRoot32BitConstant(2, frustum_offset + i * sizeof(XMVECTOR), 0);
                        commandList->SetGraphicsRoot32BitConstant(2, 0xFF00FF00, 1);
                        commandList->SetGraphicsRoot32BitConstant(2, subdivision_count, 2);
                        commandList->DrawInstanced(vertex_count, 1, 0, 0);
                    }
                }
            }

            //Transition resources for presenting, flush the gpu caches
            {
                D3D12_RESOURCE_BARRIER barrier[2] = {};

                barrier[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                barrier[0].Transition.pResource = m_swap_chain_buffers[m_frame_index].get();
                barrier[0].Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
                barrier[0].Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
                barrier[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

                barrier[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                barrier[1].Transition.pResource = m_geometry.get();
                barrier[1].Transition.StateBefore = D3D12_RESOURCE_STATE_GENERIC_READ;
                barrier[1].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
                barrier[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;


                commandList->ResourceBarrier(2, &barrier[0]);
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
        m_aabb_state     = CreateAabbPipelineState(m_device.get(), m_root_signature.get());
        m_spheres_state  = CreateSpheresPipelineState(m_device.get(), m_root_signature.get());
    }

    void SetWindow(const CoreWindow& w)
    {
        m_closed			    = w.Closed(winrt::auto_revoke, { this, &ViewProvider::OnWindowClosed });
        m_size_changed		    = w.SizeChanged(winrt::auto_revoke, { this, &ViewProvider::OnWindowSizeChanged });
        m_pointer_moved         = w.PointerMoved(winrt::auto_revoke, { this, &ViewProvider::OnPointerMoved });
        m_pointer_pressed       = w.PointerPressed(winrt::auto_revoke, { this, &ViewProvider::OnPointerPressed });
        m_pointer_released      = w.PointerReleased(winrt::auto_revoke, { this, &ViewProvider::OnPointerReleased });

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
        CreateSwapChainDescriptor(m_device.get(), m_swap_chain_buffers[0].get(), CpuView(m_device.get(), m_descriptorHeapRTV.get()) + 0);
        CreateSwapChainDescriptor(m_device.get(), m_swap_chain_buffers[1].get(), CpuView(m_device.get(), m_descriptorHeapRTV.get()) + 1);

        //Where are located the descriptors
        m_swap_chain_descriptors[0] = 0;
        m_swap_chain_descriptors[1] = 1;


        m_debug_buffer1 = CreateDebugBuffer1(m_device.get(), m_back_buffer_width, m_back_buffer_height);
        //create render target views, that will be used for rendering
        CreateDebugBuffer1Descriptior(m_device.get(), m_debug_buffer1.get(), CpuView(m_device.get(), m_descriptorHeapRTV.get()) + 2);
        //Where are located the descriptors
        m_debug_buffer1_descriptor = 2;

        m_depth_buffer = CreateDepthResource(m_device.get(), m_back_buffer_width, m_back_buffer_height);
        m_depth_buffer->SetName(L"Depth Buffer");

        CreateDepthWriteDescriptor(m_device.get(), m_depth_buffer.get(), CpuView(m_device.get(), m_descriptorHeapDSV.get()) + 0);
        CreateDepthReadDescriptor(m_device.get(), m_depth_buffer.get(), CpuView(m_device.get(), m_descriptorHeapDSV.get()) + 1);

        m_depth_descriptor[0] = 0;
        m_depth_descriptor[1] = 1;
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

		ThrowIfFailed(m_swap_chain->ResizeBuffers(2, m_back_buffer_width, m_back_buffer_height, DXGI_FORMAT_R8G8B8A8_UNORM, 0));

        //allocate memory for the swap chain again
        m_swap_chain_buffers[0] = CreateSwapChainResource(m_device.get(), m_swap_chain.get(), 0);
        m_swap_chain_buffers[1] = CreateSwapChainResource(m_device.get(), m_swap_chain.get(), 1);

        //set names so we can see them in pix
        m_swap_chain_buffers[0]->SetName(L"Buffer 0");
        m_swap_chain_buffers[1]->SetName(L"Buffer 1");

        //create render target views, that will be used for rendering
        CreateSwapChainDescriptor(m_device.get(), m_swap_chain_buffers[0].get(), CpuView(m_device.get(), m_descriptorHeapRTV.get()) + 0);
        CreateSwapChainDescriptor(m_device.get(), m_swap_chain_buffers[1].get(), CpuView(m_device.get(), m_descriptorHeapRTV.get()) + 1);

        m_swap_chain_descriptors[0] = 0;
        m_swap_chain_descriptors[1] = 1;

        m_debug_buffer1 = CreateDebugBuffer1(m_device.get(), m_back_buffer_width, m_back_buffer_height);
        //create render target views, that will be used for rendering
        CreateDebugBuffer1Descriptior(m_device.get(), m_debug_buffer1.get(), CpuView(m_device.get(), m_descriptorHeapRTV.get()) + 2);
        //Where are located the descriptors
        m_debug_buffer1_descriptor = 2;

        m_depth_buffer = CreateDepthResource(m_device.get(), m_back_buffer_width, m_back_buffer_height);
        m_depth_buffer->SetName(L"Depth Buffer");

        CreateDepthWriteDescriptor(m_device.get(), m_depth_buffer.get(), CpuView(m_device.get(), m_descriptorHeapDSV.get()) + 0);
        CreateDepthReadDescriptor(m_device.get(), m_depth_buffer.get(), CpuView(m_device.get(), m_descriptorHeapDSV.get()) + 1);

        m_depth_descriptor[0] = 0;
        m_depth_descriptor[1] = 1;
    }

    void OnPointerMoved(const CoreWindow& w, const PointerEventArgs& a)
    {
        const auto&& point = a.CurrentPoint().Position();
        m_x = point.X;
        m_y = point.Y;
        
    }

    void OnPointerPressed(const CoreWindow& w, const PointerEventArgs& a)
    {
        const auto&& point = a.CurrentPoint().Position();
        m_begin_x = point.X;
        m_begin_y = point.Y;
        m_mode = 1;
    }

    void OnPointerReleased(const CoreWindow& w, const PointerEventArgs& a)
    {
        m_begin_x = 0;
        m_begin_y = 0;
        m_x       = 0;
        m_y       = 0;
        m_mode    = 0;
    }
    
    bool m_window_running = true;

    CoreWindow::Closed_revoker					m_closed;
    CoreWindow::SizeChanged_revoker				m_size_changed;
    CoreWindow::PointerMoved_revoker			m_pointer_moved;
    CoreWindow::PointerPressed_revoker			m_pointer_pressed;
    CoreWindow::PointerReleased_revoker			m_pointer_released;

    float                                       m_begin_x = 0;
    float                                       m_begin_y = 0;
    float                                       m_x = 0;
    float                                       m_y = 0;
    uint32_t                                    m_mode = 0;
    DirectX::XMMATRIX                           m_view;


    CoreApplicationView::Activated_revoker		m_activated;
    
    winrt::com_ptr <ID3D12Debug>                m_debug;
    winrt::com_ptr <ID3D12Device1>				m_device;                       //device for gpu resources
    winrt::com_ptr <IDXGISwapChain3>			m_swap_chain;                   //swap chain for 

    winrt::com_ptr <ID3D12Fence>        		m_fence;                        //fence for cpu/gpu synchronization
    winrt::com_ptr <ID3D12CommandQueue>   		m_queue;                        //queue to the device

    winrt::com_ptr <ID3D12DescriptorHeap>   	m_descriptorHeapRTV;            //descriptor heap for the resources
    winrt::com_ptr <ID3D12DescriptorHeap>       m_descriptorHeapDSV;            //descriptor heap for the resources
    winrt::com_ptr <ID3D12DescriptorHeap>   	m_descriptorHeapSRV;            //descriptor heap for the resources

    winrt::com_ptr <ID3D12Heap>   	            m_uploadHeap[2];                //upload heap
    winrt::com_ptr <ID3D12Heap>   	            m_geometryHeap;                 //gpu heap
    winrt::com_ptr<ID3D12Resource1>             m_uploadResource[2];            //alias the whole upload heap
    winrt::com_ptr<ID3D12Resource1>             m_geometry;                     //alias the whole geometry heap
    uint8_t*                                    m_upload[2];

    std::mutex                                  m_blockRendering;               //block render thread for the swap chain resizes

    winrt::com_ptr<ID3D12Resource1>             m_swap_chain_buffers[2];
    uint64_t                                    m_swap_chain_descriptors[2];

    winrt::com_ptr<ID3D12Resource1>             m_debug_buffer1;
    uint64_t                                    m_debug_buffer1_descriptor;

    winrt::com_ptr<ID3D12Resource1>             m_frame_buffer;
    uint32_t                                    m_frame_buffer_srv;
    uint32_t                                    m_frame_buffer_uav;

    uint32_t									m_back_buffer_width = 0;
    uint32_t									m_back_buffer_height = 0;

    winrt::com_ptr<ID3D12Resource1>             m_depth_buffer;
    uint64_t                                    m_depth_descriptor[2];

    winrt::com_ptr <ID3D12CommandAllocator>   	m_command_allocator[2];		    //one per frame
    winrt::com_ptr <ID3D12GraphicsCommandList1> m_command_list[2];			    //one per frame

    uint64_t                                    m_frame_index	= 0;
    uint64_t									m_fence_value	= 1;
    HANDLE										m_fence_event = {};

    //Rendering
    winrt::com_ptr< ID3D12RootSignature>		m_root_signature;
    winrt::com_ptr< ID3D12PipelineState>		m_triangle_state;

    winrt::com_ptr< ID3D12PipelineState>		m_aabb_state;
    winrt::com_ptr< ID3D12PipelineState>		m_spheres_state;
};

int32_t __stdcall wWinMain( HINSTANCE, HINSTANCE,PWSTR, int32_t )
{
    ThrowIfFailed(CoInitializeEx(nullptr, COINIT_MULTITHREADED));
    CoreApplication::Run(ViewProvider());
    CoUninitialize();
    return 0;
}





//Step 1.

//1.Get Frustum points in ws from depth
//2.Get Shadow Casters which is is a box around the frustum tbd
//3.Include light volume in frustum points, 
//4.Get Light View Transform, add Lispsm Perspective view transform just z and y swapped with reversed directions
//5.Project all frustum points in this new space and find the closest point to the camera in this view frustum and do a new light view direction
//6.Do the lispsm perspective transform frame
//7.

using namespace DirectX;


XMGLOBALCONST XMVECTORU32 g_XMSelect0111 = { { { XM_SELECT_0, XM_SELECT_1, XM_SELECT_0, XM_SELECT_1 } } };
XMGLOBALCONST XMVECTORU32 g_XMSelect1101 = { { { XM_SELECT_1, XM_SELECT_1, XM_SELECT_1, XM_SELECT_0 } } };
XMGLOBALCONST XMVECTORU32 g_XMSelect0001 = { { { XM_SELECT_0, XM_SELECT_0, XM_SELECT_0, XM_SELECT_1 } } };
XMGLOBALCONST XMVECTORU32 g_XMSelect0011 = { { { XM_SELECT_0, XM_SELECT_0, XM_SELECT_1, XM_SELECT_1 } } };


XMMATRIX fitting_projection_matrix(const AABB box)
{
    XMMATRIX m;

    //XMVECTOR  min               = XMLoadFloat4(&box.m_min);
    //XMVECTOR  max               = XMLoadFloat4(&box.m_max);

    XMVECTOR  min               = box.m_min;
    XMVECTOR  max               = box.m_max;

    XMVECTOR  sum               = XMVectorAdd(max, min);
    XMVECTOR  extents           = XMVectorSubtract(max, min);
    XMVECTOR  two               = XMVectorReplicate(2.0f);

    XMVECTOR  r_scale           = XMVectorDivide(two, extents);
    XMVECTOR  r_translation0    = XMVectorDivide(sum, extents);
    XMVECTOR  r_translation1    = XMVectorNegate(r_translation0);

    //g_XMOne
    //g_XMZero
    XMVECTOR r0                 = XMVectorSelect(r_scale, g_XMZero, g_XMSelect0111);
    XMVECTOR r1                 = XMVectorSelect(r_scale, g_XMZero, g_XMSelect1011);
    XMVECTOR r2                 = XMVectorSelect(r_scale, g_XMZero, g_XMSelect1101);
    XMVECTOR r3                 = XMVectorSelect(r_scale, g_XMOne,  g_XMSelect0001);

    m.r[0]                      = r0;
    m.r[1]                      = r1;
    m.r[2]                      = r2;
    m.r[3]                      = r3;

    /*

    m.x[0][0] = 2.0f / (box.Max()[0] - box.Min()[0]);
    m.x[0][1] = .0f;
    m.x[0][2] = .0f;
    m.x[0][3] = .0f;

    m.x[1][0] = .0f;
    m.x[1][1] = 2.0f / (box.Max()[1] - box.Min()[1]);
    m.x[1][2] = .0f;
    m.x[1][3] = .0f;

    m.x[2][0] = .0f;
    m.x[2][1] = .0f;
    m.x[2][2] = 2.0f / (box.Max()[2] - box.Min()[2]);
    m.x[2][3] = .0f;

    m.x[3][0] = -(box.Max()[0] + box.Min()[0]) / (box.Max()[0] - box.Min()[0]);
    m.x[3][1] = -(box.Max()[1] + box.Min()[1]) / (box.Max()[1] - box.Min()[1]);
    m.x[3][2] = -(box.Max()[2] + box.Min()[2]) / (box.Max()[2] - box.Min()[2]);
    m.x[3][3] = 1.0f;

    //m.x[3][0] = m.x[3][1] = m.x[3][2] = .0f; m.x[3][3] = 1.0f;

    */

    return m;
}

XMVECTOR compute_light_up_vector(XMVECTOR camera_direction_ws, XMVECTOR light_direction_ws)
{
    //right handed

    // left is the normalized vector perpendicular to lightDir and viewDir
    // this means left is the normalvector of the yz-plane from the paper
    XMVECTOR left_ws = XMVector3Cross(light_direction_ws, camera_direction_ws);

    // we now can calculate the rotated(in the yz-plane) viewDir vector
    // and use it as up vector in further transformations
    XMVECTOR up_ws   = XMVector3Cross(left_ws, light_direction_ws);

    return up_ws;
}

//y points towards the light
//z is roughly along the camera view direction
//x is orthogonal
//xz in the shadow plane
//inputs are expected to be normalized
XMMATRIX light_view_matrix( XMVECTOR camera_direction_ws, XMVECTOR camera_position_ws, XMVECTOR light_direction_ws)
{
    //right handed
    XMVECTOR up = compute_light_up_vector(camera_direction_ws, light_direction_ws);
    XMMATRIX m  = XMMatrixLookToRH(camera_position_ws, light_direction_ws, up);
    return m;
}

namespace
{
    bool above0(int32_t mask)
    {
        return (mask & 0x1) != 0;
    }

    bool above1(int32_t mask)
    {
        return (mask & 0x2) != 0;
    }

    bool above2(int32_t mask)
    {
        return (mask & 0x4) != 0;
    }

    bool above3(int32_t mask)
    {
        return (mask & 0x8) != 0;
    }
}

void clip3(XMVECTOR n, XMVECTOR v0, XMVECTOR v1, XMVECTOR v2, XMVECTOR* clipped_frustum, uint32_t* clipped_frustum_count )
{
    //Distances to the plane (this is an array parallel // to v[], stored as a vec3)
    //XMVECTOR dist = vec3(dot(v0, n), dot(v1, n), dot(v2, n));

    XMVECTOR v3;
    XMVECTOR d0             = XMVector3Dot(n, v0);
    XMVECTOR d1             = XMVector3Dot(n, v1);
    XMVECTOR d2             = XMVector3Dot(n, v2);

    XMVECTOR clipEpsilon    = XMVectorNegate(XMVectorReplicate(0.00001f));
    XMVECTOR clipEpsilon2   = XMVectorReplicate(0.01f);
    
    XMVECTOR r0             = d0;
    XMVECTOR r1             = XMVectorSelect(r0, d1, g_XMSelect0111);
    XMVECTOR r2             = XMVectorSelect(r1, d2, g_XMSelect0011);
    XMVECTOR r3             = XMVectorSelect(r2, g_XMZero, g_XMSelect1110);
    XMVECTOR dist           = r3;

    //all clipped
    if (!XMComparisonAnyTrue(XMVector3GreaterOrEqualR(dist, clipEpsilon2)))
    {
        return;
    }

    //none clipped
    if (XMComparisonAllTrue(XMVector3GreaterOrEqualR(dist, clipEpsilon)))
    {
        *clipped_frustum++ = v0;
        *clipped_frustum++ = v1;
        *clipped_frustum++ = v2;
        *clipped_frustum_count += 3;
    }

    // There are either 1 or 2 vertices above the clipping plane .
    XMVECTOR abovex  = XMVectorGreaterOrEqual(r3, g_XMZero);
    int32_t  above   = _mm_movemask_ps(abovex);
    bool nextIsAbove;

    // Find the CCW - most vertex above the plane .
    if (above1(above) && !above0(above) )
    {
        // Cycle once CCW . Use v3 as a temp
        nextIsAbove = above2(above);
        v3 = v0; v0 = v1; v1 = v2; v2 = v3;

        //dist = dist.yzx;
        dist = XMVectorSwizzle<1, 2, 0, 3>(dist);

        XMVECTOR temp = d0;
        d0 = d1;
        d1 = d2;
        d2 = temp;
        
    } else if (above2(above) && !above1(above))
    {
        // Cycle once CW . Use v3 as a temp .
        nextIsAbove = above0(above);
        v3 = v2; v2 = v1; v1 = v0; v0 = v3;
        //dist = dist.zxy;
        dist = XMVectorSwizzle<2, 0, 1, 3>(dist);

        XMVECTOR temp = d0;
        d0 = d2;
        d2 = d1;
        d1 = temp;
    }
    else
    {
        nextIsAbove = above1(above);
    }

    v3 = XMVectorLerpV(v0, v2, XMVectorDivide(d0, XMVectorSubtract(d0, d2)));

    if (nextIsAbove)
    {
        v2 = XMVectorLerpV(v1, v2, XMVectorDivide(d1, XMVectorSubtract(d1, d2)));

        *clipped_frustum++ = v0;
        *clipped_frustum++ = v1;
        *clipped_frustum++ = v2;
        *clipped_frustum++ = v3;
        *clipped_frustum_count += 4;

        //4 points
    }
    else
    {
        v1 = XMVectorLerpV(v0, v1, XMVectorDivide(d0, XMVectorSubtract(d0, d1)));
        v2 = v3;
        v3 = v0;

        *clipped_frustum++ = v0;
        *clipped_frustum++ = v1;
        *clipped_frustum++ = v2;
        *clipped_frustum_count += 3;
        //3 points
    }
}

void compute_clipped_frustum(const XMVECTOR* triangles, uint32_t triangle_count, const XMVECTOR frustumPlanes[6], XMVECTOR* clipped_frustum, uint32_t* clipped_frustum_count)
{
    *clipped_frustum_count = 0;

}

void triangulate_aabb( const AABB aabb, XMVECTOR* points )
{
    XMVECTOR center             = XMVectorMultiply(XMVectorAdd(aabb.m_max, aabb.m_min), XMVectorReplicate(0.5f));
    XMVECTOR extents            = XMVectorSubtract(aabb.m_max, aabb.m_min);

    XMVECTOR back_top_left      = XMVectorSet(-1, 1, -1, 1);
    XMVECTOR back_top_right     = XMVectorSet( 1, 1, -1, 1);
    XMVECTOR front_top_right    = XMVectorSet( 1, 1,  1, 1);
    XMVECTOR front_top_left     = XMVectorSet(-1, 1,  1, 1);

    XMVECTOR back_bottom_left   = XMVectorSet(-1, -1, -1, 1);
    XMVECTOR back_bottom_right  = XMVectorSet( 1, -1, -1, 1);
    XMVECTOR front_bottom_right = XMVectorSet( 1, -1,  1, 1);
    XMVECTOR front_bottom_left  = XMVectorSet(-1, -1,  1, 1);

    const XMVECTOR masks[8]           =
    {
        back_top_left, 
        back_top_right, 
        front_top_right, 
        front_top_left, 

        back_bottom_left,
        back_bottom_right,
        front_bottom_right,
        front_bottom_left
    };

    const XMVECTOR aabb_points[8]          =
    {
        XMVectorAdd(center, XMVectorMultiply(extents, masks[0])),
        XMVectorAdd(center, XMVectorMultiply(extents, masks[1])),
        XMVectorAdd(center, XMVectorMultiply(extents, masks[2])),
        XMVectorAdd(center, XMVectorMultiply(extents, masks[3])),
        
        XMVectorAdd(center, XMVectorMultiply(extents, masks[4])),
        XMVectorAdd(center, XMVectorMultiply(extents, masks[5])),
        XMVectorAdd(center, XMVectorMultiply(extents, masks[6])),
        XMVectorAdd(center, XMVectorMultiply(extents, masks[7]))
    };

    const uint32_t indices[36] =
    {
        0, 1, 3,
        1, 2, 3,

        3, 2, 7,
        6, 7, 2,

        2, 1, 6,
        5, 6, 1,

        1, 0, 5,
        4, 5, 0,

        0, 3, 4,
        7, 4, 3,

        7, 6, 4,
        5, 4, 6,
    };

    for (auto i = 0U; i < 36; ++i)
    {
        points[i] = aabb_points[ indices[i] ];
    }
}

void aabb_points(const AABB aabb, XMVECTOR* points)
{
    XMVECTOR center = XMVectorMultiply(XMVectorAdd(aabb.m_max, aabb.m_min), XMVectorReplicate(0.5f));
    XMVECTOR extents = XMVectorSubtract(aabb.m_max, aabb.m_min);

    XMVECTOR back_top_left = XMVectorSet(-1, 1, -1, 1);
    XMVECTOR back_top_right = XMVectorSet(1, 1, -1, 1);
    XMVECTOR front_top_right = XMVectorSet(1, 1, 1, 1);
    XMVECTOR front_top_left = XMVectorSet(-1, 1, 1, 1);

    XMVECTOR back_bottom_left = XMVectorSet(-1, -1, -1, 1);
    XMVECTOR back_bottom_right = XMVectorSet(1, -1, -1, 1);
    XMVECTOR front_bottom_right = XMVectorSet(1, -1, 1, 1);
    XMVECTOR front_bottom_left = XMVectorSet(-1, -1, 1, 1);

    const XMVECTOR masks[8] =
    {
        back_top_left,
        back_top_right,
        front_top_right,
        front_top_left,

        back_bottom_left,
        back_bottom_right,
        front_bottom_right,
        front_bottom_left
    };

    const XMVECTOR aabb_points[8] =
    {
        XMVectorAdd(center, XMVectorMultiply(extents, masks[0])),
        XMVectorAdd(center, XMVectorMultiply(extents, masks[1])),
        XMVectorAdd(center, XMVectorMultiply(extents, masks[2])),
        XMVectorAdd(center, XMVectorMultiply(extents, masks[3])),

        XMVectorAdd(center, XMVectorMultiply(extents, masks[4])),
        XMVectorAdd(center, XMVectorMultiply(extents, masks[5])),
        XMVectorAdd(center, XMVectorMultiply(extents, masks[6])),
        XMVectorAdd(center, XMVectorMultiply(extents, masks[7]))
    };

    for (auto i = 0U; i < 8; ++i)
    {
        points[i] = aabb_points[i];
    }
}

void compute_clipped_frustum(const AABB shadowCasters, const XMVECTOR frustumPlanes[6], XMVECTOR* clipped_frustum, uint32_t* clipped_frustum_count)
{
    *clipped_frustum_count = 0;
}

void include_light_volume( AABB shadowCasters, XMVECTOR light_direction_ws, XMVECTOR* clipped_frustum, uint32_t* clipped_frustum_count)
{
    
}

XMMATRIX compute_light_projection( XMMATRIX light_view, AABB shadowCasters, XMVECTOR frustumPlanes[6] )
{
    //allocate buffer here
    //XMVECTOR clipped_frustum[512];
    //XMVECTOR clipped_frustum_count;
}

























