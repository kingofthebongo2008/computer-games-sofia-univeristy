#include "pch.h"
#include <cstdint>
#include "build_window_environment.h"

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

    d.NumDescriptors = 3;
    d.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    device->CreateDescriptorHeap(&d, __uuidof(ID3D12DescriptorHeap), r.put_void());
    return r;
}

static winrt::com_ptr <ID3D12DescriptorHeap> CreateDescriptorHeapRendering(ID3D12Device1* device)
{
    winrt::com_ptr<ID3D12DescriptorHeap> r;
    D3D12_DESCRIPTOR_HEAP_DESC d = {};

    d.NumDescriptors = 3;
    d.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
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

namespace lispsm
{
    struct float4
    {
        float x;
        float y;
        float z;
        float w;

        float4()
        {

        }

        float4(float v0, float v1, float v2, float v3) 
        {
            x = v0;
            y = v1;
            z = v2;
            w = v3;
        }
    };

    struct matrix44
    {
        float4 r[4];
    };


    struct float3
    {
        float x;
        float y;
        float z;

        float3()
        {

        }

        float3(float v0, float v1, float v2)
        {
            x = v0;
            y = v1;
            z = v2;
        }
    };

    struct float2
    {
        float x;
        float y;

        float2()
        {

        }

        float2(float v0, float v1)
        {
            x = v0;
            y = v1;
        }
    };

    struct vector4
    {
        float4 m_value;

        vector4() {}
        vector4(float  v0, float  v1, float  v2, float  v3) { m_value.x = v0; m_value.y = v1; m_value.z = v2; m_value.w = v3; }
        vector4(float4 v) { m_value = v; }
        vector4(float3 v)
        {
            m_value.x = v.x;
            m_value.y = v.y;
            m_value.z = v.z;
            m_value.w = 0.0;
        }
    };

    struct vector3
    {
        float3 m_value;

        vector3() {}
        vector3(float  v0, float  v1, float  v2) { m_value.x = v0; m_value.y = v1; m_value.z = v2;}
        vector3(float3 v) { m_value = v; }
        vector3(float2 v)
        {
            m_value.x = v.x;
            m_value.y = v.y;
            m_value.z = 0.0;
        }
    };

    struct point4
    {
        float4 m_value;

        point4() {}
        point4(float  v0, float  v1, float  v2, float  v3) { m_value.x = v0; m_value.y = v1; m_value.z = v2; m_value.w = v3; }
        point4(float4 v) { m_value = v; }
        point4(float3 v)
        {
            m_value.x = v.x;
            m_value.y = v.y;
            m_value.z = v.z;
            m_value.w = 0.0;
        }
    };

    struct point3
    {
        float3 m_value;

        point3() {}
        point3(float  v0, float  v1, float  v2) { m_value.x = v0; m_value.y = v1; m_value.z = v2; }
        point3(float3 v) { m_value = v; }
        point3(float2 v)
        {
            m_value.x = v.x;
            m_value.y = v.y;
            m_value.z = 0.0f;
        }
    };

    vector4 make_vector4(vector3 v)
    {
        vector4 r;

        r.m_value.x = r.m_value.x;
        r.m_value.y = r.m_value.y;
        r.m_value.z = r.m_value.z;
        r.m_value.w = 0.0f;
        return r;
    }

    vector3 unit_x()
    {
        vector3 v;
        v.m_value.x = 1.0f;
        v.m_value.y = 0.0f;
        v.m_value.z = 0.0f;
        return v;
    }

    vector3 unit_y()
    {
        vector3 v;
        v.m_value.x = 0.0f;
        v.m_value.y = 1.0f;
        v.m_value.z = 0.0f;
        return v;
    }

    vector3 unit_z()
    {
        vector3 v;
        v.m_value.x = 0.0f;
        v.m_value.y = 0.0f;
        v.m_value.z = 1.0f;
        return v;
    }

    vector3 mul(vector3 v, float scalar)
    {
        vector3 r;

        r.m_value.x = v.m_value.x * scalar;
        r.m_value.y = v.m_value.y * scalar;
        r.m_value.z = v.m_value.z * scalar;

        return r;
    }

    vector3 add(vector3 v0, vector3 v1)
    {
        vector3 r;

        r.m_value.x = v0.m_value.x + v1.m_value.x;
        r.m_value.y = v0.m_value.y + v1.m_value.y;
        r.m_value.z = v0.m_value.z + v1.m_value.z;
        return r;
    }

    point3 add(point3 v0, vector3 v1)
    {
        point3 r;

        r.m_value.x = v0.m_value.x + v1.m_value.x;
        r.m_value.y = v0.m_value.y + v1.m_value.y;
        r.m_value.z = v0.m_value.z + v1.m_value.z;

        return r;
    }

    vector3 sub(point3 v0, point3 v1)
    {
        vector3 v;

        v.m_value.x = v0.m_value.x - v0.m_value.x;
        v.m_value.y = v0.m_value.y - v0.m_value.y;
        v.m_value.z = v0.m_value.z - v0.m_value.z;
        return v;
    }

    vector3 negate(vector3 v0)
    {
        vector3 r;

        r.m_value.x = -v0.m_value.x;
        r.m_value.y = -v0.m_value.y;
        r.m_value.z = -v0.m_value.z;
        return r;
    }

    vector3 subtract(vector3 v0, vector3 v1)
    {
        return add(v0, negate(v1));
    }

    float dot(vector3 v0, vector3 v1)
    {
        return v0.m_value.x * v1.m_value.x + v0.m_value.y * v1.m_value.y + v0.m_value.z * v1.m_value.z;
    }

    vector3 normalize(vector3 v)
    {
        float norm = dot(v, v);
        return mul(v, 1.0f / norm);
    }

    vector3 cross(vector3 v0, vector3 v1)
    {
        vector3 r;

        float ax = v0.m_value.x;
        float ay = v0.m_value.y;
        float az = v0.m_value.z;

        float bx = v1.m_value.x;
        float by = v1.m_value.y;
        float bz = v1.m_value.z;


        r.m_value.x = ay * bz - az * by;
        r.m_value.y = az * bx - ax * bz;
        r.m_value.z = ax * by - ay * bx;

        return r;
    }

    point3 zero()
    {
        point3 v;
        v.m_value.x = 0.0f;
        v.m_value.y = 0.0f;
        v.m_value.z = 0.0f;
        return v;
    }

    struct aabb
    {
        float3 m_min;
        float3 m_max;
    };

    struct distance
    {
        float m_value;
    };

    struct camera
    {
        point3      m_position;
        distance    m_near;
        vector3     m_direction;
        distance    m_far;
        vector3     m_up;
    };

    struct radian
    {
        float m_value;
    };

    struct degree
    {
        float m_value;
    };

    struct ratio
    {
        float m_value;
    };

    struct ortho_camera : camera
    {
        float m_left;
        float m_right;
        float m_top;
        float m_bottom;
    };

    struct perspective_camera : camera
    {
        ratio  m_aspect;
        radian m_fov_y;
    };

    vector3 up(const camera c)
    {
        return c.m_up;
    }

    vector3 forward(const camera c)
    {
        return c.m_direction;
    }

    vector3 right(const camera c)
    {
        return cross(up(c), forward(c));
    }

    point3 position(const camera c)
    {
        return c.m_position;
    }

    float project(vector3 v, point3 p)
    {
        vector3 v0;
        v0.m_value.x = p.m_value.x;
        v0.m_value.y = p.m_value.y;
        v0.m_value.z = p.m_value.z;
        return dot(v, v0);
    }

    struct view_transform
    {
        matrix44 m_matrix;
    };

    struct perspective_transform
    {
        matrix44 m_matrix;
    };

    view_transform make_view_transform(const camera c)
    {
        matrix44 r;

        vector3 translation;

        vector3 right_          = right(c);
        vector3 up_             = up(c);
        vector3 forward_        = forward(c);

        translation.m_value.x   = project(right_, position(c));
        translation.m_value.y   = project(up_, position(c));
        translation.m_value.z   = project(forward_, position(c));
        translation             = negate(translation);

        r.r[0]                  = float4(right_.m_value.x, right_.m_value.y, right_.m_value.z, 0.0);
        r.r[1]                  = float4(up_.m_value.x, up_.m_value.y, up_.m_value.z, 0.0);
        r.r[2]                  = float4(forward_.m_value.x, forward_.m_value.y, forward_.m_value.z, 0.0);
        r.r[3]                  = float4(translation.m_value.x, translation.m_value.y, translation.m_value.z, 1.0);

        view_transform t;
        t.m_matrix = r;
        return t;
    }

    matrix44 perspective_matrix(const perspective_camera c)
    {
        matrix44 r;

        float sinFov = sinf(c.m_fov_y.m_value / 2.0f);
        float cosFov = cosf(c.m_fov_y.m_value / 2.0f);

        float height = cosFov / sinFov;
        float width  = c.m_aspect.m_value * height;
        float nearz  = c.m_near.m_value;
        float farz   = c.m_far.m_value;
        float range  = farz / (farz - nearz);

        r.r[0]  = float4(width,    0,      0,                0);
        r.r[1]  = float4(0,        height, 0,                0);
        r.r[2]  = float4(0,        0,      range,            0);
        r.r[3]  = float4(0,        0,      -range * nearz,   0);

        return r;
    }

    perspective_transform make_perspective_transform(const perspective_camera c)
    {
        matrix44 r;
        perspective_transform t;
        t.m_matrix = r;
        return t;
    }

    matrix44 perspective_matrix(const ortho_camera c)
    {
        matrix44 r;

        return r;
    }

    point4 transform_point( view_transform m, point4 p )
    {
        point4  r;
        point4  p0;
        float4  v    = m.m_matrix.r[3];

        p0.m_value.x = p.m_value.x;
        p0.m_value.y = p.m_value.y;
        p0.m_value.z = p.m_value.z;
        p0.m_value.w = 1.0;

        r.m_value.x  = m.m_matrix.r[0].x * p0.m_value.x + m.m_matrix.r[1].x * p0.m_value.y + m.m_matrix.r[2].x * p0.m_value.z + m.m_matrix.r[3].x * p0.m_value.w;
        r.m_value.y  = m.m_matrix.r[0].y * p0.m_value.x + m.m_matrix.r[1].y * p0.m_value.y + m.m_matrix.r[2].y * p0.m_value.z + m.m_matrix.r[3].y * p0.m_value.w;
        r.m_value.z  = m.m_matrix.r[0].z * p0.m_value.x + m.m_matrix.r[1].z * p0.m_value.y + m.m_matrix.r[2].z * p0.m_value.z + m.m_matrix.r[3].z * p0.m_value.w;

        return r;
    }

    point3 transform(matrix44 m, point3 p)
    {
        return { 0,1,0 };
    }

    vector3 project_view_direction_in_light_space(matrix44 light, point3 closest_point_to_camera_ws, vector3 camera_dir_ws )
    {
        point3  b = closest_point_to_camera_ws;
        point3  e = add(closest_point_to_camera_ws, camera_dir_ws);

        point3  b_ls            = transform(light, b);
        point3  e_ls            = transform(light, e);

        vector3 projected_dir   = sub(e_ls, b_ls);
        projected_dir.m_value.y = 0;

        return normalize(projected_dir);
    }

    point3 get_closest_point(point3 frustum_points_ws[8], point3 camera_position_ws)
    {
        vector3 min_difference      = sub(camera_position_ws, frustum_points_ws[0]);
        float min_norm_squared      = dot(min_difference, min_difference);
        point3 min_point            = frustum_points_ws[0];

        for (uint32_t i = 0U; i < 7; ++i)
        {
            vector3 difference      = sub(camera_position_ws, frustum_points_ws[i]);
            float   norm_squared    = dot(difference, difference);

            if (norm_squared < min_norm_squared)
            {
                min_norm_squared    = norm_squared;
                min_point           = frustum_points_ws[i];
            }
        }

        return min_point;
    }

    float Pi()
    {
        return 3.14159265358979323846f;
    }

    float radians(float degrees)
    {
        return ((degrees) / 180.0f) * Pi();
    }

}

namespace storage_factors
{
    float r_end_b_t(float theta)
    {
        return 1.0f / cos(theta);
    }

    float r_side_b_s(float theta)
    {
        return 1.0f / cos(theta);
    }

    float r_side_b_t(float theta, float n_e, float f_e)
    {
        return logf(f_e / n_e) / (2.0f * tanf(theta) * cosf(theta) * cosf(theta));
    }

    float uniform(float theta, float n_e, float f_e)
    {
        float ratio  = f_e / n_e;
        float cos    = cosf(theta);
        return ratio * ((ratio - 1) / (2 * tanf(theta) * cos * cos * cos));
    }

    float perspective(float theta, float n_e, float f_e)
    {
        float ratio = f_e / n_e;
        float cos = cosf(theta);
        return ((ratio - 1) / (2 * tanf(theta) * cos * cos * cos));
    }


    
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
    

    state.DepthStencilState.DepthEnable     = TRUE;
    state.DepthStencilState.StencilEnable   = FALSE;
    state.DepthStencilState.DepthWriteMask  = D3D12_DEPTH_WRITE_MASK_ALL;
    state.DepthStencilState.DepthFunc       = D3D12_COMPARISON_FUNC_GREATER_EQUAL;

    state.VS = { &g_triangle_vertex[0], sizeof(g_triangle_vertex) };
    state.PS = { &g_triangle_pixel[0], sizeof(g_triangle_pixel) };

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
        float theta                 = 0.785398185f;
        float n_e                   = 0.25f;
        float f_e                   = 32000.f;

        float r_end_t               = storage_factors::r_end_b_t(theta);
        float r_end_s               = storage_factors::r_end_b_t(theta);

        float r_side_s              = storage_factors::r_side_b_s(theta);
        float r_side_t              = storage_factors::r_side_b_t(theta, n_e, f_e);

        float uniform               = storage_factors::uniform(theta, n_e, f_e);
        float storage               = sqrtf(uniform);

        float perspective           = storage_factors::perspective(theta, n_e, f_e);
        float storage_p             = sqrtf(perspective);

        float n_opt                 = n_e + sqrtf(n_e * f_e);


        using namespace lispsm;

        vector3 up      = unit_y();
        vector3 forward = unit_z();
        vector3 right   = cross(up, forward);

        camera c;

        c.m_up        = up;
        c.m_direction = forward;
        c.m_position = { 0.0,1200.0,10.0f };
        c.m_near      = { 0.25f };
        c.m_far       = { 32000.f };

        view_transform view_ = make_view_transform(c);

        point3              p(0.0f, 0.0f, 16440.0f);

        point4          t0          = transform_point(view_, point4( p.m_value ));


        DirectX::XMMATRIX m0        = DirectX::XMMatrixPerspectiveFovLH(radians(75.0f), 1200.0f/900.0f , 1.0f, 64000.f );
        DirectX::XMVECTOR v0        = DirectX::XMVector3Transform({ -16000, -16000, 96000 }, m0);

        

        m_activated					= v.Activated(winrt::auto_revoke, { this, &ViewProvider::OnActivated });
        m_debug						= CreateDebug();
        m_device					= CreateDevice();

        m_queue					    = CreateCommandQueue(m_device.get());

        m_descriptorHeap		    = CreateDescriptorHeap(m_device.get());
        m_descriptorHeapDepth       = CreateDescriptorHeapDepth(m_device.get());
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
            D3D12_CPU_DESCRIPTOR_HANDLE back_buffer  = CpuView(m_device.get(), m_descriptorHeap.get()) + m_swap_chain_descriptors[m_frame_index];
            D3D12_CPU_DESCRIPTOR_HANDLE depth_buffer = CpuView(m_device.get(), m_descriptorHeapDepth.get()) + 0;

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

            D3D12_CPU_DESCRIPTOR_HANDLE debug_buffer_1 = CpuView(m_device.get(), m_descriptorHeap.get()) + m_debug_buffer1_descriptor;

            //Mark the resources in the rasterizer output
            {
                commandList->OMSetRenderTargets(1, &debug_buffer_1, TRUE, &depth_buffer);
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


        m_debug_buffer1 = CreateDebugBuffer1(m_device.get(), m_back_buffer_width, m_back_buffer_height);
        //create render target views, that will be used for rendering
        CreateDebugBuffer1Descriptior(m_device.get(), m_debug_buffer1.get(), CpuView(m_device.get(), m_descriptorHeap.get()) + 2);
        //Where are located the descriptors
        m_debug_buffer1_descriptor = 2;

        m_depth_buffer = CreateDepthResource(m_device.get(), m_back_buffer_width, m_back_buffer_height);
        m_depth_buffer->SetName(L"Depth Buffer");

        CreateDepthWriteDescriptor(m_device.get(), m_depth_buffer.get(), CpuView(m_device.get(), m_descriptorHeapDepth.get()) + 0);
        CreateDepthReadDescriptor(m_device.get(), m_depth_buffer.get(), CpuView(m_device.get(), m_descriptorHeapDepth.get()) + 1);

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

        m_debug_buffer1 = CreateDebugBuffer1(m_device.get(), m_back_buffer_width, m_back_buffer_height);
        //create render target views, that will be used for rendering
        CreateDebugBuffer1Descriptior(m_device.get(), m_debug_buffer1.get(), CpuView(m_device.get(), m_descriptorHeap.get()) + 2);
        //Where are located the descriptors
        m_debug_buffer1_descriptor = 2;

        m_depth_buffer = CreateDepthResource(m_device.get(), m_back_buffer_width, m_back_buffer_height);
        m_depth_buffer->SetName(L"Depth Buffer");

        CreateDepthWriteDescriptor(m_device.get(), m_depth_buffer.get(), CpuView(m_device.get(), m_descriptorHeapDepth.get()) + 0);
        CreateDepthReadDescriptor(m_device.get(), m_depth_buffer.get(), CpuView(m_device.get(), m_descriptorHeapDepth.get()) + 1);

        m_depth_descriptor[0] = 0;
        m_depth_descriptor[1] = 1;
    }

    bool m_window_running = true;

    CoreWindow::Closed_revoker					m_closed;
    CoreWindow::SizeChanged_revoker				m_size_changed;
    CoreApplicationView::Activated_revoker		m_activated;
    
    winrt::com_ptr <ID3D12Debug>                m_debug;
    winrt::com_ptr <ID3D12Device1>				m_device;                       //device for gpu resources
    winrt::com_ptr <IDXGISwapChain3>			m_swap_chain;                   //swap chain for 

    winrt::com_ptr <ID3D12Fence>        		m_fence;                        //fence for cpu/gpu synchronization
    winrt::com_ptr <ID3D12CommandQueue>   		m_queue;                        //queue to the device

    winrt::com_ptr <ID3D12DescriptorHeap>   	m_descriptorHeap;               //descriptor heap for the resources
    winrt::com_ptr <ID3D12DescriptorHeap>       m_descriptorHeapDepth;          //descriptor heap for the resources
    winrt::com_ptr <ID3D12DescriptorHeap>   	m_descriptorHeapRendering;      //descriptor heap for the resources

    std::mutex                                  m_blockRendering;               //block render thread for the swap chain resizes

    winrt::com_ptr<ID3D12Resource1>             m_swap_chain_buffers[2];
    uint64_t                                    m_swap_chain_descriptors[2];

    winrt::com_ptr<ID3D12Resource1>             m_debug_buffer1;
    uint64_t                                    m_debug_buffer1_descriptor;

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
};

int32_t __stdcall wWinMain( HINSTANCE, HINSTANCE,PWSTR, int32_t )
{
    ThrowIfFailed(CoInitializeEx(nullptr, COINIT_MULTITHREADED));
    CoreApplication::Run(ViewProvider());
    CoUninitialize();
    return 0;
}
