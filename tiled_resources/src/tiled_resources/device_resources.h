#pragma once

#include <dxgi1_6.h>
#include <d3d12.h>

namespace sample
{
    class DeviceResources
    {
        public:
        DeviceResources();

        ID3D12Device1*                              Device() const;
        ID3D12Fence*                                Fence() const;
        ID3D12CommandQueue*                         Queue() const;
        ID3D12Resource1*                            SwapChainBuffer(uint32_t index) const;
        IDXGISwapChain3*                            SwapChain() const;

        uint32_t                                    CreateSwapChain(::IUnknown* w, uint32_t width, uint32_t height);
        uint32_t                                    ResizeBuffers( uint32_t width, uint32_t height);
         
        uint32_t                                    SwapChainWidth() const;
        uint32_t                                    SwapChainHeight() const;

        void                                        WaitForFenceValue(uint64_t value);
        void                                        SignalFenceValue(uint64_t value);

        D3D12_CPU_DESCRIPTOR_HANDLE                 SwapChainHandle(uint32_t) const;
        D3D12_CPU_DESCRIPTOR_HANDLE                 SwapChainDepthHandle(uint32_t) const;

        ID3D12DescriptorHeap*                       RenderTargetHeap() const;
        ID3D12DescriptorHeap*                       DepthHeap() const;

		ID3D12DescriptorHeap*						ShaderHeap() const;
		ID3D12DescriptorHeap*						ShaderHeapGpu() const;

        private:
        winrt::com_ptr <ID3D12Debug1>               m_debug;                    //debug interface
        winrt::com_ptr <ID3D12Device1>				m_device;                   //device for gpu resources
        winrt::com_ptr <IDXGISwapChain3>			m_swap_chain;               //swap chain for 

        HANDLE										m_fence_event = {};
        winrt::com_ptr <ID3D12Fence>        		m_fence;                     //fence for cpu/gpu synchronization
        winrt::com_ptr <ID3D12CommandQueue>   		m_queue;                     //queue to the device

        winrt::com_ptr <ID3D12DescriptorHeap>   	m_render_target_descriptor_heap;	 //descriptor heap for the render_targets
        winrt::com_ptr <ID3D12DescriptorHeap>   	m_depth_stencil_descriptor_heap;	 //descriptor heap for the render_targets
		winrt::com_ptr <ID3D12DescriptorHeap>   	m_shader_descriptor_heap;			 //descriptor heap for the shaders
		winrt::com_ptr <ID3D12DescriptorHeap>   	m_shader_descriptor_heap_gpu;		 //descriptor heap for the shaders

        winrt::com_ptr<ID3D12Resource1>             m_swap_chain_buffers[2];
        winrt::com_ptr<ID3D12Resource1>             m_swap_chain_depths[1];

        uint64_t                                    m_swap_chain_descriptors[2];
        uint32_t									m_back_buffer_width = 0;
        uint32_t									m_back_buffer_height = 0;
    };
}

