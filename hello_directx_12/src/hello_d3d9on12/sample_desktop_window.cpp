//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved

#include "pch.h"
#include "sample_desktop_window.h"
#include "build_window_environment.h"

#include "d3dx12.h"

namespace
{
    //Helper class that assists us using the descriptors
    struct DescriptorHeapCpuView
    {
        DescriptorHeapCpuView(D3D12_CPU_DESCRIPTOR_HANDLE  base, uint64_t offset) : m_base(base), m_offset(offset)
        {

        }

        D3D12_CPU_DESCRIPTOR_HANDLE operator () (size_t index) const
        {
            return { m_base.ptr + index * m_offset };
        }

        D3D12_CPU_DESCRIPTOR_HANDLE operator + (size_t index) const
        {
            return { m_base.ptr + index * m_offset };
        }

        D3D12_CPU_DESCRIPTOR_HANDLE m_base = {};
        uint64_t                    m_offset;
    };

    DescriptorHeapCpuView CpuView(ID3D12Device* d, ID3D12DescriptorHeap* heap)
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
        if (D3D12GetDebugInterface(IID_PPV_ARGS(r.GetAddressOf())) == S_OK)
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


    static Microsoft::WRL::ComPtr<ID3D12CommandQueue> CreateCommandQueue(ID3D12Device* d)
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


        desc.BufferCount = 2;
        desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        desc.Width = static_cast<UINT>(e.m_back_buffer_size.Width);
        desc.Height = static_cast<UINT>(e.m_back_buffer_size.Height);
        desc.SampleDesc.Count = 1;
        desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
        desc.Scaling = DXGI_SCALING_NONE;

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
        d.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
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
    static D3D12_RESOURCE_DESC DescribeSwapChain(uint32_t width, uint32_t height)
    {
        D3D12_RESOURCE_DESC d = {};
        d.Alignment = 0;
        d.DepthOrArraySize = 1;
        d.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        d.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        d.Format = DXGI_FORMAT_B8G8R8A8_TYPELESS;     //important for computing the resource footprint
        d.Height = height;
        d.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        d.MipLevels = 1;
        d.SampleDesc.Count = 1;
        d.SampleDesc.Quality = 0;
        d.Width = width;
        return                  d;
    }

    static Microsoft::WRL::ComPtr<ID3D12Resource1> CreateSwapChainResource1(ID3D12Device1* device, uint32_t width, uint32_t height)
    {
        D3D12_RESOURCE_DESC d = DescribeSwapChain(width, height);

        Microsoft::WRL::ComPtr<ID3D12Resource1>     r;
        D3D12_HEAP_PROPERTIES p = {};
        p.Type = D3D12_HEAP_TYPE_DEFAULT;
        D3D12_RESOURCE_STATES       state = D3D12_RESOURCE_STATE_PRESENT;

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
    static void CreateSwapChainDescriptor(ID3D12Device1* device, ID3D12Resource1* resource, D3D12_CPU_DESCRIPTOR_HANDLE handle)
    {
        D3D12_RENDER_TARGET_VIEW_DESC d = {};
        d.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        d.Format = DXGI_FORMAT_B8G8R8A8_UNORM;       //how we will view the resource during rendering
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
        ThrowIfFailed(device->CreateRootSignature(0, &g_default_graphics_signature[0], sizeof(g_default_graphics_signature), IID_PPV_ARGS(r.GetAddressOf())));
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
}

CSampleDesktopWindow::CSampleDesktopWindow()
{
    // Set member variables to zero or NULL defaults.
    m_visible = FALSE; 
    m_occlusion = DWORD(0.0);
}

CSampleDesktopWindow::~CSampleDesktopWindow()
{
    /*
    if (m_deviceResources)
    {
        m_deviceResources->SetWindow(nullptr, 96.0F);
        m_deviceResources.reset();
    }
    */
}

// <summary>
// These functions are used to initialize and configure the main
// application window and message pumps.
// </summary>
HRESULT
CSampleDesktopWindow::Initialize(
    _In_    RECT bounds,
    _In_    std::wstring title
    )
{
    // Create device resources required to render content.
    //m_deviceResources = std::make_shared<DeviceResources>();
    //if (!m_deviceResources)
    {
      //  return HRESULT_FROM_WIN32(GetLastError());
    }

    CreateDeviceIndependentResources();
    CreateDeviceResources();

    // Create main application window.
    m_hWnd = __super::Create(nullptr, bounds, title.c_str());

    if (!m_hWnd)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    // Initialize member variables with default values.
    m_visible = TRUE; 
    m_occlusion = DWORD(0.0);

    // Enable mouse to act as pointing device for this application.
    if(!EnableMouseInPointer(TRUE))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return S_OK;
}

// <summary>
// This method checks the current DPI against what the application has stored.
// If the DPI has changed, update DPI for D2D resources.
// </summary>
void
CSampleDesktopWindow::SetNewDpi(_In_ float newPerMonitorDpi)
{
    m_window_environment = sample::build_environment(m_hWnd);

    /*
    if (m_deviceResources && m_deviceResources->GetDpi() != newPerMonitorDpi)
    {
        m_deviceResources->SetDpi(newPerMonitorDpi);
    }
    */
}

// Main message loop for application.
HRESULT
CSampleDesktopWindow::Run()
{
    HRESULT hr = S_OK;

    MSG message = { };
    do
    {
        if (m_visible)
        {
            hr = Render();
        }
        else
        {
            WaitMessage();
        }

        while (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE) && (message.message != WM_QUIT))
        {
			TranslateMessage(&message);
            
            DispatchMessage(&message);
        }
    } while (message.message != WM_QUIT);

    return hr;
}

// <summary>
// This method is called in response to message handlers and
// as part of the main message loop.
// </summary>
HRESULT
CSampleDesktopWindow::Render()
{
    HRESULT hr = S_OK;
    //auto d2dContext = m_deviceResources->GetD2DDeviceContext();

    //d2dContext->BeginDraw();

    // Draw window background.
    //d2dContext->Clear(D2D1::ColorF(0.8764F, 0.8764F, 0.8882F));

    // Draw client area as implemented by any derived classes.
    Draw();

    //hr = d2dContext->EndDraw();

    if (FAILED(hr))
    {
        return hr;
    }

    if ( false ) //!m_deviceResources->Present())
    {
        hr = S_OK;// m_deviceResources->GetDxgiFactory()->RegisterOcclusionStatusWindow(m_hWnd, WM_USER, &m_occlusion);
        if (FAILED(hr))
        {
            return hr;
        }
        else
        {
            m_visible = false;
        }
    }

    return hr;
}

// <summary>
// These functions will be called as messages are processed by message map
// defined in the Desktop Window class.
// </summary>
LRESULT
CSampleDesktopWindow::OnCreate(
    _In_ UINT,
    _In_ WPARAM,
    _In_ LPARAM lParam,
    _Out_ BOOL &bHandled
    )
{
    auto cs = reinterpret_cast<CREATESTRUCT *>(lParam);

    m_window_environment = sample::build_environment(m_hWnd);

    // Store a reference to the hWnd so DirectX can render to this surface.
    //m_deviceResources->SetWindow(m_hWnd, windowDpi);

    // Set styles needed to avoid drawing over any child or sibling windows.
    cs->style |= ( WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS );

    // Set styles required to avoid overdraw. 
    cs->dwExStyle |= ( WS_EX_LAYERED | WS_EX_NOREDIRECTIONBITMAP );

    // Apply selected styles.
    SetWindowLong(GWL_STYLE, cs->style);
    SetWindowLong(GWL_EXSTYLE, cs->dwExStyle);


    m_swap_chain = CreateSwapChain(w, m_queue.get());
    m_frame_index = m_swap_chain->GetCurrentBackBufferIndex();


    //Now recreate the swap chain with the new dimensions, we must have back buffer as the window size
    m_back_buffer_width = static_cast<UINT>(e.m_back_buffer_size.Width);
    m_back_buffer_height = static_cast<UINT>(e.m_back_buffer_size.Height);

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

    bHandled = TRUE;
    return 0;
}

//
// Destroying this window will also quit the application. 
//
LRESULT
CSampleDesktopWindow::OnDestroy(
    _In_ UINT,
    _In_ WPARAM,
    _In_ LPARAM,
    _Out_ BOOL &bHandled
    )
{
    m_visible = false;

    PostQuitMessage(0);

    bHandled = TRUE;
    return 0;
}

LRESULT
CSampleDesktopWindow::OnPaint(
    _In_ UINT,
    _In_ WPARAM,
    _In_ LPARAM,
    _Out_ BOOL &bHandled
    )
{
    HDC hDC;
    PAINTSTRUCT ps;

    hDC = BeginPaint(&ps);
    if (hDC)
    {
        Render();

        EndPaint(&ps);

        bHandled = TRUE;
        return 0;
    }
    else
    {
        bHandled = FALSE;
    }

    return 0;
}

LRESULT
CSampleDesktopWindow::OnWindowPosChanged(
    _In_ UINT,
    _In_ WPARAM,
    _In_ LPARAM lparam,
    _Out_ BOOL &bHandled
    )
{
    RECT clientRect;
    auto windowPos = reinterpret_cast<WINDOWPOS *>(lparam);
    GetClientRect(&clientRect);
    if (!(windowPos->flags & SWP_NOSIZE))
    {
        //DeviceResources::Size size;
        //size.Width = static_cast<float>(clientRect.right - clientRect.left)  / (m_window_environment.m_dpi / 96.0F);
        //size.Height = static_cast<float>(clientRect.bottom - clientRect.top) / (m_window_environment.m_dpi / 96.0F);
        //m_deviceResources->SetLogicalSize(size);
        Render();
    }

    bHandled = TRUE;
    return 0;
}

LRESULT
CSampleDesktopWindow::OnDisplayChange(
    _In_ UINT,
    _In_ WPARAM,
    _In_ LPARAM,
    _Out_ BOOL &bHandled
    )
{
    Render();
    OnDisplayChange();
    bHandled = TRUE;
    return 0;
}

LRESULT
CSampleDesktopWindow::OnGetMinMaxInfo(
    _In_ UINT,
    _In_ WPARAM,
    _In_ LPARAM lparam,
    _Out_ BOOL &bHandled
    )
{
    auto minMaxInfo = reinterpret_cast<MINMAXINFO *>(lparam);

    minMaxInfo->ptMinTrackSize.y = 200;

    bHandled = TRUE;
    return 0;
}

LRESULT
CSampleDesktopWindow::OnActivate(
    _In_ UINT, 
    _In_ WPARAM wparam, 
    _In_ LPARAM, 
    _Out_ BOOL &bHandled
    )
{
    m_visible = !HIWORD(wparam);

    bHandled = TRUE;
    return 0;
}

LRESULT
CSampleDesktopWindow::OnPowerBroadcast(
    _In_ UINT,
    _In_ WPARAM,
    _In_ LPARAM lparam,
    _Out_ BOOL &bHandled
    )
{
    if (lparam > 0)
    {
        auto const ps = reinterpret_cast<POWERBROADCAST_SETTING *>(lparam);
        m_visible = 0 != *reinterpret_cast<DWORD const *>(ps->Data);
    }

    if (m_visible)
    {
        PostMessage(WM_NULL);
    }

    bHandled = TRUE;
    return TRUE;
}

LRESULT
CSampleDesktopWindow::OnOcclusion(
    _In_ UINT,
    _In_ WPARAM,
    _In_ LPARAM,
    _Out_ BOOL &bHandled
    )
{
    //ASSERT(m_occlusion);

    //if (S_OK == m_deviceResources->GetSwapChain()->Present(0, DXGI_PRESENT_TEST))
    {
      //  m_deviceResources->GetDxgiFactory()->UnregisterOcclusionStatus(m_occlusion);
       // m_occlusion = 0;
       // m_visible = true;
    }

    bHandled = TRUE;
    return 0;
}

LRESULT
CSampleDesktopWindow::OnPointerDown(
    _In_ UINT,
    _In_ WPARAM,
    _In_ LPARAM lparam,
    _Out_ BOOL &bHandled
    )
{
    auto x = GET_X_LPARAM(lparam);
    auto y = GET_Y_LPARAM(lparam);

    POINT pt;
    pt.x = x;
    pt.y = y;

    ScreenToClient(&pt);

    auto localx = static_cast<float>(pt.x) / (m_window_environment.m_dpi / 96.0F);
    auto localy = static_cast<float>(pt.y) / (m_window_environment.m_dpi / 96.0F);

    // Call handler implemented by derived class for WM_POINTERDOWN message.
    OnPointerDown(localx, localy);

    bHandled = TRUE;
    return 0;
}

LRESULT
CSampleDesktopWindow::OnPointerUp(
    _In_ UINT,
    _In_ WPARAM,
    _In_ LPARAM lparam,
    _Out_ BOOL &bHandled
    )
{
    auto x = GET_X_LPARAM(lparam);
    auto y = GET_Y_LPARAM(lparam);

    POINT pt;
    pt.x = x;
    pt.y = y;

    ScreenToClient(&pt);

    auto localX = static_cast<float>(pt.x) / (m_window_environment.m_dpi / 96.0F);
    auto localY = static_cast<float>(pt.y) / (m_window_environment.m_dpi / 96.0F);


    // Call handler implemented by derived class for WM_POINTERUP message.
    OnPointerUp(localX, localY);

    bHandled = TRUE;
    return 0;
}

LRESULT
CSampleDesktopWindow::OnEnterSizeMove(
    _In_ UINT,
    _In_ WPARAM,
    _In_ LPARAM,
    _Out_ BOOL &bHandled
    )
{
    // Call handler implemented by derived class for WM_ENTERSIZEMOVE message.
    OnEnterSizeMove();

    bHandled = TRUE;
    return 0;
}

LRESULT
CSampleDesktopWindow::OnExitSizeMove(
    _In_ UINT,
    _In_ WPARAM,
    _In_ LPARAM,
    _Out_ BOOL &bHandled
    )
{
    // Call handler implemented by derived class for WM_EXITSIZEMOVE message.
    OnExitSizeMove();

    bHandled = TRUE;
    return 0;
}

LRESULT
CSampleDesktopWindow::OnDpiChange(
    _In_ UINT,
    _In_ WPARAM wparam,
    _In_ LPARAM lparam,
    _Out_ BOOL &bHandled
    )
{
    auto lprcNewScale = reinterpret_cast<LPRECT>(lparam);

    // Call handler implemented by derived class for WM_DPICHANGED message.
    OnDpiChange(LOWORD(wparam), lprcNewScale);

    bHandled = TRUE;
    return 0;
}

LRESULT
 CSampleDesktopWindow::OnPointerUpdate(
    _In_ UINT,
    _In_ WPARAM,
    _In_ LPARAM lparam,
    _Out_ BOOL &bHandled
    )
{
    auto x = GET_X_LPARAM(lparam);
    auto y = GET_Y_LPARAM(lparam);

    POINT pt;
    pt.x = x;
    pt.y = y;

    ScreenToClient(&pt);

    auto localx = static_cast<float>(pt.x) / (m_window_environment.m_dpi / 96.0F);
    auto localy = static_cast<float>(pt.y) / (m_window_environment.m_dpi / 96.0F);

    // Call handler implemented by derived class for WM_POINTERUPDATE message.
    OnPointerUpdate(localx, localy);

    bHandled = TRUE;
    return 0;
}
