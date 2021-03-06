#pragma once
#include "cpu_view.h"
#include "samples.h"

//Responsible for residency management.
//Which parts of the resources get upoaded


namespace sample
{
    struct ResizeSamplingRendererContext
    {
        ID3D12Device1*        m_device;

        ID3D12DescriptorHeap* m_render_target_heap;
        uint32_t              m_render_target_index;    //free slot

        ID3D12DescriptorHeap* m_depth_heap;
        uint32_t              m_depth_index;            //free slot

        uint32_t              m_width;
        uint32_t              m_height;
    };


    
    struct ResizeSamplingRendererResult
    {
        uint32_t m_allocated_render_slots;
        uint32_t m_allocated_depth_slots;
    };

    class SamplingRenderer
    {
        public:

		struct CollectParameters
		{
			uint64_t m_width;
			uint64_t m_height;
			uint64_t m_row_pitch;
			uint64_t m_total_bytes;
		};

        SamplingRenderer();

        ResizeSamplingRendererResult                CreateSamplingRenderer(const ResizeSamplingRendererContext& ctx);
        ResizeSamplingRendererResult                ResizeBuffers(const ResizeSamplingRendererContext& ctx);

        uint32_t                                    SamplingWidth() const;
        uint32_t                                    SamplingHeight() const;

        D3D12_CPU_DESCRIPTOR_HANDLE                 SamplingHandle(uint32_t) const;
        D3D12_CPU_DESCRIPTOR_HANDLE                 SamplingDepthHandle(uint32_t) const;

		ID3D12Resource1*							SamplingRenderTarget(uint32_t) const;
		ID3D12Resource1*							SamplingStaging(uint32_t) const;


		void										CollectSamples(uint32_t index, CollectParameters values);

		const std::vector< DecodedSample >&			Samples() const;

        private:

        DescriptorHeapCpuView                       m_render_target_descriptor_heap;
        DescriptorHeapCpuView                       m_depth_stencil_descriptor_heap;

        winrt::com_ptr<ID3D12Resource1>             m_sampling_render_target[1];
        winrt::com_ptr<ID3D12Resource1>             m_sampling_depth[1];

		uint64_t                                    m_sampling_descriptors[1] = {2};
		uint64_t                                    m_sampling_depth_descriptors[1] = { 1 };

		winrt::com_ptr<ID3D12Resource1>             m_sampling_staging[2];

        uint32_t									m_sampling_width = 0;
        uint32_t									m_sampling_height = 0;

		std::vector<DecodedSample>					m_samples;
    };
}

