//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved

#include "pch.h"
#include "DeviceResources.h"

#include "DirectXHelper.h"                   // For ThrowIfFailed
#include <windows.ui.xaml.media.dxinterop.h> // For SwapChainBackgroundPanel native methods

using namespace TiledResources;

using namespace D2D1;
using namespace DirectX;
using namespace Microsoft::WRL;

using namespace std;

// Constructor for DeviceResources.
DeviceResources::DeviceResources() :
    m_screenViewport(),
    m_d3dFeatureLevel(D3D_FEATURE_LEVEL_9_1),
    m_d3dRenderTargetSize(),
    m_windowBounds(),
    m_tiledResourcesTier(D3D11_TILED_RESOURCES_NOT_SUPPORTED)
{
    CreateDeviceIndependentResources();
    CreateDeviceResources();
}

// Configures resources that don't depend on the Direct3D device.
void DeviceResources::CreateDeviceIndependentResources()
{
    // Initialize Direct2D resources
    D2D1_FACTORY_OPTIONS options;
    ZeroMemory(&options, sizeof(D2D1_FACTORY_OPTIONS));

#if defined(_DEBUG)
    // If the project is in a debug build, enable Direct2D debugging via SDK Layers.
    options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif

    // Initialize the Direct2D Factory
    DX::ThrowIfFailed(
        D2D1CreateFactory(
        D2D1_FACTORY_TYPE_SINGLE_THREADED,
        __uuidof(ID2D1Factory2),
        &options,
        &m_d2dFactory
        )
        );

    // Initialize the DirectWrite Factory
    DX::ThrowIfFailed(
        DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory2),
        &m_dwriteFactory
        )
        );

    // Initialize the Windows Imaging Component (WIC) Factory
    DX::ThrowIfFailed(
        CoCreateInstance(
        CLSID_WICImagingFactory2,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&m_wicFactory)
        )
        );
}

// Configures the Direct3D device, and stores handles to it and the device context.
void DeviceResources::CreateDeviceResources()
{

    HR hr = S_OK;

    hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&m_dxgiFactory));

    wcout << L"Finding an adapter that supports tiled resources..." << endl;
    UINT i = 0;
    while (true)
    {
        //if (i == 0){ i++; continue; } // reenable this (assuming the hw adapter is adapter 0) to make it run on warp
        ComPtr<IDXGIAdapter1> adapter;
        HRESULT ehr = m_dxgiFactory->EnumAdapters1(i, &adapter);
        if (ehr == DXGI_ERROR_NOT_FOUND)
        {
            wcout << L"couldn't find an adapter supporting tiled resources, and WARP didn't work for some reason" << endl;
            hr = E_UNEXPECTED;
        }
        else
        {
            hr = ehr;
            DXGI_ADAPTER_DESC1 desc = { 0 };
            hr = adapter->GetDesc1(&desc);
            wcout << i << L": " << desc.Description << L" - ";
            UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
            flags |= D3D11_CREATE_DEVICE_DEBUG; // for now, disable debug spew (currently incorrect regarding barrier warning)
#endif
            D3D_FEATURE_LEVEL featureLevels [] =
            {
                D3D_FEATURE_LEVEL_11_1,
                D3D_FEATURE_LEVEL_11_0
            };
            ComPtr<ID3D11Device> dev;
            ComPtr<ID3D11DeviceContext> con;
            D3D_FEATURE_LEVEL featureLevel;
            HRESULT chr = D3D11CreateDevice(
                adapter.Get(),
                D3D_DRIVER_TYPE_UNKNOWN,
                NULL,
                flags,
                featureLevels,
                ARRAYSIZE(featureLevels),
                D3D11_SDK_VERSION,
                &dev,
                &featureLevel,
                &con
                );
            if (FAILED(chr))
            {
                wcout << L"couldn't create feature level 11+ device" << endl;
            }
            else
            {
                D3D11_FEATURE_DATA_D3D11_OPTIONS1 supportData;
                hr = dev->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS1, &supportData, sizeof(supportData));
                if (supportData.TiledResourcesTier == D3D11_TILED_RESOURCES_NOT_SUPPORTED)
                {
                    wcout << L"no support for tiled resources" << endl;
                }
                else
                {
                    if (supportData.TiledResourcesTier == D3D11_TILED_RESOURCES_TIER_1)
                    {
                        wcout << L"supports tier 1 - ";
                    }
                    else if (supportData.TiledResourcesTier == D3D11_TILED_RESOURCES_TIER_2)
                    {
                        wcout << L"supports tier 2 - ";
                    }
                    else
                    {
                        wcout << L"supports unknown tier - ";
                    }
                    hr = dev.As(&m_d3dDevice);
                    hr = con.As(&m_d3dContext);
                    m_tiledResourcesTier = supportData.TiledResourcesTier;
					m_tiledResourcesTier = D3D11_TILED_RESOURCES_TIER_1; // for now, force tier-1 behavior - still some issues in the sample code
                    wcout << L"using this adapter" << endl;
                    break;
                }
            }
        }
        i++;
    }

    // Create the Direct2D device object and a corresponding context.
    ComPtr<IDXGIDevice3> dxgiDevice;
    DX::ThrowIfFailed(
        m_d3dDevice.As(&dxgiDevice)
        );

    DX::ThrowIfFailed(
        m_d2dFactory->CreateDevice(dxgiDevice.Get(), &m_d2dDevice)
        );

    DX::ThrowIfFailed(
        m_d2dDevice->CreateDeviceContext(
        D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
        &m_d2dContext
        )
        );
}

// These resources need to be recreated every time the window size is changed.
void DeviceResources::CreateWindowSizeDependentResources()
{
    // Store the window bounds so the next time we get a SizeChanged event we can
    // avoid rebuilding everything if the size is identical.
    GetWindowRect(m_window, &m_windowBounds);

    // Calculate the necessary swap chain and render target size in pixels.
    float windowWidth = (float)(m_windowBounds.right - m_windowBounds.left);
    float windowHeight = (float)(m_windowBounds.bottom - m_windowBounds.top);

    // The width and height of the swap chain must be based on the window's
    // natively-oriented width and height. If the window is not in the native
    // orientation, the dimensions must be reversed.

    m_d3dRenderTargetSize.cx = (LONG)windowWidth;
    m_d3dRenderTargetSize.cy = (LONG)windowHeight;

    if (m_swapChain)
    {
        // If the swap chain already exists, resize it.
        HRESULT hr = m_swapChain->ResizeBuffers(
            2, // Double-buffered swap chain.
            static_cast<UINT>(m_d3dRenderTargetSize.cx),
            static_cast<UINT>(m_d3dRenderTargetSize.cy),
            DXGI_FORMAT_B8G8R8A8_UNORM,
            0
            );

        if (hr == DXGI_ERROR_DEVICE_REMOVED)
        {
            // If the device was removed for any reason, a new device and swap chain will need to be created.
            //HandleDeviceLost();

            // Everything is set up now. Do not continue execution of this method. 
            return;
        }
        else
        {
            DX::ThrowIfFailed(hr);
        }
    }
    else
    {
        // Otherwise, create a new one using the same adapter as the existing Direct3D device.
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {0};

        swapChainDesc.Width = static_cast<UINT>(m_d3dRenderTargetSize.cx); // Match the size of the window.
        swapChainDesc.Height = static_cast<UINT>(m_d3dRenderTargetSize.cy);
        swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // This is the most common swap chain format.
        swapChainDesc.Stereo = false;
        swapChainDesc.SampleDesc.Count = 1; // Don't use multi-sampling.
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = 2; // Use double-buffering to minimize latency.
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; // All Windows Store apps must use this SwapEffect.
        swapChainDesc.Flags = 0;

        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;

        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE; // When using XAML interop, this value cannot be DXGI_ALPHA_MODE_PREMULTIPLIED

        // This sequence obtains the DXGI factory that was used to create the Direct3D device above.
        ComPtr<IDXGIDevice3> dxgiDevice;
        DX::ThrowIfFailed(
            m_d3dDevice.As(&dxgiDevice)
            );

        DX::ThrowIfFailed(
            m_dxgiFactory->CreateSwapChainForHwnd(
                m_d3dDevice.Get(),
                m_window,
                &swapChainDesc,
                NULL,
                NULL,
                &m_swapChain
                )
            );

        // Ensure that DXGI does not queue more than one frame at a time. This both reduces latency and
        // ensures that the application will only render after each VSync, minimizing power consumption.
        DX::ThrowIfFailed(
            dxgiDevice->SetMaximumFrameLatency(1)
            );
    }

    // Create a render target view of the swap chain back buffer.
    ComPtr<ID3D11Texture2D> backBuffer;
    DX::ThrowIfFailed(
        m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer))
        );

    DX::ThrowIfFailed(
        m_d3dDevice->CreateRenderTargetView(
        backBuffer.Get(),
        nullptr,
        &m_d3dRenderTargetView
        )
        );

    // Create a depth stencil view for use with 3D rendering if needed.
    CD3D11_TEXTURE2D_DESC depthStencilDesc(
        DXGI_FORMAT_D24_UNORM_S8_UINT,
        static_cast<UINT>(m_d3dRenderTargetSize.cx),
        static_cast<UINT>(m_d3dRenderTargetSize.cy),
        1, // This depth stencil view has only one texture.
        1, // Use a single mipmap level.
        D3D11_BIND_DEPTH_STENCIL
        );

    ComPtr<ID3D11Texture2D> depthStencil;
    DX::ThrowIfFailed(
        m_d3dDevice->CreateTexture2D(
            &depthStencilDesc,
            nullptr,
            &depthStencil
            )
        );

    CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2D);
    DX::ThrowIfFailed(
        m_d3dDevice->CreateDepthStencilView(
        depthStencil.Get(),
        &depthStencilViewDesc,
        &m_d3dDepthStencilView
        )
        );

    // Set the 3D rendering viewport to target the entire window.
    m_screenViewport = CD3D11_VIEWPORT(
        0.0f,
        0.0f,
        (float)m_d3dRenderTargetSize.cx,
        (float)m_d3dRenderTargetSize.cy
        );

    m_d3dContext->RSSetViewports(1, &m_screenViewport);

    // Create a Direct2D target bitmap associated with the
    // swap chain back buffer and set it as the current target.
    D2D1_BITMAP_PROPERTIES1 bitmapProperties =
        D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
        96.0f,
        96.0f
        );

    ComPtr<IDXGISurface2> dxgiBackBuffer;
    DX::ThrowIfFailed(
        m_swapChain->GetBuffer(0, IID_PPV_ARGS(&dxgiBackBuffer))
        );

    DX::ThrowIfFailed(
        m_d2dContext->CreateBitmapFromDxgiSurface(
        dxgiBackBuffer.Get(),
        &bitmapProperties,
        &m_d2dTargetBitmap
        )
        );

    m_d2dContext->SetTarget(m_d2dTargetBitmap.Get());

    // Grayscale text anti-aliasing is recommended for all Windows Store apps.
    m_d2dContext->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
}

// This method is called when the CoreWindow is created (or re-created)
void DeviceResources::SetWindow(HWND window)
{
    m_window = window;

    // SetDpi() will call CreateWindowSizeDependentResources()
    // if those resources have not been created yet.
    m_d2dContext->SetDpi(96.0f, 96.0f);
    UpdateForWindowSizeChange();
}

// This method is called in the event handler for the SizeChanged event.
void DeviceResources::UpdateForWindowSizeChange()
{
    ID3D11RenderTargetView* nullViews[] = {nullptr};
    m_d3dContext->OMSetRenderTargets(ARRAYSIZE(nullViews), nullViews, nullptr);
    m_d3dRenderTargetView = nullptr;
    m_d2dContext->SetTarget(nullptr);
    m_d2dTargetBitmap = nullptr;
    m_d3dDepthStencilView = nullptr;
    m_d3dContext->Flush();
    CreateWindowSizeDependentResources();
}


// Present the contents of the swap chain to the screen.
void DeviceResources::Present()
{
    // The first argument instructs DXGI to block until VSync, putting the application
    // to sleep until the next VSync. This ensures we don't waste any cycles rendering
    // frames that will never be displayed to the screen.
    HRESULT hr = m_swapChain->Present(1, 0);

    // Discard the contents of the render target.
    // This is a valid operation only when the existing contents will be entirely
    // overwritten. If dirty or scroll rects are used, this call should be removed.
    m_d3dContext->DiscardView(m_d3dRenderTargetView.Get());

    // Discard the contents of the depth stencil.
    m_d3dContext->DiscardView(m_d3dDepthStencilView.Get());

    // If the device was removed either by a disconnect or a driver upgrade, we 
    // must recreate all device resources.
    if (hr == DXGI_ERROR_DEVICE_REMOVED)
    {
        DX::ThrowIfFailed(hr);
    }
    else
    {
        DX::ThrowIfFailed(hr);
    }
}
