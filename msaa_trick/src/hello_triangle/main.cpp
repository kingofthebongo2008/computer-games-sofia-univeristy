#include "pch.h"
#include <cstdint>

#include "d3dx12.h"
#include "cpu_view.h"
#include <pix3.h>
#include "build_window_environment.h"

using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::ApplicationModel::Core;
using namespace winrt::Windows::ApplicationModel::Activation;

using namespace winrt::Windows::UI::Core;
using namespace winrt::Windows::UI::ViewManagement;

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
        v.attach(sample::copy_to_abi_private<::IUnknown>(w));
        return v;
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

static winrt::com_ptr<IDXGISwapChain3> CreateSwapChain(const CoreWindow& w, ID3D12CommandQueue* d, uint32_t width, uint32_t height)
{
    winrt::com_ptr<IDXGIFactory2> f;
    winrt::com_ptr<IDXGISwapChain1> r;
    
    ThrowIfFailed(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG,__uuidof(IDXGIFactory2), f.put_void()));

    DXGI_SWAP_CHAIN_DESC1 desc = {};

    desc.BufferCount    = 2;
    desc.Format         = DXGI_FORMAT_B8G8R8A8_UNORM;
	desc.Width			= width;
    desc.Height         = height;
    desc.SampleDesc.Count = 1;
    desc.SwapEffect     = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.BufferUsage    = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.AlphaMode      = DXGI_ALPHA_MODE_IGNORE;
    desc.Scaling        = DXGI_SCALING_NONE;

    ThrowIfFailed(f->CreateSwapChainForCoreWindow(d, sample::copy_to_abi<::IUnknown>(w).get(), &desc, nullptr, r.put()));
    return r.as< IDXGISwapChain3>();
}

static winrt::com_ptr <ID3D12Fence> CreateFence(ID3D12Device1* device, uint64_t initialValue = 0)
{
    winrt::com_ptr<ID3D12Fence> r;
    ThrowIfFailed(device->CreateFence(initialValue, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), r.put_void()));
    return r;
}

static winrt::com_ptr <ID3D12DescriptorHeap> CreateDescriptorHeapTargets(ID3D12Device1* device)
{
    winrt::com_ptr<ID3D12DescriptorHeap> r;
    D3D12_DESCRIPTOR_HEAP_DESC d = {};

    d.NumDescriptors = 4;
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

    d.NumDescriptors = 4;
    d.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    device->CreateDescriptorHeap(&d, __uuidof(ID3D12DescriptorHeap), r.put_void());
    return r;
}

static winrt::com_ptr <ID3D12DescriptorHeap> CreateDescriptorHeapShaderGpu(ID3D12Device1* device)
{
    winrt::com_ptr<ID3D12DescriptorHeap> r;
    D3D12_DESCRIPTOR_HEAP_DESC d = {};

    d.NumDescriptors = 4;
    d.Type  = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
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
    d.Alignment = D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT;
    d.DepthOrArraySize = 1;
    d.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    d.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    d.Format = DXGI_FORMAT_D32_FLOAT;
    d.Height = height;
    d.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    d.MipLevels = 1;
    d.SampleDesc.Count = 4;
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

static winrt::com_ptr<ID3D12Heap> CreateRenderTargetsHeap(ID3D12Device1* device)
{
    winrt::com_ptr<ID3D12Heap>     r;

    D3D12_HEAP_DESC         d = {};
    D3D12_HEAP_PROPERTIES   p = {};

    p.Type = D3D12_HEAP_TYPE_DEFAULT;

    d.Alignment     = D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT;
    d.Flags         = D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES;
    d.Properties    = p;
    d.SizeInBytes   = 1024 * 1024 * 64;

    ThrowIfFailed(device->CreateHeap(&d, __uuidof(ID3D12Heap), r.put_void()));

    return r;
}
    
namespace
{
    uint64_t align_value(uint64_t v, uint64_t alignment)
    {
        return (v + alignment - 1) & ~( alignment - 1 );
    }
}

static winrt::com_ptr<ID3D12Resource1> CreateLightingResourceMSAA(ID3D12Device1* device, ID3D12Heap* heap, uint32_t index, uint32_t width, uint32_t height)
{
    D3D12_RESOURCE_DESC d = {};
    d.Alignment = D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT;
    d.DepthOrArraySize = 1;
    d.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    d.Flags     = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    d.Format    = DXGI_FORMAT_B8G8R8A8_TYPELESS;
    d.Height    = height;
    d.Layout    = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    d.MipLevels = 1;
    d.SampleDesc.Count = 4;
    d.SampleDesc.Quality = 0;
    d.Width = width;

    D3D12_RESOURCE_STATES       state = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

    D3D12_CLEAR_VALUE v = {};
    v.Format = DXGI_FORMAT_B8G8R8A8_UNORM;

    D3D12_RESOURCE_ALLOCATION_INFO info = device->GetResourceAllocationInfo(0, 1, &d);
    uint64_t offset                     = align_value(info.SizeInBytes * index, info.Alignment);

    winrt::com_ptr<ID3D12Resource1>     r;
    ThrowIfFailed(device->CreatePlacedResource(heap, offset , &d, state, &v, __uuidof(ID3D12Resource1), r.put_void()));

    return r;
}

static winrt::com_ptr<ID3D12Resource1> CreateLightingResource(ID3D12Device1* device, uint32_t width, uint32_t height, D3D12_RESOURCE_STATES state)
{
    D3D12_RESOURCE_DESC d = {};
    d.Alignment = 0;
    d.DepthOrArraySize = 1;
    d.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    d.Flags     = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    d.Format    = DXGI_FORMAT_B8G8R8A8_UNORM;
    d.Height    = height;
    d.Layout    = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    d.MipLevels = 1;
    d.SampleDesc.Count = 1;
    d.SampleDesc.Quality = 0;
    d.Width = width;

    winrt::com_ptr<ID3D12Resource1>     r;
    D3D12_HEAP_PROPERTIES p = {};
    p.Type = D3D12_HEAP_TYPE_DEFAULT;
    //D3D12_RESOURCE_STATES       state = D3D12_RESOURCE_STATE_COPY_SOURCE;

    ThrowIfFailed(device->CreateCommittedResource(&p, D3D12_HEAP_FLAG_NONE, &d, state, nullptr, __uuidof(ID3D12Resource1), r.put_void()));
    return r;
}

static winrt::com_ptr<ID3D12Resource1> CreateLightingResource1(ID3D12Device1* device, uint32_t width, uint32_t height)
{
    D3D12_RESOURCE_DESC d = {};
    d.Alignment = 0;
    d.DepthOrArraySize = 1;
    d.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    d.Flags  = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    d.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    d.Height = height;
    d.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    d.MipLevels = 1;
    d.SampleDesc.Count = 1;
    d.SampleDesc.Quality = 0;
    d.Width = width;

    winrt::com_ptr<ID3D12Resource1>     r;
    D3D12_HEAP_PROPERTIES p = {};
    p.Type = D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_STATES       state = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

    D3D12_CLEAR_VALUE v = {};
    v.Format = DXGI_FORMAT_B8G8R8A8_UNORM;

    ThrowIfFailed(device->CreateCommittedResource(&p, D3D12_HEAP_FLAG_NONE, &d, state, &v, __uuidof(ID3D12Resource1), r.put_void()));
    return r;
}

//Create a gpu metadata that describes the swap chain, type, format. it will be used by the gpu interpret the data in the swap chain(reading/writing).
static void CreateDepthWriteDescriptor(ID3D12Device1* device, ID3D12Resource1* resource, D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
    D3D12_DEPTH_STENCIL_VIEW_DESC d = {};
    d.ViewDimension                 = D3D12_DSV_DIMENSION_TEXTURE2DMS;
    d.Format                        = DXGI_FORMAT_D32_FLOAT;       //how we will view the resource during rendering
    device->CreateDepthStencilView(resource, &d, handle);
}

static void CreateDepthReadDescriptor(ID3D12Device1* device, ID3D12Resource1* resource, D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
    D3D12_DEPTH_STENCIL_VIEW_DESC d = {};
    d.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
    d.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH;
    d.Format = DXGI_FORMAT_D32_FLOAT;               //how we will view the resource during rendering
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
static void CreateRenderTargetDescriptor(ID3D12Device1* device, ID3D12Resource1* resource, D3D12_CPU_DESCRIPTOR_HANDLE handle )
{
    D3D12_RENDER_TARGET_VIEW_DESC d = {};
    d.ViewDimension                 = D3D12_RTV_DIMENSION_TEXTURE2DMS;
    d.Format                        = DXGI_FORMAT_B8G8R8A8_UNORM;       //how we will view the resource during rendering
    device->CreateRenderTargetView(resource, &d, handle);
}

//Create a gpu metadata that describes the swap chain, type, format. it will be used by the gpu interpret the data in the swap chain(reading/writing).
static void CreateShaderResourceViewDescriptor(ID3D12Device1* device, ID3D12Resource1* resource, D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC d = {};
    d.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
    d.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    d.Format = DXGI_FORMAT_B8G8R8A8_UNORM;       //how we will view the resource during rendering
    device->CreateShaderResourceView(resource, &d, handle);
}

//Create a gpu metadata that describes the swap chain, type, format. it will be used by the gpu interpret the data in the swap chain(reading/writing).
static void CreateUnorderedAccessViewDescriptor(ID3D12Device1* device, ID3D12Resource1* resource, D3D12_CPU_DESCRIPTOR_HANDLE handle, uint32_t width, uint32_t height)
{
    D3D12_UNORDERED_ACCESS_VIEW_DESC d = {};
    d.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    d.Format        = DXGI_FORMAT_B8G8R8A8_UNORM;          //how we will view the resource during rendering
    device->CreateUnorderedAccessView(resource, nullptr, &d, handle);
}

//Create the memory manager for the gpu commands
static winrt::com_ptr <ID3D12CommandAllocator> CreateGraphicsAllocator(ID3D12Device1* device, const wchar_t* name)
{
    winrt::com_ptr<ID3D12CommandAllocator> r;
    ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), r.put_void()));
	r->SetName(name);
    return r;
}

//Create the memory manager for the gpu commands
static winrt::com_ptr <ID3D12CommandAllocator> CreateComputeAllocator(ID3D12Device1* device, const wchar_t* name)
{
    winrt::com_ptr<ID3D12CommandAllocator> r;
    ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, __uuidof(ID3D12CommandAllocator), r.put_void()));
	r->SetName(name);
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
static winrt::com_ptr< ID3D12RootSignature>  CreateGraphicsRootSignature(ID3D12Device1* device)
{
    static 
    #include <default_graphics_signature.h>

    winrt::com_ptr<ID3D12RootSignature> r;
    ThrowIfFailed(device->CreateRootSignature( 0, &g_default_graphics_signature[0], sizeof(g_default_graphics_signature), __uuidof(ID3D12RootSignature), r.put_void()));
    return r;
}

//create an object which represents what types of external data the shaders will use. You can imagine f(int x, float y); Root Signature is that we have two parameters on locations 0 and 1 types int and float
static winrt::com_ptr< ID3D12RootSignature>  CreateComputeRootSignature(ID3D12Device1* device)
{
    static
    #include <default_compute_signature.h>

    winrt::com_ptr<ID3D12RootSignature> r;
    ThrowIfFailed(device->CreateRootSignature(0, &g_default_compute_signature[0], sizeof(g_default_compute_signature), __uuidof(ID3D12RootSignature), r.put_void()));
    return r;
}

D3D12_GRAPHICS_PIPELINE_STATE_DESC CreateTriangleDescription()
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC state = {};
    state.SampleMask = UINT_MAX;
    state.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    state.RasterizerState.MultisampleEnable = TRUE;
    
    state.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    state.RasterizerState.FrontCounterClockwise = TRUE;

    state.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    state.NumRenderTargets  = 0;
    state.SampleDesc.Count  = 4;
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
static winrt::com_ptr< ID3D12PipelineState>  CreateTrianglePipelineState(ID3D12Device1* device, ID3D12RootSignature* root)
{
    static
#include <triangle_pixel.h>

        static
#include <triangle_vertex.h>

    D3D12_GRAPHICS_PIPELINE_STATE_DESC state = CreateTriangleDescription();
    state.pRootSignature            = root;
    state.NumRenderTargets          = 1;
    state.RTVFormats[0]             = DXGI_FORMAT_B8G8R8A8_UNORM;

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
static winrt::com_ptr< ID3D12PipelineState>  CreateTriangleDepthPipelineState(ID3D12Device1* device, ID3D12RootSignature* root)
{
        static
#include <triangle_vertex.h>

    D3D12_GRAPHICS_PIPELINE_STATE_DESC state = CreateTriangleDescription();
    state.pRootSignature = root;

    state.DepthStencilState.DepthEnable     = TRUE;
    state.DepthStencilState.StencilEnable   = FALSE;
    state.DepthStencilState.DepthWriteMask  = D3D12_DEPTH_WRITE_MASK_ALL;
    state.DepthStencilState.DepthFunc       = D3D12_COMPARISON_FUNC_LESS;

    state.VS = { &g_triangle_vertex[0], sizeof(g_triangle_vertex) };
    
    winrt::com_ptr<ID3D12PipelineState> r;

    ThrowIfFailed(device->CreateGraphicsPipelineState(&state, __uuidof(ID3D12PipelineState), r.put_void()));
    return r;
}


//create a state for the rasterizer. that will be set a whole big monolitic block. Below the driver optimizes it in the most compact form for it. 
//It can be something as 16 DWORDS that gpu will read and trigger its internal rasterizer state
static winrt::com_ptr< ID3D12PipelineState>  CreateGaussianBlurPipelineState(ID3D12Device1* device, ID3D12RootSignature* root)
{
    static
#include <gaussian_blur.h>

        D3D12_COMPUTE_PIPELINE_STATE_DESC state = {};
    state.pRootSignature = root;

    state.CS = { &g_gaussian_blur[0], sizeof(g_gaussian_blur) };

    winrt::com_ptr<ID3D12PipelineState> r;

    ThrowIfFailed(device->CreateComputePipelineState(&state, __uuidof(ID3D12PipelineState), r.put_void()));
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
        ApplicationView::PreferredLaunchViewSize(Size(1600, 900));
        ApplicationView::PreferredLaunchWindowingMode(ApplicationViewWindowingMode::PreferredLaunchViewSize);

        m_activated                 = v.Activated(winrt::auto_revoke, { this, &ViewProvider::OnActivated });
        m_debug                     = CreateDebug();
        m_device                    = CreateDevice();

        m_graphics_queue            = CreateGraphicsQueue(m_device.get());
        m_compute_queue             = CreateComputeQueue(m_device.get());

        m_HeapTargets               = CreateRenderTargetsHeap(m_device.get());

        m_descriptorHeapTargets     = CreateDescriptorHeapTargets(m_device.get());
        m_descriptorHeapDepth       = CreateDescriptorHeapDepth(m_device.get());

        m_descriptorHeapShader      = CreateDescriptorHeapShader(m_device.get());
        m_descriptorHeapShaderGpu   = CreateDescriptorHeapShaderGpu(m_device.get());

        //if you have many threads that generate commands. 1 per thread per frame
        m_graphics_allocator[0][0]  = CreateGraphicsAllocator(m_device.get(), L"GraphicsAllocator00");
        m_graphics_allocator[0][1]  = CreateGraphicsAllocator(m_device.get(), L"GraphicsAllocator01");
        m_graphics_allocator[0][2]  = CreateGraphicsAllocator(m_device.get(), L"GraphicsAllocator02");

        m_graphics_allocator[1][0]  = CreateGraphicsAllocator(m_device.get(), L"GraphicsAllocator10");
        m_graphics_allocator[1][1]  = CreateGraphicsAllocator(m_device.get(), L"GraphicsAllocator11");
        m_graphics_allocator[1][2]  = CreateGraphicsAllocator(m_device.get(), L"GraphicsAllocator12");

        m_graphics_list[0][0]       = CreateGraphicsList(m_device.get(), m_graphics_allocator[0][0].get());
        m_graphics_list[0][1]       = CreateGraphicsList(m_device.get(), m_graphics_allocator[0][1].get());
        m_graphics_list[0][2]       = CreateGraphicsList(m_device.get(), m_graphics_allocator[0][2].get());

        m_graphics_list[1][0]       = CreateGraphicsList(m_device.get(), m_graphics_allocator[1][0].get());
        m_graphics_list[1][1]       = CreateGraphicsList(m_device.get(), m_graphics_allocator[1][1].get());
        m_graphics_list[1][2]       = CreateGraphicsList(m_device.get(), m_graphics_allocator[1][2].get());

        m_compute_allocator[0]      = CreateComputeAllocator(m_device.get(),L"ComputeAllocator0");
        m_compute_allocator[1]      = CreateComputeAllocator(m_device.get(),L"ComputeAllocator1");

        m_compute_list[0]           = CreateComputeList(m_device.get(), m_compute_allocator[0].get());
        m_compute_list[1]           = CreateComputeList(m_device.get(), m_compute_allocator[1].get());

        //fence, sync from the gpu and cpu
        m_graphics_fence            = CreateFence(m_device.get());
        m_graphics_event            = CreateEvent(nullptr, false, false, nullptr);

        m_compute_fence             = CreateFence(m_device.get());
        m_compute_event             = CreateEvent(nullptr, false, false, nullptr);
    }

    void Uninitialize() 
    {

    }

    void Load(winrt::hstring h)
    {
        m_graphics_signature            = CreateGraphicsRootSignature(m_device.get());
        m_compute_signature             = CreateComputeRootSignature(m_device.get());
        m_triangle_state                = CreateTrianglePipelineState(m_device.get(), m_graphics_signature.get());
        m_triangle_state_depth_prepass  = CreateTriangleDepthPipelineState(m_device.get(), m_graphics_signature.get());
        m_gaussian_blur                 = CreateGaussianBlurPipelineState(m_device.get(), m_compute_signature.get());
    }

	void CreateViewDependentResources(uint32_t width, uint32_t height)
	{
		m_back_buffer_width = width;
		m_back_buffer_height = height;

        m_lighting_buffer[0]      = nullptr;
        m_lighting_buffer[1]      = nullptr;
        m_lighting_buffer_msaa[0] = nullptr;
        m_lighting_buffer_msaa[1] = nullptr;
        m_swap_chain_buffers[0]   = nullptr;
        m_swap_chain_buffers[1]   = nullptr;


		//allocate memory for the view
		m_swap_chain_buffers[0] = CreateSwapChainResource(m_device.get(), m_swap_chain.get(), 0);
		m_swap_chain_buffers[1] = CreateSwapChainResource(m_device.get(), m_swap_chain.get(), 1);

		m_swap_chain_buffers[0]->SetName(L"Buffer 0");
		m_swap_chain_buffers[1]->SetName(L"Buffer 1");

		//Where are located the descriptors
		m_swap_chain_descriptor[0] = 0;
		m_swap_chain_descriptor[1] = 1;

		//create render target views, that will be used for rendering
		CreateRenderTargetDescriptor(m_device.get(), m_swap_chain_buffers[0].get(), CpuView(m_device.get(), m_descriptorHeapTargets.get()) + 0);
		CreateRenderTargetDescriptor(m_device.get(), m_swap_chain_buffers[1].get(), CpuView(m_device.get(), m_descriptorHeapTargets.get()) + 1);

		m_depth_buffer = CreateDepthResource(m_device.get(), m_back_buffer_width / 2, m_back_buffer_height / 2);
		m_depth_buffer->SetName(L"Depth Buffer");

		CreateDepthWriteDescriptor(m_device.get(), m_depth_buffer.get(), CpuView(m_device.get(), m_descriptorHeapDepth.get()) + 0);
		CreateDepthReadDescriptor(m_device.get(), m_depth_buffer.get(), CpuView(m_device.get(), m_descriptorHeapDepth.get()) + 1);

		m_depth_descriptor[0] = 0;
		m_depth_descriptor[1] = 1;

		//create render target views, that will be used for rendering
        m_lighting_buffer[0] = CreateLightingResource(m_device.get(), m_back_buffer_width, m_back_buffer_height, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);// D3D12_RESOURCE_STATE_COPY_SOURCE);
        m_lighting_buffer[1] = CreateLightingResource(m_device.get(), m_back_buffer_width, m_back_buffer_height, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);// D3D12_RESOURCE_STATE_COPY_SOURCE);// D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        m_lighting_buffer_msaa[0] = CreateLightingResourceMSAA(m_device.get(), m_HeapTargets.get(), 0, m_back_buffer_width / 2, m_back_buffer_height / 2);
        m_lighting_buffer_msaa[1] = CreateLightingResourceMSAA(m_device.get(), m_HeapTargets.get(), 1, m_back_buffer_width / 2, m_back_buffer_height / 2);

		m_lighting_buffer[0]->SetName(L"Lighting Buffer 0");
		m_lighting_buffer[1]->SetName(L"Lighting Buffer 1");

        m_lighting_buffer_msaa[0]->SetName(L"MSAA Lighting Buffer 0");
        m_lighting_buffer_msaa[1]->SetName(L"MSAA Lighting Buffer 1");

		//create render target views, that will be used for rendering
		CreateRenderTargetDescriptor(m_device.get(), m_lighting_buffer_msaa[0].get(), CpuView(m_device.get(), m_descriptorHeapTargets.get()) + 2);
		CreateRenderTargetDescriptor(m_device.get(), m_lighting_buffer_msaa[1].get(), CpuView(m_device.get(), m_descriptorHeapTargets.get()) + 3);

        CreateShaderResourceViewDescriptor(m_device.get(), m_lighting_buffer_msaa[0].get(), CpuView(m_device.get(), m_descriptorHeapShader.get()) + 0);
        CreateShaderResourceViewDescriptor(m_device.get(), m_lighting_buffer_msaa[1].get(), CpuView(m_device.get(), m_descriptorHeapShader.get()) + 1);

        CreateUnorderedAccessViewDescriptor(m_device.get(), m_lighting_buffer[0].get(), CpuView(m_device.get(), m_descriptorHeapShader.get()) + 2, m_back_buffer_width, m_back_buffer_height);
        CreateUnorderedAccessViewDescriptor(m_device.get(), m_lighting_buffer[1].get(), CpuView(m_device.get(), m_descriptorHeapShader.get()) + 3, m_back_buffer_width, m_back_buffer_height);

        m_lighting_descriptor[0]     = 2;
		m_lighting_descriptor[1]     = 3;

		m_lighting_descriptor_uav[0] = 0;
		m_lighting_descriptor_uav[1] = 1;

        m_lighting_descriptor_uav[2] = 2;
        m_lighting_descriptor_uav[3] = 3;

		//Copy
		m_device->CopyDescriptorsSimple(4, CpuView(m_device.get(), m_descriptorHeapShaderGpu.get()) + m_lighting_descriptor_uav[0], CpuView(m_device.get(), m_descriptorHeapShader.get()) + m_lighting_descriptor_uav[0], D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

    void SetWindow(const CoreWindow& w)
    {
        auto e = sample::build_environment(w, winrt::Windows::Graphics::Display::DisplayInformation::GetForCurrentView());
        m_closed = w.Closed(winrt::auto_revoke, { this, &ViewProvider::OnWindowClosed });
        m_size_changed = w.SizeChanged(winrt::auto_revoke, { this, &ViewProvider::OnWindowSizeChanged });

		uint32_t width  = static_cast<UINT>(e.m_back_buffer_size.Width);
		uint32_t height = static_cast<UINT>(e.m_back_buffer_size.Height);

		m_swap_chain = CreateSwapChain(w, m_graphics_queue.get(), width, height);
        m_graphics_frame_index = m_swap_chain->GetCurrentBackBufferIndex();

		CreateViewDependentResources(width, height);
    }

    void OnWindowClosed(const CoreWindow & w, const CoreWindowEventArgs & a)
    {
        m_window_running = false;
    }

    void OnActivated(const CoreApplicationView&, const IActivatedEventArgs&)
    {
        CoreWindow::GetForCurrentThread().Activate();
    }

    void OnWindowSizeChanged(const CoreWindow & w, const WindowSizeChangedEventArgs & a)
    {
        //wait for the render thread to finish and block it so we can submit a command
        std::lock_guard lock(m_blockRendering);

        //Now wait for the gpu to finish what it has from the main thread

		ThrowIfFailed(m_graphics_queue->Signal(m_graphics_fence.get(), m_frame_number + 1));
		ThrowIfFailed(m_graphics_fence->SetEventOnCompletion(m_frame_number + 1, m_graphics_event));
		WaitForSingleObject(m_graphics_event, INFINITE);

		ThrowIfFailed(m_compute_queue->Signal(m_compute_fence.get(), m_frame_number + 1));
		ThrowIfFailed(m_compute_fence->SetEventOnCompletion(m_frame_number + 1, m_compute_event));
		WaitForSingleObject(m_compute_event, INFINITE);

		//Prepare to unblock the rendering
		m_frame_number = m_frame_number + 1;

		m_swap_chain_buffers[0] = nullptr;
		m_swap_chain_buffers[1] = nullptr;

		auto e = sample::build_environment(w, winrt::Windows::Graphics::Display::DisplayInformation::GetForCurrentView());
		uint32_t width = static_cast<UINT>(e.m_back_buffer_size.Width);
		uint32_t height = static_cast<UINT>(e.m_back_buffer_size.Height);

		m_swap_chain->ResizeBuffers(2, width, height, DXGI_FORMAT_B8G8R8A8_UNORM, 0);

		m_swap_chain->Present(1, 0);
		
		m_graphics_frame_index = 1 - m_swap_chain->GetCurrentBackBufferIndex();
		CreateViewDependentResources(static_cast<UINT>(e.m_back_buffer_size.Width), static_cast<UINT>(e.m_back_buffer_size.Height));
    }

    void Run()
    {
        m_swap_chain->Present(0,0);

        while (m_window_running)
        {
            using namespace sample;
            CoreWindow::GetForCurrentThread().Dispatcher().ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);

            std::lock_guard lock(m_blockRendering);

            uint32_t graphics_frame_index = m_graphics_frame_index;
            uint32_t compute_frame_index = 1 - m_graphics_frame_index;

            {
                ID3D12CommandAllocator* computeAllocator = m_compute_allocator[graphics_frame_index].get();
                computeAllocator->Reset();
                ID3D12GraphicsCommandList1* computeList = m_compute_list[graphics_frame_index].get();
                computeList->Reset(computeAllocator, nullptr);
            }

            for (auto i = 0; i < 3; ++i)
            {
                ID3D12CommandAllocator* graphicsAllocator   = m_graphics_allocator[graphics_frame_index][i].get();
                ID3D12GraphicsCommandList1* graphicsList    = m_graphics_list[graphics_frame_index][i].get();
                graphicsAllocator->Reset();
                graphicsList->Reset(graphicsAllocator, nullptr);
            }

            //Compute queue
            {

                ID3D12GraphicsCommandList1* computeList = m_compute_list[graphics_frame_index].get();
                PIXBeginEvent(computeList, 0, L"PostProcess");
                // Set Descriptor heaps 
                ID3D12DescriptorHeap* heaps[] = { m_descriptorHeapShaderGpu.get() };
                computeList->SetDescriptorHeaps(1, heaps);

                //Now do post processing
                {
                    //set the type of the parameters that we will use in the shader
                    computeList->SetComputeRootSignature(m_compute_signature.get());
                    computeList->SetComputeRootDescriptorTable(1, GpuView(m_device.get(), m_descriptorHeapShaderGpu.get()) + ( m_lighting_descriptor_uav[compute_frame_index] + 2));
                    computeList->SetComputeRootDescriptorTable(2, GpuView(m_device.get(), m_descriptorHeapShaderGpu.get()) + ( m_lighting_descriptor_uav[compute_frame_index] + 0));
                    computeList->SetPipelineState(m_gaussian_blur.get());

                    uint32_t x = (m_back_buffer_width + 7) / 8;
                    uint32_t y = (m_back_buffer_height+ 7) / 8;

                    computeList->Dispatch(x, y , 1);
                }

                PIXEndEvent(computeList);
                computeList->Close();
                
            }

            //Graphics Queue, Depth Prepass
            {
                ID3D12GraphicsCommandList1* graphicsList = m_graphics_list[graphics_frame_index][0].get();
                PIXBeginEvent(graphicsList, 0, "Depth Prepass");


                D3D12_SAMPLE_POSITION pos[4] =
                {
                    {-4,-4},
                    {4,-4},
                    {-4,4},
                    {4,4},
                };

                graphicsList->SetSamplePositions(4, 1, &pos[0]);


                //Depth prepass for frame N
                //get the pointer to the gpu memory
                D3D12_CPU_DESCRIPTOR_HANDLE depth_buffer = CpuView(m_device.get(), m_descriptorHeapDepth.get()) + m_depth_descriptor[0];
                {
                    graphicsList->OMSetRenderTargets(0, 0, TRUE, &depth_buffer);
                    graphicsList->ClearDepthStencilView(depth_buffer, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

                    //set the type of the parameters that we will use in the shader
                    graphicsList->SetGraphicsRootSignature(m_graphics_signature.get());

                    //set the scissor test separately (which parts of the view port will survive)
                    {
                        D3D12_RECT r = { 0, 0, static_cast<int32_t>(m_back_buffer_width), static_cast<int32_t>(m_back_buffer_height) };
                        graphicsList->RSSetScissorRects(1, &r);
                    }

                    //set the viewport. 
                    {
                        D3D12_VIEWPORT v;
                        v.TopLeftX = 0;
                        v.TopLeftY = 0;
                        v.MinDepth = 0.0f;
                        v.MaxDepth = 1.0f;
                        v.Width = static_cast<float>(m_back_buffer_width / 2);
                        v.Height = static_cast<float>(m_back_buffer_height / 2);
                        graphicsList->RSSetViewports(1, &v);
                    }

                    //set the types of the triangles we will use
                    {
                        graphicsList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                    }

                    //Draw first to clear the depth buffer
                    //set the raster pipeline state as a whole, it was prebuilt before
                    graphicsList->SetPipelineState(m_triangle_state_depth_prepass.get());
                    //draw the triangle
                    graphicsList->DrawInstanced(3, 1, 0, 0);
                }

                PIXEndEvent(graphicsList);
                graphicsList->Close();
            }

            //Graphics Queue, Color Pass
            {
                ID3D12GraphicsCommandList1* graphicsList = m_graphics_list[graphics_frame_index][1].get();

                PIXBeginEvent(graphicsList, 0, "Heavy Load Pass");

                {
                    D3D12_RESOURCE_BARRIER barrier[1] = {};

                    barrier[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                    barrier[0].Transition.pResource     = m_lighting_buffer_msaa[graphics_frame_index].get();
                    barrier[0].Transition.StateBefore   = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
                    barrier[0].Transition.StateAfter    = D3D12_RESOURCE_STATE_RENDER_TARGET;
                    barrier[0].Transition.Subresource   = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                    graphicsList->ResourceBarrier(1, &barrier[0]);
                }

                graphicsList->DiscardResource(m_lighting_buffer_msaa[graphics_frame_index].get(), nullptr);


                {
                    D3D12_SAMPLE_POSITION pos[4] =
                    {
                        {-4,-4},
                        {4,-4},
                        {-4,4},
                        {4,4},
                    };

                    graphicsList->SetSamplePositions(4, 1, &pos[0]);
                }

                //Color pass for frame N
                //get the pointer to the gpu memory
                D3D12_CPU_DESCRIPTOR_HANDLE back_buffer     = CpuView(m_device.get(), m_descriptorHeapTargets.get()) + m_lighting_descriptor[graphics_frame_index];
                D3D12_CPU_DESCRIPTOR_HANDLE depth_buffer    = CpuView(m_device.get(), m_descriptorHeapDepth.get())   + m_depth_descriptor[0];
                {
                    graphicsList->OMSetRenderTargets(1, &back_buffer, TRUE, &depth_buffer);
                    FLOAT c[4] = { 0.0f, 0.f,0.f,0.f };
                    graphicsList->ClearRenderTargetView(back_buffer, c, 0, nullptr);

                    //set the type of the parameters that we will use in the shader
                    graphicsList->SetGraphicsRootSignature(m_graphics_signature.get());

                    //set the scissor test separately (which parts of the view port will survive)
                    {
                        D3D12_RECT r = { 0, 0, static_cast<int32_t>(m_back_buffer_width), static_cast<int32_t>(m_back_buffer_height) };
                        graphicsList->RSSetScissorRects(1, &r);
                    }

                    //set the viewport. 
                    {
                        D3D12_VIEWPORT v;
                        v.TopLeftX = 0;
                        v.TopLeftY = 0;
                        v.MinDepth = 0.0f;
                        v.MaxDepth = 1.0f;
                        v.Width = static_cast<float>(m_back_buffer_width / 2 );
                        v.Height = static_cast<float>(m_back_buffer_height / 2);
                        graphicsList->RSSetViewports(1, &v);
                    }

                    graphicsList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                    graphicsList->SetPipelineState(m_triangle_state.get());
                    graphicsList->DrawInstanced(3, 1, 0, 0);
                }

                {
                    D3D12_RESOURCE_BARRIER barrier[1] = {};

                    barrier[0].Type                     = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                    barrier[0].Transition.pResource     = m_lighting_buffer_msaa[graphics_frame_index].get();
                    barrier[0].Transition.StateBefore   = D3D12_RESOURCE_STATE_RENDER_TARGET;
                    barrier[0].Transition.StateAfter    = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
                    barrier[0].Transition.Subresource   = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

                    graphicsList->ResourceBarrier(1, &barrier[0]);
                }

                PIXEndEvent(graphicsList);

                graphicsList->Close();
            
            }

            //Graphics queue, present to the backbuffer
            {
                ID3D12GraphicsCommandList1* graphicsList = m_graphics_list[graphics_frame_index][2].get();

                //PIXBeginEvent(graphicsList, 0, "Present Frame");

                {
                    D3D12_RESOURCE_BARRIER barrier[2] = {};

                    barrier[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                    barrier[0].Transition.pResource     = m_lighting_buffer[compute_frame_index].get();
                    barrier[0].Transition.StateBefore   = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
                    barrier[0].Transition.StateAfter    = D3D12_RESOURCE_STATE_COPY_SOURCE;
                    barrier[0].Transition.Subresource   = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

                    barrier[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                    barrier[1].Transition.pResource     = m_swap_chain_buffers[compute_frame_index].get();
                    barrier[1].Transition.StateBefore   = D3D12_RESOURCE_STATE_PRESENT;
                    barrier[1].Transition.StateAfter    = D3D12_RESOURCE_STATE_COPY_DEST;
                    barrier[1].Transition.Subresource   = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

                    graphicsList->ResourceBarrier(2, &barrier[0]);
                }

                D3D12_TEXTURE_COPY_LOCATION s = {};

                s.pResource = m_lighting_buffer[compute_frame_index].get();
                s.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
                s.SubresourceIndex = 0;

                D3D12_TEXTURE_COPY_LOCATION d = {};

                d.pResource = m_swap_chain_buffers[compute_frame_index].get();
                d.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
                d.SubresourceIndex = 0;

                graphicsList->CopyTextureRegion(&d, 0, 0, 0, &s, nullptr);

                {
                    D3D12_RESOURCE_BARRIER barrier[2] = {};

                    barrier[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                    barrier[0].Transition.pResource     = m_lighting_buffer[compute_frame_index].get();
                    barrier[0].Transition.StateBefore   = D3D12_RESOURCE_STATE_COPY_SOURCE;
                    barrier[0].Transition.StateAfter    = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
                    barrier[0].Transition.Subresource   = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

                    barrier[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                    barrier[1].Transition.pResource     = m_swap_chain_buffers[compute_frame_index].get();
                    barrier[1].Transition.StateBefore   = D3D12_RESOURCE_STATE_COPY_DEST;
                    barrier[1].Transition.StateAfter    = D3D12_RESOURCE_STATE_PRESENT;
                    barrier[1].Transition.Subresource   = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                    graphicsList->ResourceBarrier(2, &barrier[0]);
                }

                graphicsList->Close();

                //PIXEndEvent(graphicsList);
            }

            {
                //Wait for the graphics, execute the compute work load
                m_compute_queue->Wait(m_graphics_fence.get(), m_frame_number - 1);
                {
                    ID3D12GraphicsCommandList1* computeList = m_compute_list[graphics_frame_index].get();
                    ID3D12CommandList* lists[] = { computeList };
                    m_compute_queue->ExecuteCommandLists(1, lists);
                }
                ThrowIfFailed(m_compute_queue->Signal(m_compute_fence.get(), m_frame_number - 1));
            }

            {
                //execute depth prepass
                {
                    ID3D12CommandList* lists[] = { m_graphics_list[graphics_frame_index][0].get() };
                    m_graphics_queue->ExecuteCommandLists(1, lists);
                }
            }

            {
                //wait for the compute work of the previous frame
                m_graphics_queue->Wait(m_compute_fence.get(), m_frame_number - 1);

                {
                    ID3D12CommandList* lists[] = { 
                    
                        m_graphics_list[graphics_frame_index][2].get(),
                        m_graphics_list[graphics_frame_index][1].get()
                    };

                    m_graphics_queue->ExecuteCommandLists(2, lists);
                }

                ThrowIfFailed(m_graphics_queue->Signal(m_graphics_fence.get(), m_frame_number));
            }

            m_swap_chain->Present(1, 0);    //present the swap chain

            {
                //Now block the cpu until the gpu if it gets too far
                if (m_graphics_fence->GetCompletedValue() < m_frame_number - 1)
                {
                    ThrowIfFailed(m_graphics_fence->SetEventOnCompletion(m_frame_number - 1, m_graphics_event));
                    WaitForSingleObject(m_graphics_event, INFINITE);
                }
            }

            m_frame_number = m_frame_number + 1;
            m_graphics_frame_index = 1 - m_graphics_frame_index;
        }
    }

    bool m_window_running = true;

    CoreWindow::Closed_revoker                  m_closed;
    CoreWindow::SizeChanged_revoker             m_size_changed;
    CoreApplicationView::Activated_revoker      m_activated;
    
    winrt::com_ptr <ID3D12Debug>                m_debug;
    winrt::com_ptr <ID3D12Device1>              m_device;                       //device for gpu resources
    winrt::com_ptr <IDXGISwapChain3>            m_swap_chain;                   //swap chain for 

    winrt::com_ptr <ID3D12Fence>                m_graphics_fence;               //fence for cpu/gpu synchronization
    winrt::com_ptr <ID3D12Fence>                m_compute_fence;                //fence for compute queue

    winrt::com_ptr <ID3D12CommandQueue>         m_compute_queue;                //queue to the device
    winrt::com_ptr <ID3D12CommandQueue>         m_graphics_queue;               //queue to the device

    winrt::com_ptr <ID3D12Heap>                 m_HeapTargets;                  //descriptor heap for the resources

    winrt::com_ptr <ID3D12DescriptorHeap>       m_descriptorHeapTargets;        //descriptor heap for the resources
    winrt::com_ptr <ID3D12DescriptorHeap>       m_descriptorHeapDepth;          //descriptor heap for the resources
    winrt::com_ptr <ID3D12DescriptorHeap>       m_descriptorHeapShader;         //descriptor heap for the resources
    winrt::com_ptr <ID3D12DescriptorHeap>       m_descriptorHeapShaderGpu;      //descriptor heap for the resources

    std::mutex                                  m_blockRendering;               //block render thread for the swap chain resizes

    winrt::com_ptr<ID3D12Resource1>             m_swap_chain_buffers[2];        //back buffer resources
    uint64_t                                    m_swap_chain_descriptor[2];
    winrt::com_ptr<ID3D12Resource1>             m_depth_buffer;
    uint64_t                                    m_depth_descriptor[2];

    winrt::com_ptr<ID3D12Resource1>             m_lighting_buffer[2];           //lighting buffers ( ComputeGraphicsLatency + 1 )
    winrt::com_ptr<ID3D12Resource1>             m_lighting_buffer_msaa[2];      //lighting msaa    ( ComputeGraphicsLatency + 1 )
    uint64_t                                    m_lighting_descriptor[2];
    uint64_t                                    m_lighting_descriptor_uav[4];

    winrt::com_ptr<ID3D12Resource1>             m_depth;                        //depth buffer

    uint32_t                                    m_back_buffer_width = 0;
    uint32_t                                    m_back_buffer_height = 0;

    winrt::com_ptr <ID3D12CommandAllocator>     m_graphics_allocator[2][3];     //one per frame
    winrt::com_ptr <ID3D12CommandAllocator>     m_compute_allocator[2];         //one per frame
    winrt::com_ptr <ID3D12GraphicsCommandList1> m_graphics_list[2][3];          //three per frame 
    winrt::com_ptr <ID3D12GraphicsCommandList1> m_compute_list[2];              //one per frame 

    uint64_t                                    m_frame_number          = 1;
    uint64_t                                    m_graphics_frame_index  = 0;

	HANDLE                                      m_graphics_event        = {};
    HANDLE                                      m_compute_event         = {};

    //Signatures
    winrt::com_ptr< ID3D12RootSignature>        m_compute_signature;
    winrt::com_ptr< ID3D12RootSignature>        m_graphics_signature;

    //States
    winrt::com_ptr< ID3D12PipelineState>        m_triangle_state;
    winrt::com_ptr< ID3D12PipelineState>        m_triangle_state_depth_prepass;
    winrt::com_ptr< ID3D12PipelineState>        m_gaussian_blur;
};

int32_t __stdcall wWinMain( HINSTANCE, HINSTANCE,PWSTR, int32_t )
{
    ThrowIfFailed(CoInitializeEx(nullptr, COINIT_MULTITHREADED));
    CoreApplication::Run(ViewProvider());
    CoUninitialize();
    return 0;
}
