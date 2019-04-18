#include "pch.h"
#include "sampling_renderer.h"
#include "sample_settings.h"
#include "error.h"
#include "cpu_view.h"
#include "d3dx12.h"

namespace sample
{
    namespace
    {
		DecodedSample DecodeSample(unsigned int encodedSample)
		{
			//Separate the bytes
			uint8_t sampleB = static_cast<uint8_t>(encodedSample & 0xFF);
			uint8_t sampleG = static_cast<uint8_t>((encodedSample >> 8) & 0xFF);
			uint8_t sampleR = static_cast<uint8_t>((encodedSample >> 16) & 0xFF);
			uint8_t sampleA = static_cast<uint8_t>((encodedSample >> 24) & 0xFF);

			//Dequantize to [-1 ; 1]
			float x = 2.0f * static_cast<float>(sampleR) / 255.0f - 1.0f;
			float y = 2.0f * static_cast<float>(sampleG) / 255.0f - 1.0f;
			float z = 2.0f * static_cast<float>(sampleB) / 255.0f - 1.0f;

			//Compute the load
			float lod = (static_cast<float>(sampleA) / 255.0f) * 16.0f;	//maximum mip levels from 0 - 15

			short mip = lod < 0.0f ? 0 : lod > 14.0f ? 14 : static_cast<unsigned short>(lod); //clamp to texture data

			short face = 0;
			float u = 0.0f;
			float v = 0.0f;

			if (abs(x) > abs(y) && abs(x) > abs(z))
			{
				if (x > 0) // +X
				{
					face = 0;
					u = (1.0f - z / x) / 2.0f;
					v = (1.0f - y / x) / 2.0f;
				}
				else // -X
				{
					face = 1;
					u = (z / -x + 1.0f) / 2.0f;
					v = (1.0f - y / -x) / 2.0f;
				}
			}
			else if (abs(y) > abs(x) && abs(y) > abs(z))
			{
				if (y > 0) // +Y
				{
					face = 2;
					u = (x / y + 1.0f) / 2.0f;
					v = (z / y + 1.0f) / 2.0f;
				}
				else // -Y
				{
					face = 3;
					u = (x / -y + 1.0f) / 2.0f;
					v = (1.0f - z / -y) / 2.0f;
				}
			}
			else
			{
				if (z > 0) // +Z
				{
					face = 4;
					u = (x / z + 1.0f) / 2.0f;
					v = (1.0f - y / z) / 2.0f;
				}
				else // -Z
				{
					face = 5;
					u = (1.0f - x / -z) / 2.0f;
					v = (1.0f - y / -z) / 2.0f;
				}
			}

			return  { u, v, mip, face };
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

        //compute sizes
        static D3D12_RESOURCE_DESC DescribeRenderTarget(uint32_t width, uint32_t height)
        {
            D3D12_RESOURCE_DESC d = {};
            d.Alignment = 0;
            d.DepthOrArraySize = 1;
            d.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            d.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
            d.Format = DXGI_FORMAT_B8G8R8A8_TYPELESS;           //important for computing the resource footprint
            d.Height = height;
            d.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
            d.MipLevels = 1;
            d.SampleDesc.Count = 1;
            d.SampleDesc.Quality = 0;
            d.Width = width;
            return                  d;
        }

        static winrt::com_ptr<ID3D12Resource1> CreateRenderTargetResource1(ID3D12Device1* device, uint32_t width, uint32_t height)
        {
            D3D12_RESOURCE_DESC d = DescribeRenderTarget(width, height);

            winrt::com_ptr<ID3D12Resource1>     r;
            D3D12_HEAP_PROPERTIES p = {};
            p.Type = D3D12_HEAP_TYPE_DEFAULT;
			D3D12_RESOURCE_STATES       state = D3D12_RESOURCE_STATE_COPY_SOURCE;

            D3D12_CLEAR_VALUE v = {};
            v.Format = DXGI_FORMAT_B8G8R8A8_UNORM;

            ThrowIfFailed(device->CreateCommittedResource(&p, D3D12_HEAP_FLAG_NONE, &d, state, &v, __uuidof(ID3D12Resource1), r.put_void()));
            return r;
        }

		//compute sizes
		static D3D12_RESOURCE_DESC DescribeStagingResource(uint32_t width)
		{
			D3D12_RESOURCE_DESC d = {};
			d.Alignment = 0;
			d.DepthOrArraySize = 1;
			d.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			d.Format = DXGI_FORMAT_UNKNOWN;			          //important for computing the resource footprint
			d.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			d.MipLevels = 1;
			d.SampleDesc.Count = 1;
			d.SampleDesc.Quality = 0;
			d.Width = width;
			d.Height = 1;
			return   d;
		}

		static winrt::com_ptr<ID3D12Resource1> CreateStagingResource1(ID3D12Device1* device, ID3D12Resource1* source)
		{
			uint64_t size = 0;
			device->GetCopyableFootprints(&source->GetDesc(), 0, 1, 0, nullptr, nullptr, nullptr, &size);
			
			D3D12_RESOURCE_DESC d = DescribeStagingResource(size);
			winrt::com_ptr<ID3D12Resource1>     r;
			D3D12_HEAP_PROPERTIES p = {};
			p.Type = D3D12_HEAP_TYPE_READBACK;
			D3D12_RESOURCE_STATES       state = D3D12_RESOURCE_STATE_COPY_DEST;
			ThrowIfFailed(device->CreateCommittedResource(&p, D3D12_HEAP_FLAG_NONE, &d, state, nullptr, __uuidof(ID3D12Resource1), r.put_void()));
			return r;
		}

        //Create a gpu metadata that describes the swap chain, type, format. it will be used by the gpu interpret the data in the swap chain(reading/writing).
        static void CreateRenderTargetDescriptor(ID3D12Device1* device, ID3D12Resource1* resource, D3D12_CPU_DESCRIPTOR_HANDLE handle)
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

    SamplingRenderer::SamplingRenderer()
    {

    }

  
    ResizeSamplingRendererResult SamplingRenderer::CreateSamplingRenderer(const ResizeSamplingRendererContext& ctx)
    {
        m_sampling_width  = ctx.m_width / sample::SampleSettings::Sampling::Ratio;
        m_sampling_height = ctx.m_height / sample::SampleSettings::Sampling::Ratio;

        ID3D12Device1* d = ctx.m_device;

        //allocate memory for the render target for sampling renderer
        m_sampling_render_target[0] = CreateRenderTargetResource1(d, m_sampling_width, m_sampling_height);

        //allocate memory for the depth buffers in the swap chain
        m_sampling_depth[0] = CreateDepthResource1(d, m_sampling_width, m_sampling_height);

		//allocate memory that will be read on the cpu
		m_sampling_staging[0] = CreateStagingResource1(d, m_sampling_render_target[0].get());
		m_sampling_staging[1] = CreateStagingResource1(d, m_sampling_render_target[1].get());

        //set names so we can see them in pix
        m_sampling_render_target[0]->SetName(L"Sampling Buffer");
        m_sampling_depth[0]->SetName(L"Sampling Depth");

        m_sampling_descriptors[0]		= ctx.m_render_target_index;
		m_sampling_depth_descriptors[0] = ctx.m_depth_index;

        m_render_target_descriptor_heap = CpuView(d, ctx.m_render_target_heap);
        m_depth_stencil_descriptor_heap = CpuView(d, ctx.m_depth_heap);

        //create render target views, that will be used for rendering
        CreateRenderTargetDescriptor(d, m_sampling_render_target[0].get(), m_render_target_descriptor_heap + m_sampling_descriptors[0]);

        //create depth stencil view, that will be used for rendering
        CreateDepthDescriptor(d, m_sampling_depth[0].get(), m_depth_stencil_descriptor_heap + m_sampling_depth_descriptors[0]);

        //how many descriptors we have allocated
        return  { 1, 1 };
    }

    ResizeSamplingRendererResult SamplingRenderer::ResizeBuffers(const ResizeSamplingRendererContext& ctx)
    {
        //note: gpu must blocked here, so we can reallocate

        //allocate memory for the render target
        m_sampling_render_target[0] = nullptr;

        //allocate memory for the depth buffers
        m_sampling_depth[0] = nullptr;

        return CreateSamplingRenderer(ctx);
    }

    uint32_t SamplingRenderer::SamplingWidth() const
    {
        return m_sampling_width;
    }

    uint32_t SamplingRenderer::SamplingHeight() const
    {
        return m_sampling_height;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE  SamplingRenderer::SamplingHandle(uint32_t index) const
    {
        return m_render_target_descriptor_heap + m_sampling_descriptors[0];
    }

    D3D12_CPU_DESCRIPTOR_HANDLE  SamplingRenderer::SamplingDepthHandle(uint32_t index) const
    {
        return m_depth_stencil_descriptor_heap + m_sampling_depth_descriptors[0];
    }

	ID3D12Resource1* SamplingRenderer::SamplingRenderTarget(uint32_t index) const
	{
		return m_sampling_render_target[0].get();
	}

	ID3D12Resource1* SamplingRenderer::SamplingStaging(uint32_t index) const
	{
		return m_sampling_staging[index].get();
	}

	const std::vector<DecodedSample>& SamplingRenderer::Samples() const
	{
		return m_samples;
	}

	void SamplingRenderer::CollectSamples( uint32_t index, CollectParameters values)
	{
		std::vector<uint8_t> v;

		{
			v.resize(values.m_total_bytes);
			auto resource = m_sampling_staging[index].get();
			uint8_t* data;
			resource->Map(0, &CD3DX12_RANGE(0, values.m_total_bytes), reinterpret_cast<void**>(&data));
			std::memcpy(&v[0], data, values.m_total_bytes);
			resource->Unmap(0, nullptr);
		}

		std::vector< DecodedSample > samples;
		samples.reserve(10000);

		for ( auto y = 0UL; y < values.m_height; ++y )
		{
			uint32_t* row = reinterpret_cast<uint32_t*> (&v[0] + y * values.m_row_pitch);

			for (auto x = 0U; x <  values.m_width; ++x)
			{
				uint32_t value = row[x];
				//clear value
				if (value > 0)
				{
					samples.push_back(DecodeSample(value));
				}
			}
		}

		m_samples = std::move(samples);
	}
}