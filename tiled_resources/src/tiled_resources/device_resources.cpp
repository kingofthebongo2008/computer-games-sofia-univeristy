#include "pch.h"
#include "device_resources.h"

#include <d3d12.h>
#include <dxgi1_5.h>

#include "error.h"
#include "cpu_view.h"

namespace sample
{
    namespace
    {
        //Debug layer, issues warnings if something broken. Use it when you develop stuff
        static winrt::com_ptr<ID3D12Debug1> CreateDebug()
        {
            winrt::com_ptr<ID3D12Debug1> r;
            //check if you have installed debug layer, from the option windows components
            if (D3D12GetDebugInterface(__uuidof(ID3D12Debug1), r.put_void()) == S_OK)
            {
                r->EnableDebugLayer();
				r->SetEnableGPUBasedValidation(TRUE);
            }
            return r;
        }

        static winrt::com_ptr<ID3D12Device4> CreateDevice(IUnknown* adapter = nullptr)
        {
            winrt::com_ptr<ID3D12Device4> r;

            //One can use d3d12 rendering with d3d11 capable hardware. You will just be missing new functionality.
            //Example, d3d12 on a D3D_FEATURE_LEVEL_9_1 hardare (as some phone are ).
            D3D_FEATURE_LEVEL features = D3D_FEATURE_LEVEL_11_1;
            ThrowIfFailed(D3D12CreateDevice(adapter, features, __uuidof(ID3D12Device4), r.put_void()));
            return r;
        }

        static winrt::com_ptr<ID3D12Device4> CreateWarpDevice() 
        {
            winrt::com_ptr<IDXGIFactory4> f;

            ThrowIfFailed(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, __uuidof(IDXGIFactory4), f.put_void()));

            winrt::com_ptr<IDXGIAdapter1> adapter;

            ThrowIfFailed(f->EnumWarpAdapter(__uuidof(IDXGIAdapter1), adapter.put_void()));

            return CreateDevice(adapter.get());
        }

        static winrt::com_ptr<ID3D12CommandQueue> CreateCommandQueue(ID3D12Device* d)
        {
            winrt::com_ptr<ID3D12CommandQueue> r;
            D3D12_COMMAND_QUEUE_DESC q = {};

            q.Type = D3D12_COMMAND_LIST_TYPE_DIRECT; //submit copy, raster, compute payloads
            ThrowIfFailed(d->CreateCommandQueue(&q, __uuidof(ID3D12CommandQueue), r.put_void()));
            return r;
        }

        static winrt::com_ptr <ID3D12Fence> CreateFence(ID3D12Device1* device, uint64_t initialValue = 1)
        {
            winrt::com_ptr<ID3D12Fence> r;
            ThrowIfFailed(device->CreateFence(initialValue, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), r.put_void()));
            return r;
        }

        static winrt::com_ptr <ID3D12DescriptorHeap> CreateRenderDescriptorHeap(ID3D12Device1* device)
        {
            winrt::com_ptr<ID3D12DescriptorHeap> r;
            D3D12_DESCRIPTOR_HEAP_DESC d = {};

            d.NumDescriptors = 4;
            d.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            device->CreateDescriptorHeap(&d, __uuidof(ID3D12DescriptorHeap), r.put_void());
            return r;
        }

        static winrt::com_ptr <ID3D12DescriptorHeap> CreateDepthDescriptorHeap(ID3D12Device1* device)
        {
            winrt::com_ptr<ID3D12DescriptorHeap> r;
            D3D12_DESCRIPTOR_HEAP_DESC d = {};

            d.NumDescriptors = 4;
            d.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
            device->CreateDescriptorHeap(&d, __uuidof(ID3D12DescriptorHeap), r.put_void());
            return r;
        }

        static winrt::com_ptr<IDXGISwapChain3> CreateSwapChainPrivate( IUnknown* w, ID3D12CommandQueue* d, uint32_t width, uint32_t height)
        {
            winrt::com_ptr<IDXGIFactory2> f;
            winrt::com_ptr<IDXGISwapChain1> r;

            ThrowIfFailed(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, __uuidof(IDXGIFactory2), f.put_void()));

            DXGI_SWAP_CHAIN_DESC1 desc = {};

            desc.BufferCount = 2;
            desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            desc.Width = width;
            desc.Height = height;
            desc.SampleDesc.Count = 1;
            desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
            desc.Scaling = DXGI_SCALING_NONE;

            //ThrowIfFailed(f->CreateSwapChainForCoreWindow(d, sample::copy_to_abi<IUnknown>(w).get(), &desc, nullptr, r.put()));
			ThrowIfFailed(f->CreateSwapChainForCoreWindow(d, w, &desc, nullptr, r.put()));
            return r.as< IDXGISwapChain3>();
        }



        //Get the buffer for the swap chain, this is the end result for the window
        static winrt::com_ptr<ID3D12Resource1> CreateSwapChainResource(ID3D12Device1* device, IDXGISwapChain* chain, uint32_t buffer)
        {
            winrt::com_ptr<ID3D12Resource1> r;

            chain->GetBuffer(buffer, __uuidof(ID3D12Resource1), r.put_void());
            return r;
        }

        //compute sizes
        static D3D12_RESOURCE_DESC DescribeDepth(uint32_t width, uint32_t height)
        {
            D3D12_RESOURCE_DESC d = {};
            d.Alignment = 0;
            d.DepthOrArraySize = 1;
            d.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            d.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE; //say that the depth will not be used as a shader resource to trigger compression
            d.Format = DXGI_FORMAT_D32_FLOAT;           //important for computing the resource footprint
            d.Height = height;
            d.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
            d.MipLevels = 1;
            d.SampleDesc.Count = 1;
            d.SampleDesc.Quality = 0;
            d.Width = width;
            return                  d;
        }

        static winrt::com_ptr<ID3D12Resource1> CreateDepthResource1(ID3D12Device1* device, uint32_t width, uint32_t height)
        {
            D3D12_RESOURCE_DESC d = DescribeDepth(width, height);

            winrt::com_ptr<ID3D12Resource1>     r;
            D3D12_HEAP_PROPERTIES p = {};
            p.Type = D3D12_HEAP_TYPE_DEFAULT;
            D3D12_RESOURCE_STATES       state = D3D12_RESOURCE_STATE_DEPTH_WRITE;

            D3D12_CLEAR_VALUE v = {};
            v.DepthStencil.Depth = 1.0f;
            v.DepthStencil.Stencil = 0;
            v.Format = DXGI_FORMAT_D32_FLOAT;

            ThrowIfFailed(device->CreateCommittedResource(&p, D3D12_HEAP_FLAG_NONE, &d, state, &v, __uuidof(ID3D12Resource1), r.put_void()));
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

        static void CreateDepthDescriptor(ID3D12Device1* device, ID3D12Resource1* resource, D3D12_CPU_DESCRIPTOR_HANDLE handle)
        {
            D3D12_DEPTH_STENCIL_VIEW_DESC d = {};
            d.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            d.Format        = DXGI_FORMAT_D32_FLOAT;       //how we will view the resource during rendering
            device->CreateDepthStencilView(resource, &d, handle);
        }
    }

    DeviceResources::DeviceResources()
    {
        m_debug = CreateDebug();
		
        m_device = CreateWarpDevice();
		//m_device = CreateDevice();

        m_queue = CreateCommandQueue(m_device.get());
        m_render_target_descriptor_heap = CreateRenderDescriptorHeap(m_device.get());
        m_depth_stencil_descriptor_heap = CreateDepthDescriptorHeap(m_device.get());

        //fence, sync from the gpu and cpu
        m_fence = CreateFence(m_device.get(), 0);
        m_fence_event = CreateEvent(nullptr, false, false, nullptr);

        if (m_fence_event == nullptr)
        {
            ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }
    }

    ID3D12Device1* DeviceResources::Device() const
    {
        return m_device.get();
    }

    ID3D12Fence* DeviceResources::Fence() const
    {
        return m_fence.get();
    }

    ID3D12CommandQueue* DeviceResources::Queue() const
    {
        return m_queue.get();
    }

    ID3D12Resource1* DeviceResources::SwapChainBuffer(uint32_t index) const
    {
        return m_swap_chain_buffers[index].get();
    }

    IDXGISwapChain3* DeviceResources::SwapChain() const
    {
        return m_swap_chain.get();
    }

    uint32_t DeviceResources::CreateSwapChain( ::IUnknown* w, uint32_t width, uint32_t height)
    {
        m_swap_chain = CreateSwapChainPrivate(w, m_queue.get(), width, height);

        m_back_buffer_width = width;
        m_back_buffer_height = height;

        //allocate memory for the swap chain again
        m_swap_chain_buffers[0] = CreateSwapChainResource(m_device.get(), m_swap_chain.get(), 0);
        m_swap_chain_buffers[1] = CreateSwapChainResource(m_device.get(), m_swap_chain.get(), 1);

        //allocate memory for the depth buffers in the swap chain
        m_swap_chain_depths[0] = CreateDepthResource1(m_device.get(), width, height);
        m_swap_chain_depths[1] = CreateDepthResource1(m_device.get(), width, height);

        //set names so we can see them in pix
        m_swap_chain_buffers[0]->SetName(L"Buffer 0");
        m_swap_chain_buffers[1]->SetName(L"Buffer 1");

        m_swap_chain_depths[0]->SetName(L"Depth 0");
        m_swap_chain_depths[1]->SetName(L"Depth 1");

        //create render target views, that will be used for rendering
        CreateSwapChainDescriptor(m_device.get(), m_swap_chain_buffers[0].get(), CpuView(m_device.get(), m_render_target_descriptor_heap.get()) + 0);
        CreateSwapChainDescriptor(m_device.get(), m_swap_chain_buffers[1].get(), CpuView(m_device.get(), m_render_target_descriptor_heap.get()) + 1);

        //create depth stencil view, that will be used for rendering
        CreateDepthDescriptor(m_device.get(), m_swap_chain_depths[0].get(), CpuView(m_device.get(), m_depth_stencil_descriptor_heap.get()) + 0);
        CreateDepthDescriptor(m_device.get(), m_swap_chain_depths[1].get(), CpuView(m_device.get(), m_depth_stencil_descriptor_heap.get()) + 1);

        m_swap_chain_descriptors[0] = 0;
        m_swap_chain_descriptors[1] = 1;

        return m_swap_chain->GetCurrentBackBufferIndex();
    }

    uint32_t DeviceResources::ResizeBuffers(uint32_t width, uint32_t height)
    {
        m_back_buffer_width     = width;
        m_back_buffer_height    = height;

        //Reset swap chains
        m_swap_chain_buffers[0] = nullptr;
        m_swap_chain_buffers[1] = nullptr;

        //Reset depths. Reset First ensure that we release the pressure on the driver to allocate new memory for us and release the old one
        m_swap_chain_depths[0] = nullptr;
        m_swap_chain_depths[1] = nullptr;

        sample::ThrowIfFailed(m_swap_chain->ResizeBuffers(2, m_back_buffer_width, m_back_buffer_height, DXGI_FORMAT_B8G8R8A8_UNORM, 0));

        //allocate memory for the swap chain again
        m_swap_chain_buffers[0] = CreateSwapChainResource(m_device.get(), m_swap_chain.get(), 0);
        m_swap_chain_buffers[1] = CreateSwapChainResource(m_device.get(), m_swap_chain.get(), 1);

        //allocate memory for the depth buffers in the swap chain
        m_swap_chain_depths[0] = CreateDepthResource1(m_device.get(), width, height);
        m_swap_chain_depths[1] = CreateDepthResource1(m_device.get(), width, height);

        //set names so we can see them in pix
        m_swap_chain_buffers[0]->SetName(L"Buffer 0");
        m_swap_chain_buffers[1]->SetName(L"Buffer 1");

        m_swap_chain_depths[0]->SetName(L"Depth 0");
        m_swap_chain_depths[1]->SetName(L"Depth 1");

        //create render target views, that will be used for rendering
        CreateSwapChainDescriptor(m_device.get(), m_swap_chain_buffers[0].get(), CpuView(m_device.get(), m_render_target_descriptor_heap.get()) + 0);
        CreateSwapChainDescriptor(m_device.get(), m_swap_chain_buffers[1].get(), CpuView(m_device.get(), m_render_target_descriptor_heap.get()) + 1);

        //create depth stencil view, that will be used for rendering
        CreateDepthDescriptor(m_device.get(), m_swap_chain_depths[0].get(), CpuView(m_device.get(), m_depth_stencil_descriptor_heap.get()) + 0);
        CreateDepthDescriptor(m_device.get(), m_swap_chain_depths[1].get(), CpuView(m_device.get(), m_depth_stencil_descriptor_heap.get()) + 1);

        m_swap_chain_descriptors[0] = 0;
        m_swap_chain_descriptors[1] = 1;

        return m_swap_chain->GetCurrentBackBufferIndex();
    }

    uint32_t DeviceResources::SwapChainWidth() const
    {
        return m_back_buffer_width;
    }

    uint32_t DeviceResources::SwapChainHeight() const
    {
        return m_back_buffer_height;
    }

    void  DeviceResources::WaitForFenceValue(uint64_t value)
    {
        //Now block the cpu until the gpu completes the previous frame
        if (m_fence->GetCompletedValue() < value)
        {
            ThrowIfFailed(m_fence->SetEventOnCompletion(value, m_fence_event));
            WaitForSingleObjectEx(m_fence_event, INFINITE, FALSE);
        }
    }

    void  DeviceResources::SignalFenceValue(uint64_t value)
    {
        ThrowIfFailed(m_queue->Signal(m_fence.get(), value));
    }

    D3D12_CPU_DESCRIPTOR_HANDLE  DeviceResources::SwapChainHandle(uint32_t index) const
    {
        return CpuView(m_device.get(), m_render_target_descriptor_heap.get()) + m_swap_chain_descriptors[index];
    }

    D3D12_CPU_DESCRIPTOR_HANDLE  DeviceResources::SwapChainDepthHandle(uint32_t index) const
    {
        return CpuView(m_device.get(), m_depth_stencil_descriptor_heap.get()) + m_swap_chain_descriptors[index];
    }

    ID3D12DescriptorHeap* DeviceResources::RenderTargetHeap() const
    {
        return m_render_target_descriptor_heap.get();
    }

    ID3D12DescriptorHeap* DeviceResources::DepthHeap() const
    {
        return m_depth_stencil_descriptor_heap.get();
    }
}