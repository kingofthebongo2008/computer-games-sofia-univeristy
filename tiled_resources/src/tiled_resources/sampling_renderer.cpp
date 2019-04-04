#include "pch.h"
#include "sampling_renderer.h"
#include "sample_settings.h"
#include "error.h"
#include "cpu_view.h"

namespace sample
{
    namespace
    {
        //compute sizes
        static D3D12_RESOURCE_DESC DescribeDepth(uint32_t width, uint32_t height)
        {
            D3D12_RESOURCE_DESC d = {};
            d.Alignment = 0;
            d.DepthOrArraySize = 1;
            d.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            d.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
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
            D3D12_RESOURCE_STATES       state = D3D12_RESOURCE_STATE_RENDER_TARGET;

            D3D12_CLEAR_VALUE v = {};
            v.Format = DXGI_FORMAT_B8G8R8A8_UNORM;

            ThrowIfFailed(device->CreateCommittedResource(&p, D3D12_HEAP_FLAG_NONE, &d, state, &v, __uuidof(ID3D12Resource1), r.put_void()));
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
        m_sampling_width  = ctx.m_width / TiledResources::SampleSettings::Sampling::Ratio;
        m_sampling_height = ctx.m_height / TiledResources::SampleSettings::Sampling::Ratio;

        ID3D12Device1* d = ctx.m_device;

        //allocate memory for the render target
        m_sampling_render_target[0] = CreateRenderTargetResource1(d, m_sampling_width, m_sampling_height);
        m_sampling_render_target[1] = CreateRenderTargetResource1(d, m_sampling_width, m_sampling_height);

        //allocate memory for the depth buffers in the swap chain
        m_sampling_depth[0] = CreateDepthResource1(d, m_sampling_width, m_sampling_height);
        m_sampling_depth[1] = CreateDepthResource1(d, m_sampling_width, m_sampling_height);

        //set names so we can see them in pix
        m_sampling_render_target[0]->SetName(L"Sampling Buffer 0");
        m_sampling_render_target[1]->SetName(L"Sampling Buffer 1");

        m_sampling_depth[0]->SetName(L"Sampling Depth 0");
        m_sampling_depth[1]->SetName(L"Sampling Depth 1");

        m_sampling_descriptors[0] = ctx.m_render_target_index;
        m_sampling_descriptors[1] = ctx.m_render_target_index + 1;

        m_render_target_descriptor_heap = CpuView(d, ctx.m_render_target_heap);
        m_depth_stencil_descriptor_heap = CpuView(d, ctx.m_depth_heap);

        //create render target views, that will be used for rendering
        CreateRenderTargetDescriptor(d, m_sampling_render_target[0].get(), m_render_target_descriptor_heap + m_sampling_descriptors[0]);
        CreateRenderTargetDescriptor(d, m_sampling_render_target[1].get(), m_render_target_descriptor_heap + m_sampling_descriptors[1]);

        //create depth stencil view, that will be used for rendering
        CreateDepthDescriptor(d, m_sampling_depth[0].get(), m_depth_stencil_descriptor_heap + m_sampling_descriptors[0]);
        CreateDepthDescriptor(d, m_sampling_depth[1].get(), m_depth_stencil_descriptor_heap + m_sampling_descriptors[1]);

        //how many descriptors we have allocated
        return  { 2, 2 };
    }

    ResizeSamplingRendererResult SamplingRenderer::ResizeBuffers(const ResizeSamplingRendererContext& ctx)
    {
        //note: gpu must blocked here, so we can reallocate

        //allocate memory for the render target
        m_sampling_render_target[0] = nullptr;
        m_sampling_render_target[1] = nullptr;

        //allocate memory for the depth buffers
        m_sampling_depth[0] = nullptr;
        m_sampling_depth[1] = nullptr;

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
        return m_render_target_descriptor_heap + m_sampling_descriptors[index];
    }

    D3D12_CPU_DESCRIPTOR_HANDLE  SamplingRenderer::SamplingDepthHandle(uint32_t index) const
    {
        return m_depth_stencil_descriptor_heap + m_sampling_descriptors[index];
    }
}