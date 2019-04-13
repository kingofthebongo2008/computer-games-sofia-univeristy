//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved

#include "pch.h"
#include "residency_manager.h"
#include "error.h"
#include "d3dx12.h"
#include "sample_settings.h"
#include "cpu_view.h"

namespace sample
{
	static winrt::com_ptr<ID3D12Resource1 > CreateReservedResource(ID3D12Device1* device, uint32_t width, uint32_t height, DXGI_FORMAT f, uint32_t mipLevels)
	{
		D3D12_RESOURCE_DESC desc = {};
		desc.Alignment = 0;
		desc.DepthOrArraySize = 6;
		desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;
		desc.Format = f;
		desc.Height = height;
		desc.Layout = D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE;
		desc.MipLevels = mipLevels;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Width = width;

		winrt::com_ptr<ID3D12Resource1>     r;
		D3D12_RESOURCE_STATES       state = D3D12_RESOURCE_STATE_COPY_DEST;
		sample::ThrowIfFailed(device->CreateReservedResource(&desc, state, nullptr, __uuidof(ID3D12Resource1), r.put_void()));
		return r;
	}

	//Diffuse Residency
	inline D3D12_RESOURCE_DESC DescribeResidency(uint32_t width, uint32_t height)
	{
		D3D12_RESOURCE_DESC desc = {};
		desc.Alignment = 0;
		desc.DepthOrArraySize = 6;
		desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;
		desc.Format = DXGI_FORMAT_R8_UNORM;
		desc.Height = height;
		desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		desc.MipLevels = 1;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Width = width;
		return desc;
	}

	static winrt::com_ptr<ID3D12Resource1> CreateResidency(ID3D12Device1* device, uint32_t width, uint32_t height)
	{
		D3D12_RESOURCE_DESC d = DescribeResidency(width, height);

		winrt::com_ptr<ID3D12Resource1>     r;
		D3D12_HEAP_PROPERTIES p = {};
		p.Type = D3D12_HEAP_TYPE_DEFAULT;
		D3D12_RESOURCE_STATES       state = D3D12_RESOURCE_STATE_COPY_DEST;

		ThrowIfFailed(device->CreateCommittedResource(&p, D3D12_HEAP_FLAG_NONE, &d, state, nullptr, __uuidof(ID3D12Resource1), r.put_void()));
		return r;
	}

	//prepare for creation
	inline D3D12_RESOURCE_DESC DescribeBuffer(uint64_t elements, uint64_t elementSize = 1)
	{
		D3D12_RESOURCE_DESC desc = {};
		desc.Alignment = 0;
		desc.DepthOrArraySize = 1;
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.Height = 1;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.MipLevels = 1;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Width = elements * elementSize;
		return desc;
	}

	static winrt::com_ptr<ID3D12Resource1> CreateUploadResource(ID3D12Device1* device, uint32_t size)
	{
		D3D12_RESOURCE_DESC d		= DescribeBuffer(size);

		winrt::com_ptr<ID3D12Resource1>     r;
		D3D12_HEAP_PROPERTIES p = {};
		p.Type = D3D12_HEAP_TYPE_UPLOAD;
		D3D12_RESOURCE_STATES       state = D3D12_RESOURCE_STATE_GENERIC_READ;

		ThrowIfFailed(device->CreateCommittedResource(&p, D3D12_HEAP_FLAG_NONE, &d, state, nullptr, __uuidof(ID3D12Resource1), r.put_void()));
		return r;
	}

	static D3D12_HEAP_DESC DescribeHeap(uint32_t size)
	{
		D3D12_HEAP_DESC d				  = {};
		D3D12_HEAP_PROPERTIES p			  = {};
		p.Type							  = D3D12_HEAP_TYPE_DEFAULT;

		d.Flags							  = D3D12_HEAP_FLAG_DENY_BUFFERS | D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES;	//only textures here;
		d.Properties					  = p;
		d.SizeInBytes					  = size;

		return d;
	}

	static winrt::com_ptr<ID3D12Heap> CreatePhysicalHeap(ID3D12Device1* device, uint32_t size)
	{
		D3D12_HEAP_DESC d = DescribeHeap(size);

		winrt::com_ptr<ID3D12Heap>     r;

		ThrowIfFailed(device->CreateHeap(&d, __uuidof(ID3D12Heap), r.put_void()));
		return r;
	}

	ResidencyManager::ResidencyManager(ID3D12Device1* d)
	{
		m_upload_heap[0]	= CreateUploadResource(d, 16 * 1024 * 1024);
		m_upload_heap[1]	= CreateUploadResource(d, 16 * 1024 * 1024);
		m_physical_heap     = CreatePhysicalHeap(d,   32 * 1024 * 1024);
	}

	std::unique_ptr<ManagedTiledResource> MakeManagedResource( ID3D12Device1* d, winrt::com_ptr<ID3D12Resource1> resource, const std::wstring& filename )
	{
		std::unique_ptr<ManagedTiledResource> r = std::make_unique<ManagedTiledResource>();
		ManagedTiledResource* p = r.get();

		//Create the reserved resource
		p->m_resource = resource;
		p->m_textureDescription = p->m_resource->GetDesc();
		uint32_t subresourceTilings = p->m_textureDescription.MipLevels * p->m_textureDescription.DepthOrArraySize;

		//Fetch tiling information
		p->m_subresourceTilings.resize(subresourceTilings);
		d->GetResourceTiling(p->m_resource.get(), &p->m_totalTiles, &p->m_packedMipDescription, &p->m_tileShape, &subresourceTilings, 0, p->m_subresourceTilings.data());
		p->m_loader = std::make_unique<TileLoader>(filename, &p->m_subresourceTilings);

		//Update the shadow residency buffer, set it up to point to the last mip
		//this will be updated from the streaming system
		for (auto face = 0U; face < 6U; face++)
		{
			p->m_residencyShadow[face].clear();
			p->m_residencyShadow[face].resize(p->ResidencyWidth() * p->ResidencyHeight(), 0xFF);
		}

		/*
		p->m_residencyShadow[0].clear();
		p->m_residencyShadow[1].clear();
		p->m_residencyShadow[2].clear();
		p->m_residencyShadow[3].clear();
		p->m_residencyShadow[4].clear();
		p->m_residencyShadow[5].clear();

		p->m_residencyShadow[0].resize(p->ResidencyWidth() * p->ResidencyHeight(), 0xFF);
		p->m_residencyShadow[1].resize(p->ResidencyWidth() * p->ResidencyHeight(), 0x28 + 0x28 + 0x28 + 0x28);
		p->m_residencyShadow[2].resize(p->ResidencyWidth() * p->ResidencyHeight(), 0x28 + 0x28 + 0x28);
		p->m_residencyShadow[3].resize(p->ResidencyWidth() * p->ResidencyHeight(), 0x28 + 0x28);
		p->m_residencyShadow[4].resize(p->ResidencyWidth() * p->ResidencyHeight(), 0x28);
		p->m_residencyShadow[5].resize(p->ResidencyWidth() * p->ResidencyHeight(), 0x00);
		*/

		//create the small texture
		p->m_residencyResource			= CreateResidency(d, p->ResidencyWidth(), p->ResidencyHeight());

		//create the upload heaps for residency texture, so we can upload it every frame if needed
		//one per frame, 
		uint64_t size = GetRequiredIntermediateSize(p->m_residencyResource.get(), 0, 6);
		p->m_residencyResourceUpload[0] = CreateUploadResource(d, size);
		p->m_residencyResourceUpload[1] = CreateUploadResource(d, size);

		return r;
	}

	std::unique_ptr<ManagedTiledResource> MakeDiffuseResource(ID3D12Device1* d, const std::wstring& filename )
	{
		using namespace SampleSettings::TerrainAssets;
		winrt::com_ptr<ID3D12Resource1> diffuse =  CreateReservedResource(d, Diffuse::DimensionSize, Diffuse::DimensionSize, Diffuse::Format, Diffuse::UnpackedMipCount);
		return MakeManagedResource(d, std::move(diffuse), filename);
	}

	std::unique_ptr<ManagedTiledResource> MakeNormalResource(ID3D12Device1* d, const std::wstring& filename)
	{
		using namespace SampleSettings::TerrainAssets;
		winrt::com_ptr<ID3D12Resource1> normal = CreateReservedResource(d, Normal::DimensionSize, Normal::DimensionSize, Normal::Format, Normal::UnpackedMipCount);
		return MakeManagedResource(d, std::move(normal), filename);
	}

	static void CreateDiffuseShaderResourceView(ID3D12Device1* d, ID3D12Resource1* r, D3D12_CPU_DESCRIPTOR_HANDLE h)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
		desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		desc.Format = SampleSettings::TerrainAssets::Diffuse::Format;
		desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		desc.TextureCube.MipLevels = SampleSettings::TerrainAssets::Diffuse::UnpackedMipCount;
		d->CreateShaderResourceView(r, &desc, h);
	}

	static void CreateNormalShaderResourceView(ID3D12Device1* d, ID3D12Resource1* r, D3D12_CPU_DESCRIPTOR_HANDLE h)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
		desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		desc.Format = SampleSettings::TerrainAssets::Normal::Format;
		desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		desc.TextureCube.MipLevels = SampleSettings::TerrainAssets::Normal::UnpackedMipCount;
		d->CreateShaderResourceView(r, &desc, h);
	}

	static void CreateResidencyShaderResourceView(ID3D12Device1* d, ID3D12Resource1* r, D3D12_CPU_DESCRIPTOR_HANDLE h)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
		desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		desc.Format = DXGI_FORMAT_R8_UNORM;
		desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		desc.TextureCube.MipLevels = 1;
		d->CreateShaderResourceView(r, &desc, h);
	}

	ResidencyManagerCreateResult ResidencyManager::CreateResidencyManager(const ResidencyManagerCreateContext& ctx)
	{
		m_resources[0] = MakeDiffuseResource(ctx.m_device, ctx.m_diffuse);
		m_resources[1] = MakeNormalResource(ctx.m_device, ctx.m_normal);

		//Now setup the views
		ResidencyManagerCreateResult r;

		r.m_diffuse_srv				= ctx.m_shader_heap_index;
		r.m_diffuse_residency_srv	= ctx.m_shader_heap_index+1;

		r.m_normal_srv				= ctx.m_shader_heap_index + 2;
		r.m_normal_residency_srv	= ctx.m_shader_heap_index + 3;

		CreateDiffuseShaderResourceView(ctx.m_device, m_resources[0]->m_resource.get(), CpuView(ctx.m_device, ctx.m_shader_heap) + r.m_diffuse_srv);
		CreateNormalShaderResourceView(ctx.m_device, m_resources[1]->m_resource.get(), CpuView(ctx.m_device, ctx.m_shader_heap) + r.m_normal_srv);

		CreateResidencyShaderResourceView(ctx.m_device, m_resources[0]->m_residencyResource.get(), CpuView(ctx.m_device, ctx.m_shader_heap) + r.m_diffuse_residency_srv);
		CreateResidencyShaderResourceView(ctx.m_device, m_resources[1]->m_residencyResource.get(), CpuView(ctx.m_device, ctx.m_shader_heap) + r.m_normal_residency_srv);

		m_resources[0]->m_resource->SetName(L"diffuse.bin");
		m_resources[1]->m_resource->SetName(L"normal.bin");

		m_resources[0]->m_residencyResource->SetName(L"diffuseResidency.bin");
		m_resources[1]->m_residencyResource->SetName(L"normalResidency.bin");

		return r;
	}

	void ResidencyManager::ResetInitialData(ID3D12GraphicsCommandList* list, uint32_t frame_index)
	{
		//Upload all residency resources, face by face

		for ( auto i = 0U; i < 2; ++i)
		{
			auto r = m_resources[i].get();

			D3D12_SUBRESOURCE_DATA data[6];
			for (auto i = 0U; i < 6; ++i)
			{
				data[i].pData		= &r->m_residencyShadow[i][0];
				data[i].RowPitch	= r->ResidencyWidth();
				data[i].SlicePitch	= r->ResidencyHeight();
			}

			UpdateSubresources<6>(list, r->m_residencyResource.get(), r->m_residencyResourceUpload[frame_index].get(), 0, 0, 6, data);
		}

		//Transtion resources, make them ready for sampling
		{
			D3D12_RESOURCE_BARRIER barrier[4] = {};

			for (auto i = 0U; i < 4; ++i)
			{
				barrier[i].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				barrier[i].Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
				barrier[i].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
				barrier[i].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			}

			barrier[0].Transition.pResource = m_resources[0]->m_resource.get();
			barrier[1].Transition.pResource = m_resources[1]->m_resource.get();

			barrier[2].Transition.pResource = m_resources[0]->m_residencyResource.get();
			barrier[3].Transition.pResource = m_resources[1]->m_residencyResource.get();

			list->ResourceBarrier(4, barrier);
		}
	}

	void ResidencyManager::UpdateTiles(ID3D12GraphicsCommandList* list, uint32_t frame_index)
	{

	}

	ID3D12Resource1* ResidencyManager::Diffuse()
	{
		return m_resources[0]->m_resource.get();
	}

	ID3D12Resource1* ResidencyManager::DiffuseResidency()
	{
		return m_resources[0]->m_residencyResource.get();
	}

	ID3D12Resource1* ResidencyManager::Normal()
	{
		return m_resources[1]->m_resource.get();
	}

	ID3D12Resource1* ResidencyManager::NormalResidency()
	{
		return m_resources[1]->m_residencyResource.get();
	}
}
/*
#include "DirectXHelper.h"
#include "SampleSettings.h"
#include "ResidencyManager.h"

using namespace TiledResources;

using namespace concurrency;
using namespace DirectX;
using namespace Windows::Foundation;

ResidencyManager::ResidencyManager(const std::shared_ptr<DX::DeviceResources>& deviceResources) :
    m_deviceResources(deviceResources),
    m_debugMode(false),
    m_activeTileLoadingOperations(0),
    m_reservedTiles(0),
    m_defaultTileIndex(-1)
{
    CreateDeviceDependentResources();
}

void ResidencyManager::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();
    auto context = m_deviceResources->GetD3DDeviceContext();

    // Create the constant buffer for viewer constants.
    D3D11_BUFFER_DESC constantBufferDesc;
    ZeroMemory(&constantBufferDesc, sizeof(constantBufferDesc));
    constantBufferDesc.ByteWidth = sizeof(XMFLOAT4X4);
    constantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    constantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    DX::ThrowIfFailed(device->CreateBuffer(&constantBufferDesc, nullptr, &m_viewerVertexShaderConstantBuffer));

    // Create the viewer vertex buffer.
    float vertexBufferData[] =
    {
        -1.0f, 0.5f, 1.0f, -1.0f, 1.0f, 0.0f,
        -0.5f, 1.0f, -1.0f, -1.0f, 1.0f, 0.0f,
        0.0f, 0.5f, -1.0f, 1.0f, 1.0f, 0.0f,
        0.5f, 1.0f, -1.0f, -1.0f, 1.0f, 0.0f,
        1.0f, 0.5f, 1.0f, -1.0f, 1.0f, 0.0f,
        1.0f, -0.5f, 1.0f, -1.0f, -1.0f, 0.0f,
        0.5f, -1.0f, 1.0f, 1.0f, -1.0f, 0.0f,
        0.0f, -0.5f, -1.0f, 1.0f, -1.0f, 0.0f,
        -0.5f, -1.0f, 1.0f, 1.0f, -1.0f, 0.0f,
        -1.0f, -0.5f, 1.0f, -1.0f, -1.0f, 0.0f,
        -0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f,
        0.5f, 0.0f, -1.0f, -1.0f, -1.0f, 0.0f
    };
    D3D11_BUFFER_DESC vertexBufferDesc;
    ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));
    vertexBufferDesc.ByteWidth = sizeof(vertexBufferData);
    vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vertexBufferInitialData = {vertexBufferData, 0, 0};
    DX::ThrowIfFailed(device->CreateBuffer(&vertexBufferDesc, &vertexBufferInitialData, &m_viewerVertexBuffer));

    // Create the viewer index buffer.
    unsigned int indexBufferData[] =
    {
        0, 1, 10,
        1, 2, 10,
        2, 7, 10,
        7, 8, 10,
        8, 9, 10,
        9, 0, 10,
        2, 3, 11,
        3, 4, 11,
        4, 5, 11,
        5, 6, 11,
        6, 7, 11,
        7, 2, 11
    };
    D3D11_BUFFER_DESC indexBufferDesc;
    ZeroMemory(&indexBufferDesc, sizeof(indexBufferDesc));
    indexBufferDesc.ByteWidth = sizeof(indexBufferData);
    indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA indexBufferInitialData = {indexBufferData, 0, 0};
    DX::ThrowIfFailed(device->CreateBuffer(&indexBufferDesc, &indexBufferInitialData, &m_viewerIndexBuffer));
    m_indexCount = ARRAYSIZE(indexBufferData);

    // Create wrapping point sampler.
    D3D11_SAMPLER_DESC samplerDesc;
    ZeroMemory(&samplerDesc, sizeof(samplerDesc));
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
    DX::ThrowIfFailed(device->CreateSamplerState(&samplerDesc, &m_sampler));

    // Create the tile pool.
    D3D11_BUFFER_DESC tilePoolDesc;
    ZeroMemory(&tilePoolDesc, sizeof(tilePoolDesc));
    tilePoolDesc.ByteWidth = SampleSettings::TileSizeInBytes * SampleSettings::TileResidency::PoolSizeInTiles;
    tilePoolDesc.Usage = D3D11_USAGE_DEFAULT;
    tilePoolDesc.MiscFlags = D3D11_RESOURCE_MISC_TILE_POOL;
    DX::ThrowIfFailed(device->CreateBuffer(&tilePoolDesc, nullptr, &m_tilePool));
}

void ResidencyManager::ReleaseDeviceDependentResources()
{
    m_viewerVertexBuffer.Reset();
    m_viewerIndexBuffer.Reset();
    m_viewerInputLayout.Reset();
    m_viewerVertexShader.Reset();
    m_viewerPixelShader.Reset();
    m_sampler.Reset();
    m_viewerVertexShaderConstantBuffer.Reset();
    m_tilePool.Reset();

    m_managedResources.clear();
    m_trackedTiles.clear();
    m_seenTileList.clear();
    m_loadingTileList.clear();
    m_mappedTileList.clear();
}

void ResidencyManager::RenderVisualization()
{
    
}

void ResidencyManager::EnqueueSamples(const std::vector<DecodedSample>& samples, const DX::StepTimer& timer)
{
    for (auto sample : samples)
    {
        // Interpret each sample in the context of each managed resource.
        for (auto managedResource : m_managedResources)
        {
            // Samples are encoded assuming a maximally-sized (15-MIP) texture, so offset the
            // sampled value by the difference between actual MIPs and maximum MIPs.
            // Also, due to low-detail MIPs not being part of the MIP chain, we clamp the actual
            // MIP to guarantee it always hits at least one level represented in the tiled resource.
            short actualMip = max(0, min(static_cast<short>(managedResource->textureDesc.MipLevels) - 1, sample.mip + static_cast<short>(managedResource->textureDesc.MipLevels) - 15));

            // Loop over sampled MIP through the least detailed MIP.
            for (short mip = actualMip; mip < static_cast<short>(managedResource->textureDesc.MipLevels); mip++)
            {
                // Calculate the tile coordinate.
                TileKey tileKey;
                tileKey.resource = managedResource->texture;
                ZeroMemory(&tileKey.coordinate, sizeof(tileKey.coordinate));
                tileKey.coordinate.Subresource = mip + sample.face * managedResource->textureDesc.MipLevels;
                float tileX = managedResource->subresourceTilings[tileKey.coordinate.Subresource].WidthInTiles * sample.u;
                tileX = min(managedResource->subresourceTilings[tileKey.coordinate.Subresource].WidthInTiles - 1, max(0, tileX));
                tileKey.coordinate.X = static_cast<UINT>(tileX);
                float tileY = managedResource->subresourceTilings[tileKey.coordinate.Subresource].HeightInTiles * sample.v;
                tileY = min(managedResource->subresourceTilings[tileKey.coordinate.Subresource].HeightInTiles - 1, max(0, tileY));
                tileKey.coordinate.Y = static_cast<UINT>(tileY);

                // See if the tile is already being tracked.
                auto tileIterator = m_trackedTiles.find(tileKey);
                if (tileIterator == m_trackedTiles.end())
                {
                    // Tile is not being tracked currently, so enqueue it for load.
                    std::shared_ptr<TrackedTile> tile(new TrackedTile);
                    tile->managedResource = managedResource.get();
                    tile->coordinate = tileKey.coordinate;
                    tile->lastSeen = timer.GetFrameCount();
                    tile->state = TileState::Seen;
                    tile->mipLevel = mip;
                    tile->face = sample.face;
                    m_trackedTiles[tileKey] = tile;
                    m_seenTileList.push_back(tile);
                }
                else
                {
                    // If tile is already tracked, simply update the last-seen value.
                    tileIterator->second->lastSeen = timer.GetFrameCount();
                }
            }
        }
    }
}

void ResidencyManager::ProcessQueues()
{
    auto context = m_deviceResources->GetD3DDeviceContext();

    // Sort the tile lists.
    m_seenTileList.sort(LoadPredicate);
    m_loadingTileList.sort(MapPredicate);
    m_mappedTileList.sort(EvictPredicate);

    // Initiate loads for seen tiles.
    for (int i = m_activeTileLoadingOperations; i < SampleSettings::TileResidency::MaxSimultaneousFileLoadTasks; i++)
    {
        if (m_seenTileList.empty())
        {
            break;
        }
        auto tileToLoad = m_seenTileList.front();
        m_seenTileList.pop_front();

        InterlockedIncrement(&m_activeTileLoadingOperations);
        tileToLoad->managedResource->loader->LoadTileAsync(tileToLoad->coordinate).then([this, tileToLoad](std::vector<byte> tileData)
        {
            tileToLoad->tileData = tileData;
            tileToLoad->state = TileState::Loaded;
            InterlockedDecrement(&m_activeTileLoadingOperations);
        });

        // Move the tile to the loading list.
        m_loadingTileList.push_back(tileToLoad);
    }

    // Loop over the loading / loaded tile list for mapping candidates.
    struct TileMappingUpdateArguments
    {
        std::vector<D3D11_TILED_RESOURCE_COORDINATE> coordinates;
        std::vector<UINT> rangeFlags;
        std::vector<UINT> physicalOffsets;
        // For convenience, the tracked tile mapping is also saved.
        std::list < std::shared_ptr<TrackedTile>> tilesToMap;
    };
    std::map<ID3D11Texture2D*, TileMappingUpdateArguments> coalescedMapArguments;

    for (int i = 0; i < SampleSettings::TileResidency::MaxTilesLoadedPerFrame; i++)
    {
        if (m_loadingTileList.empty())
        {
            break;
        }

        auto tileToMap = m_loadingTileList.front();
        if (tileToMap->state != TileState::Loaded)
        {
            // This sample's residency management assumes that for a given texcoord,
            // there will never be a detailed MIP resident where a less detailed one
            // is NULL-mapped. This is enforced by sort predicates. A side-effect of
            // this technique is that mapping cannot occur out of order.
            break;
        }
        m_loadingTileList.pop_front();

        // Default to assigning tiles to the first available tile.
        UINT physicalTileOffset = m_reservedTiles + static_cast<UINT>(m_mappedTileList.size());

        if (m_mappedTileList.size() + m_reservedTiles == SampleSettings::TileResidency::PoolSizeInTiles)
        {
            // Tile pool is full, need to unmap something.
            auto tileToEvict = m_mappedTileList.front();

            if (tileToMap->lastSeen < tileToEvict->lastSeen)
            {
                // If the candidate tile to map is older than the eviction candidate,
                // skip the mapping and discard it. This can occur if a tile load stalls,
                // and by the time it is ready it has moved off-screen.
                TileKey tileKey;
                tileKey.coordinate = tileToMap->coordinate;
                tileKey.resource = tileToMap->managedResource->texture;

                // Remove the tile from the tracked list.
                m_trackedTiles.erase(tileKey);

                // Move on to the next map candidate.
                continue;
            }

            m_mappedTileList.pop_front();
            TileKey tileKey;
            tileKey.coordinate = tileToEvict->coordinate;
            tileKey.resource = tileToEvict->managedResource->texture;

            // Remove the tile from the tracked list.
            m_trackedTiles.erase(tileKey);

            // Save the physical tile that was freed so the new tile can use it.
            physicalTileOffset = tileToEvict->physicalTileOffset;

            // Add the new NULL-mapping to the argument list.
            coalescedMapArguments[tileToEvict->managedResource->texture].coordinates.push_back(tileToEvict->coordinate);
            coalescedMapArguments[tileToEvict->managedResource->texture].rangeFlags.push_back(D3D11_TILE_RANGE_NULL);
            coalescedMapArguments[tileToEvict->managedResource->texture].physicalOffsets.push_back(physicalTileOffset);

            // Update the residency map to remove this level of detail.
            int baseTilesCoveredWidth = tileToEvict->managedResource->subresourceTilings[0].WidthInTiles / tileToEvict->managedResource->subresourceTilings[tileToEvict->coordinate.Subresource].WidthInTiles;
            int baseTilesCoveredHeight = tileToEvict->managedResource->subresourceTilings[0].HeightInTiles / tileToEvict->managedResource->subresourceTilings[tileToEvict->coordinate.Subresource].HeightInTiles;
            for (int Y = 0; Y < baseTilesCoveredHeight; Y++)
            {
                for (int X = 0; X < baseTilesCoveredWidth; X++)
                {
                    int tileY = tileToEvict->coordinate.Y * baseTilesCoveredHeight + Y;
                    int tileX = tileToEvict->coordinate.X * baseTilesCoveredWidth + X;
                    byte* value = &tileToEvict->managedResource->residency[tileToEvict->face][tileY * tileToEvict->managedResource->subresourceTilings[0].WidthInTiles + tileX];
                    *value = max((tileToEvict->mipLevel + 1) * 16, *value);
                }
            }
        }

        // Add the new mapping to the argument list.
        coalescedMapArguments[tileToMap->managedResource->texture].coordinates.push_back(tileToMap->coordinate);
        coalescedMapArguments[tileToMap->managedResource->texture].rangeFlags.push_back(0);
        coalescedMapArguments[tileToMap->managedResource->texture].physicalOffsets.push_back(physicalTileOffset);
        tileToMap->physicalTileOffset = physicalTileOffset;
        tileToMap->state = TileState::Mapped;
        coalescedMapArguments[tileToMap->managedResource->texture].tilesToMap.push_back(tileToMap);

        // Update the residency map to add this level of detail.
        int baseTilesCoveredWidth = tileToMap->managedResource->subresourceTilings[0].WidthInTiles / tileToMap->managedResource->subresourceTilings[tileToMap->coordinate.Subresource].WidthInTiles;
        int baseTilesCoveredHeight = tileToMap->managedResource->subresourceTilings[0].HeightInTiles / tileToMap->managedResource->subresourceTilings[tileToMap->coordinate.Subresource].HeightInTiles;
        for (int Y = 0; Y < baseTilesCoveredHeight; Y++)
        {
            for (int X = 0; X < baseTilesCoveredWidth; X++)
            {
                int tileY = tileToMap->coordinate.Y * baseTilesCoveredHeight + Y;
                int tileX = tileToMap->coordinate.X * baseTilesCoveredWidth + X;
                byte* value = &tileToMap->managedResource->residency[tileToMap->face][tileY * tileToMap->managedResource->subresourceTilings[0].WidthInTiles + tileX];
                *value = min(tileToMap->mipLevel * 16, *value);
            }
        }

        m_mappedTileList.push_back(tileToMap);
    }

    // Use a single call to update all tile mappings.
    for (auto perResourceArguments : coalescedMapArguments)
    {
#ifdef _DEBUG
        if (m_debugMode)
        {
            for (size_t i = 0; i < perResourceArguments.second.coordinates.size(); i++)
            {
                std::ostringstream tileMappingMessage;
                tileMappingMessage << "ResidencyManager::ProcessQueues : Updating mapping for tile ";
                tileMappingMessage << "[" << perResourceArguments.second.coordinates[i].Subresource << "][" << perResourceArguments.second.coordinates[i].Y << "][" << perResourceArguments.second.coordinates[i].X << "]";
                if (perResourceArguments.second.rangeFlags[i] == D3D11_TILE_RANGE_NULL)
                {
                    tileMappingMessage << " --> NULL" << std::endl;
                }
                else
                {
                    tileMappingMessage << " --> Tile #" << perResourceArguments.second.physicalOffsets[i] << std::endl;
                }
                OutputDebugStringA(tileMappingMessage.str().c_str());
            }
        }
#endif
        std::vector<UINT> rangeCounts(perResourceArguments.second.rangeFlags.size(), 1);
        D3D11_TILE_REGION_SIZE size;
        ZeroMemory(&size, sizeof(size));
        size.NumTiles = 1;
        std::vector<D3D11_TILE_REGION_SIZE> sizes(perResourceArguments.second.rangeFlags.size(), size);
        DX::ThrowIfFailed(
            context->UpdateTileMappings(
                perResourceArguments.first,
                static_cast<UINT>(perResourceArguments.second.coordinates.size()),
                perResourceArguments.second.coordinates.data(),
                sizes.data(),
                m_tilePool.Get(),
                static_cast<UINT>(perResourceArguments.second.rangeFlags.size()),
                perResourceArguments.second.rangeFlags.data(),
                perResourceArguments.second.physicalOffsets.data(),
                rangeCounts.data(),
                0
                )
            );
    }

    // Finally, copy the contents of the tiles mapped this frame.
    for (auto perResourceArguments : coalescedMapArguments)
    {
        for (size_t i = 0; i < perResourceArguments.second.coordinates.size(); i++)
        {
            if (perResourceArguments.second.rangeFlags[i] != D3D11_TILE_RANGE_NULL)
            {
                D3D11_TILE_REGION_SIZE regionSize;
                ZeroMemory(&regionSize, sizeof(regionSize));
                regionSize.NumTiles = 1;
                context->UpdateTiles(
                    perResourceArguments.first,
                    &perResourceArguments.second.coordinates[i],
                    &regionSize,
                    perResourceArguments.second.tilesToMap.front()->tileData.data(),
                    0
                    );
                perResourceArguments.second.tilesToMap.front()->tileData.clear();
                perResourceArguments.second.tilesToMap.pop_front();
            }
        }
    }

    // Update residency textures with the new residency data.
    for (auto resource : m_managedResources)
    {
        int baseWidthInTiles = resource->subresourceTilings[0].WidthInTiles;
        int baseHeightInTiles = resource->subresourceTilings[0].HeightInTiles;
        int baseMaxTileDimension = max(baseWidthInTiles, baseHeightInTiles);
        std::vector<byte> residencyData(baseMaxTileDimension * baseMaxTileDimension);
        for (int face = 0; face < 6; face++)
        {
            for (int Y = 0; Y < baseMaxTileDimension; Y++)
            {
                int tileY = (Y * baseHeightInTiles) / baseMaxTileDimension;
                for (int X = 0; X < baseMaxTileDimension; X++)
                {
                    int tileX = (X * baseWidthInTiles) / baseMaxTileDimension;
                    residencyData[Y * baseMaxTileDimension + X] = resource->residency[face][tileY * baseWidthInTiles + tileX];
                }
            }
            context->UpdateSubresource(
                resource->residencyTexture.Get(),
                face,
                NULL,
                residencyData.data(),
                baseMaxTileDimension,
                0
                );
        }
    }
}

void ResidencyManager::SetDebugMode(bool value)
{
    m_debugMode = value;
}

void ResidencyManager::ResetTileMappings()
{
    auto device = m_deviceResources->GetD3DDevice();
    auto context = m_deviceResources->GetD3DDeviceContext();

    // Clear tracked tiles and residency map.
    m_trackedTiles.clear();
    m_seenTileList.clear();
    m_loadingTileList.clear();
    m_mappedTileList.clear();

    for (auto resource : m_managedResources)
    {
        for (int face = 0; face < 6; face++)
        {
            resource->residency[face].clear();
            resource->residency[face].resize(resource->subresourceTilings[0].WidthInTiles * resource->subresourceTilings[0].HeightInTiles, 0xFF);
        }
    }

    // Reset tile mappings to NULL.
    for (auto resource : m_managedResources)
    {
        UINT flags = D3D11_TILE_RANGE_NULL;
        DX::ThrowIfFailed(
            context->UpdateTileMappings(
                resource->texture,
                1,
                nullptr, // Use default coordinate of all zeros.
                nullptr, // Use default region of entire resource.
                nullptr,
                1,
                &flags,
                nullptr,
                nullptr,
                0
                )
            );
    }

    if (m_deviceResources->GetTiledResourcesTier() <= D3D11_TILED_RESOURCES_TIER_1)
    {
        // On Tier-1 devices, applications must ensure that NULL-mapped tiles are never accessed.
        // Because the mapping heuristic in this sample is only approximate, it is safest to map
        // all tiles to a dummy tile that will serve as the NULL tile.

        // Create a temporary buffer to clear the dummy tile to zero.
        D3D11_BUFFER_DESC tempBufferDesc;
        ZeroMemory(&tempBufferDesc, sizeof(tempBufferDesc));
        tempBufferDesc.ByteWidth = SampleSettings::TileSizeInBytes;
        tempBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_TILED;
        tempBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        Microsoft::WRL::ComPtr<ID3D11Buffer> tempBuffer;
        DX::ThrowIfFailed(device->CreateBuffer(&tempBufferDesc, nullptr, &tempBuffer));

        // Map the single tile in the buffer to physical tile 0.
        D3D11_TILED_RESOURCE_COORDINATE startCoordinate;
        ZeroMemory(&startCoordinate, sizeof(startCoordinate));
        D3D11_TILE_REGION_SIZE regionSize;
        ZeroMemory(&regionSize, sizeof(regionSize));
        regionSize.NumTiles = 1;
        UINT rangeFlags = D3D11_TILE_RANGE_REUSE_SINGLE_TILE;
        m_defaultTileIndex = m_reservedTiles++;
        DX::ThrowIfFailed(
            context->UpdateTileMappings(
                tempBuffer.Get(),
                1,
                &startCoordinate,
                &regionSize,
                m_tilePool.Get(),
                1,
                &rangeFlags,
                &m_defaultTileIndex,
                nullptr,
                0
                )
            );

        // Clear the tile to zeros.
        byte defaultTileData[SampleSettings::TileSizeInBytes];
        ZeroMemory(defaultTileData, sizeof(defaultTileData));
        context->UpdateTiles(tempBuffer.Get(), &startCoordinate, &regionSize, defaultTileData, 0);

        // Since the runtime doesn't know that all other tiled resources in this sample will point
        // to the data written via tempBuffer, insert a manual barrier to eliminate the hazard.
        context->TiledResourceBarrier(tempBuffer.Get(), NULL);

        // Map all tiles to the dummy physical tile.
        for (auto resource : m_managedResources)
        {
            regionSize.NumTiles = resource->totalTiles;
            DX::ThrowIfFailed(
                context->UpdateTileMappings(
                    resource->texture,
                    1,
                    &startCoordinate,
                    &regionSize,
                    m_tilePool.Get(),
                    1,
                    &rangeFlags,
                    &m_defaultTileIndex,
                    nullptr,
                    0
                    )
                );
        }
    }
}
*/

/*
ID3D11ShaderResourceView* ResidencyManager::ManageTexture(ID3D11Texture2D* texture, const std::wstring& filename)
{
    auto device = m_deviceResources->GetD3DDevice();

    auto resource = std::shared_ptr<ManagedTiledResource>(new ManagedTiledResource);

    resource->texture = texture;
    texture->GetDesc(&resource->textureDesc);
    UINT subresourceTilings = resource->textureDesc.MipLevels * resource->textureDesc.ArraySize;
    resource->subresourceTilings.resize(subresourceTilings);
    device->GetResourceTiling(
        texture,
        &resource->totalTiles,
        &resource->packedMipDesc,
        &resource->tileShape,
        &subresourceTilings,
        0,
        resource->subresourceTilings.data()
        );
    DX::ThrowIfFailed(subresourceTilings == resource->textureDesc.MipLevels * resource->textureDesc.ArraySize ? S_OK : E_UNEXPECTED);

    resource->loader.reset(new TileLoader(filename, &resource->subresourceTilings));

    // Create the residency texture.
    D3D11_TEXTURE2D_DESC textureDesc;
    ZeroMemory(&textureDesc, sizeof(textureDesc));
    textureDesc.Width = max(resource->subresourceTilings[0].WidthInTiles, resource->subresourceTilings[0].HeightInTiles);
    textureDesc.Height = textureDesc.Width;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 6;
    textureDesc.Format = DXGI_FORMAT_R8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    textureDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
    DX::ThrowIfFailed(device->CreateTexture2D(&textureDesc, nullptr, &resource->residencyTexture));

    // Create the shader resource view that will be used by both the terrain renderer and the visualizer.
    D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
    ZeroMemory(&shaderResourceViewDesc, sizeof(shaderResourceViewDesc));
    shaderResourceViewDesc.Format = textureDesc.Format;
    shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
    shaderResourceViewDesc.TextureCube.MipLevels = 1;
    DX::ThrowIfFailed(device->CreateShaderResourceView(resource->residencyTexture.Get(), &shaderResourceViewDesc, &resource->residencyTextureView));

    // Allocate space for the saved residency data.
    for (int face = 0; face < 6; face++)
    {
        resource->residency[face].resize(resource->subresourceTilings[0].WidthInTiles * resource->subresourceTilings[0].HeightInTiles, 0xFF);
    }

    m_managedResources.push_back(resource);

    // Return the residency view.
    return resource->residencyTextureView.Get();
}
*/