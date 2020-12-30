//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved

#pragma once

#include "window_environment.h"


// <summary>
// This class can be used on its own or extended to provide custom functionality.
// This class implements a map to respond to messages relevant to DPI events.
// </summary>
class CSampleDesktopWindow : public CWindowImpl<CSampleDesktopWindow, CWindow, CWinTraits < WS_OVERLAPPED | WS_VISIBLE | WS_SYSMENU | WS_SIZEBOX > >
{
public:
    BEGIN_MSG_MAP(c)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_PAINT, OnPaint)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_WINDOWPOSCHANGED, OnWindowPosChanged)
        MESSAGE_HANDLER(WM_DISPLAYCHANGE, OnDisplayChange)
        MESSAGE_HANDLER(WM_GETMINMAXINFO, OnGetMinMaxInfo)
        MESSAGE_HANDLER(WM_ACTIVATE, OnActivate)
        MESSAGE_HANDLER(WM_POWERBROADCAST, OnPowerBroadcast)
        MESSAGE_HANDLER(WM_USER, OnOcclusion)
        MESSAGE_HANDLER(WM_POINTERDOWN, OnPointerDown)
        MESSAGE_HANDLER(WM_POINTERUP, OnPointerUp)
        MESSAGE_HANDLER(WM_POINTERUPDATE, OnPointerUpdate)
        MESSAGE_HANDLER(WM_ENTERSIZEMOVE, OnEnterSizeMove)
        MESSAGE_HANDLER(WM_EXITSIZEMOVE, OnExitSizeMove)
        MESSAGE_HANDLER(WM_DPICHANGED, OnDpiChange)
    END_MSG_MAP()

    DECLARE_WND_CLASS_EX(nullptr, 0, -1);

    CSampleDesktopWindow();
    ~CSampleDesktopWindow();

    // <summary>
    // This method provides the main message pump for the application.
    // </summary>
    HRESULT Run();


    // <summary>
    // This method is used to create and initialize this class
    // for use by the application.
    // </summary>
    HRESULT Initialize(_In_ RECT bounds, _In_ std::wstring title);

protected:

    // Store DPI information for use by this class.
    void SetNewDpi(_In_ float newPerMonitorDpi);

    // Cached pointer to device resources.
    //std::shared_ptr<DeviceResources> m_deviceResources;

    // Store application and display state to optimize rendering and present instructions.
    bool m_visible;
    DWORD m_occlusion;

    sample::window_environment m_window_environment;

private:
    // Draw client area of base Desktop Window.
    HRESULT Render();

    // <summary>
    // These methods will be called as messages are processed by message map
    // defined in this class.
    // </summary>
    LRESULT OnCreate(_In_ UINT, _In_ WPARAM, _In_ LPARAM lParam, _Out_ BOOL &bHandled);
    LRESULT OnDestroy(_In_ UINT, _In_ WPARAM, _In_ LPARAM, _Out_ BOOL &bHandled);
    LRESULT OnPaint(_In_ UINT, _In_ WPARAM, _In_ LPARAM, _Out_ BOOL &bHandled);

    LRESULT OnWindowPosChanged(_In_ UINT, _In_ WPARAM, _In_ LPARAM lparam, _Out_ BOOL &bHandled);

    LRESULT OnDisplayChange(_In_ UINT, _In_ WPARAM, _In_ LPARAM, _Out_ BOOL &bHandled);
    LRESULT OnGetMinMaxInfo(_In_ UINT, _In_ WPARAM, _In_ LPARAM lparam, _Out_ BOOL &bHandled);

    LRESULT OnActivate(_In_ UINT, _In_ WPARAM wparam, _In_ LPARAM, _Out_ BOOL &bHandled);
    LRESULT OnPowerBroadcast(_In_ UINT, _In_ WPARAM, _In_ LPARAM lparam, _Out_ BOOL &bHandled);
    LRESULT OnOcclusion(_In_ UINT, _In_ WPARAM, _In_ LPARAM, _Out_ BOOL &bHandled);

    LRESULT OnPointerDown(_In_ UINT, _In_ WPARAM, _In_ LPARAM lparam, _Out_ BOOL &bHandled);
    LRESULT OnPointerUp(_In_ UINT, _In_ WPARAM, _In_ LPARAM lparam, _Out_ BOOL &bHandled);
    LRESULT OnPointerUpdate(_In_ UINT, _In_ WPARAM, _In_ LPARAM lparam, _Out_ BOOL &bHandled);

    LRESULT OnEnterSizeMove(_In_ UINT, _In_ WPARAM, _In_ LPARAM, _Out_ BOOL &bHandled);
    LRESULT OnExitSizeMove(_In_ UINT, _In_ WPARAM, _In_ LPARAM, _Out_ BOOL &bHandled);

    LRESULT OnDpiChange(_In_ UINT, _In_ WPARAM, _In_ LPARAM lparam, _Out_ BOOL &bHandled);

    // <summary>
    // Allocate and release device resources required for Dx operations.
    // These are implemented by any classes deriving from SampleDesktopWindow.
    // </summary>
    virtual void CreateDeviceIndependentResources() {};
    virtual void ReleaseDeviceIndependentResources() {};

    virtual void CreateDeviceResources() {};
    virtual void ReleaseDeviceResources() {};

    // <summary>
    // Method to be implemented by any classes inheriting from SampleDesktopWindow.
    //    SampleDesktopWindow will call this once default rendering functionality
    //    is complete.
    // </summary>
    virtual void Draw() {};

// <summary>
// Extend default message handlers provided by DesktopWindow class.
// Note: These do not overwrite handlers provided by this class,
// but are called by default message handlers provided.
// </summary>
    virtual void OnPointerUp(_In_ float /* x */, _In_ float /* y */) {};
    virtual void OnPointerDown(_In_ float /* x */, _In_ float /* y */) {};
    virtual void OnPointerUpdate(_In_ float /* x */, _In_ float /* y */) {};
    virtual void OnEnterSizeMove() {};
    virtual void OnExitSizeMove() {};
    virtual void OnDpiChange(_In_ int /* dpi */, _In_ LPRECT /* rect */) {};
    virtual void OnDisplayChange() {};


    Microsoft::WRL::ComPtr <ID3D12Debug>                m_debug;
    Microsoft::WRL::ComPtr <ID3D12Device1>				m_device;           //device for gpu resources
    Microsoft::WRL::ComPtr<IDXGIFactory2>               m_dxgi_factory;
    Microsoft::WRL::ComPtr <IDXGISwapChain3>			m_swap_chain;       //swap chain for 

    Microsoft::WRL::ComPtr <ID3D12Fence>        		m_fence;                     //fence for cpu/gpu synchronization
    Microsoft::WRL::ComPtr <ID3D12CommandQueue>   		m_queue;                     //queue to the device

    Microsoft::WRL::ComPtr <ID3D12DescriptorHeap>   	m_descriptorHeap;            //descriptor heap for the resources

    Microsoft::WRL::ComPtr <ID3D12DescriptorHeap>   	m_descriptorHeapRendering;   //descriptor heap for the resources

    std::mutex                                          m_blockRendering;   //block render thread for the swap chain resizes

    Microsoft::WRL::ComPtr<ID3D12Resource1>             m_swap_chain_buffers[2];
    uint64_t                                            m_swap_chain_descriptors[2];

    uint32_t									        m_back_buffer_width = 0;
    uint32_t									        m_back_buffer_height = 0;

    Microsoft::WRL::ComPtr <ID3D12CommandAllocator>   	m_command_allocator[2];		//one per frame
    Microsoft::WRL::ComPtr <ID3D12GraphicsCommandList1> m_command_list[2];			//one per frame

    uint64_t                                            m_frame_index = 0;
    uint64_t									        m_fence_value = 1;
    HANDLE										        m_fence_event = {};

    //Rendering
    Microsoft::WRL::ComPtr< ID3D12RootSignature>		m_root_signature;
    Microsoft::WRL::ComPtr< ID3D12PipelineState>		m_triangle_state;
};