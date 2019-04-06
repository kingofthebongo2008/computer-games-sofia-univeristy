//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved

#pragma once

namespace TiledResources
{
    // Controls all the DirectX device resources.
    class DeviceResources
    {
    public:
        DeviceResources();
        void SetWindow(HWND window);
        void UpdateForWindowSizeChange();
        void Present();

        // Device Accessors.
        RECT                    GetWindowBounds() const                 { return m_windowBounds; }

        // D3D Accessors.
        ID3D11Device2*          GetD3DDevice() const                    { return m_d3dDevice.Get(); }
        ID3D11DeviceContext2*   GetD3DDeviceContext() const             { return m_d3dContext.Get(); }
        IDXGISwapChain1*        GetSwapChain() const                    { return m_swapChain.Get(); }
        D3D_FEATURE_LEVEL       GetDeviceFeatureLevel() const           { return m_d3dFeatureLevel; }
        ID3D11RenderTargetView* GetBackBufferRenderTargetView() const   { return m_d3dRenderTargetView.Get(); }
        ID3D11DepthStencilView* GetDepthStencilView() const             { return m_d3dDepthStencilView.Get(); }
        D3D11_VIEWPORT          GetScreenViewport() const               { return m_screenViewport; }

        // D2D Accessors.
        ID2D1Factory2*          GetD2DFactory() const                   { return m_d2dFactory.Get(); }
        ID2D1Device1*           GetD2DDevice() const                    { return m_d2dDevice.Get(); }
        ID2D1DeviceContext1*    GetD2DDeviceContext() const             { return m_d2dContext.Get(); }
        ID2D1Bitmap1*           GetD2DTargetBitmap() const              { return m_d2dTargetBitmap.Get(); }
        IDWriteFactory2*        GetDWriteFactory() const                { return m_dwriteFactory.Get(); }
        IWICImagingFactory2*    GetWicImagingFactory() const            { return m_wicFactory.Get(); }

        // Sample-specific Accessors.
        D3D11_TILED_RESOURCES_TIER GetTiledResourcesTier() const        { return m_tiledResourcesTier; }

    private:
        void CreateDeviceIndependentResources();
        void CreateDeviceResources();
        void CreateWindowSizeDependentResources();

        DXGI_MODE_ROTATION ComputeDisplayRotation();

        // Direct3D objects.
        Microsoft::WRL::ComPtr<ID3D11Device2>        m_d3dDevice;
        Microsoft::WRL::ComPtr<ID3D11DeviceContext2> m_d3dContext;
        Microsoft::WRL::ComPtr<IDXGISwapChain1>      m_swapChain;
        Microsoft::WRL::ComPtr<IDXGIFactory2> m_dxgiFactory;

        // Direct3D rendering objects. Required for 3D.
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_d3dRenderTargetView;
        Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_d3dDepthStencilView;
        D3D11_VIEWPORT                                 m_screenViewport;

        // Direct2D drawing components.
        Microsoft::WRL::ComPtr<ID2D1Factory2>       m_d2dFactory;
        Microsoft::WRL::ComPtr<ID2D1Device1>        m_d2dDevice;
        Microsoft::WRL::ComPtr<ID2D1DeviceContext1> m_d2dContext;
        Microsoft::WRL::ComPtr<ID2D1Bitmap1>        m_d2dTargetBitmap;

        // DirectWrite drawing components.
        Microsoft::WRL::ComPtr<IDWriteFactory2>     m_dwriteFactory;
        Microsoft::WRL::ComPtr<IWICImagingFactory2> m_wicFactory;

        // Cached reference to the Window.
        HWND m_window;

        // Cached device properties.
        D3D_FEATURE_LEVEL m_d3dFeatureLevel;
        SIZE              m_d3dRenderTargetSize;
        RECT              m_windowBounds;

        // Tiled Resources Tier.
        D3D11_TILED_RESOURCES_TIER m_tiledResourcesTier;
    };
}
