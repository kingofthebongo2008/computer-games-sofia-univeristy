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
#include "ppl.h"
#include <map>

namespace sample
{
	static winrt::com_ptr<ID3D12Resource1 > CreateReservedTexture(ID3D12Device1* device, uint32_t width, uint32_t height, DXGI_FORMAT f, uint32_t mipLevels)
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

	static winrt::com_ptr<ID3D12Resource1 > CreateReservedNullBuffer(ID3D12Device1* device)
	{
		D3D12_RESOURCE_DESC desc = {};
		desc.Alignment = 0;
		desc.DepthOrArraySize = 1;
		desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;
		desc.Format = DXGI_FORMAT_R8_UINT;
		desc.Height = 256;
		desc.Layout = D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE;
		desc.MipLevels = 1;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Width = 256;

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
		D3D12_RESOURCE_DESC d = DescribeBuffer(size);

		winrt::com_ptr<ID3D12Resource1>     r;
		D3D12_HEAP_PROPERTIES p = {};
		p.Type = D3D12_HEAP_TYPE_UPLOAD;
		D3D12_RESOURCE_STATES       state = D3D12_RESOURCE_STATE_GENERIC_READ;

		ThrowIfFailed(device->CreateCommittedResource(&p, D3D12_HEAP_FLAG_NONE, &d, state, nullptr, __uuidof(ID3D12Resource1), r.put_void()));
		return r;
	}

	static D3D12_HEAP_DESC DescribeHeap(uint32_t size)
	{
		D3D12_HEAP_DESC d = {};
		D3D12_HEAP_PROPERTIES p = {};
		p.Type = D3D12_HEAP_TYPE_DEFAULT;

		d.Flags = D3D12_HEAP_FLAG_DENY_BUFFERS | D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES;	//only textures here;
		d.Properties = p;
		d.SizeInBytes = size;

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
		using namespace SampleSettings;

		m_upload_heap[0] = CreateUploadResource(d, TileResidency::PoolSizeInTiles * TileSizeInBytes);
		m_upload_heap[1] = CreateUploadResource(d, TileResidency::PoolSizeInTiles * TileSizeInBytes);
		m_physical_heap = CreatePhysicalHeap(d, TileResidency::PoolSizeInTiles * TileSizeInBytes);

		m_null_resource = CreateReservedNullBuffer(d);
	}

	std::unique_ptr<ManagedTiledResource> MakeManagedResource(ID3D12Device1* d, winrt::com_ptr<ID3D12Resource1> resource, const std::wstring& filename)
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
		p->m_residencyResource = CreateResidency(d, p->ResidencyWidth(), p->ResidencyHeight());

		//create the upload heaps for residency texture, so we can upload it every frame if needed
		//one per frame, 

		uint64_t size = GetRequiredIntermediateSize(p->m_residencyResource.get(), 0, 6);

		p->m_residencyResourceUpload[0] = CreateUploadResource(d, size);
		p->m_residencyResourceUpload[1] = CreateUploadResource(d, size);

		return r;
	}

	std::unique_ptr<ManagedTiledResource> MakeDiffuseResource(ID3D12Device1* d, const std::wstring& filename)
	{
		using namespace SampleSettings::TerrainAssets;
		winrt::com_ptr<ID3D12Resource1> diffuse = CreateReservedTexture(d, Diffuse::DimensionSize, Diffuse::DimensionSize, Diffuse::Format, Diffuse::UnpackedMipCount);
		return MakeManagedResource(d, std::move(diffuse), filename);
	}

	std::unique_ptr<ManagedTiledResource> MakeNormalResource(ID3D12Device1* d, const std::wstring& filename)
	{
		using namespace SampleSettings::TerrainAssets;
		winrt::com_ptr<ID3D12Resource1> normal = CreateReservedTexture(d, Normal::DimensionSize, Normal::DimensionSize, Normal::Format, Normal::UnpackedMipCount);
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

		r.m_diffuse_srv = ctx.m_shader_heap_index;
		r.m_diffuse_residency_srv = ctx.m_shader_heap_index + 1;

		r.m_normal_srv = ctx.m_shader_heap_index + 2;
		r.m_normal_residency_srv = ctx.m_shader_heap_index + 3;

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

	void ResidencyManager::ResetInitialData(ID3D12CommandQueue * queue, ID3D12GraphicsCommandList * list, uint32_t frame_index)
	{
		//Upload all residency resources, face by face

		for (auto i = 0U; i < 2; ++i)
		{
			auto r = m_resources[i].get();

			D3D12_SUBRESOURCE_DATA data[6];
			for (auto i = 0U; i < 6; ++i)
			{
				data[i].pData = &r->m_residencyShadow[i][0];
				data[i].RowPitch = r->ResidencyWidth();
				data[i].SlicePitch = r->ResidencyHeight();
			}

			UpdateSubresources<6>(list, r->m_residencyResource.get(), r->m_residencyResourceUpload[frame_index].get(), 0, 0, 6, data);
		}

		//Update the null resource
		{
			auto r = m_null_resource.get();

			std::vector<uint8_t> buffer(SampleSettings::TileSizeInBytes, 0xAA);

			D3D12_SUBRESOURCE_DATA data[1] =
			{
				{
					&buffer[0],
					256,
					SampleSettings::TileSizeInBytes
				}
			};

			UpdateSubresources<1>(list, r, m_upload_heap[frame_index].get(), 0, 0, 1, data);
		}

		//Transtion resources, make them ready for sampling
		{
			D3D12_RESOURCE_BARRIER barrier[5] = {};

			for (auto i = 0U; i < 5; ++i)
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

			barrier[4].Transition.pResource = m_null_resource.get();

			list->ResourceBarrier(4, barrier);
		}

		for (auto i = 0U; i < 2; ++i)
		{
			auto r = m_resources[i]->m_resource;

			D3D12_TILE_RANGE_FLAGS flags[] = { D3D12_TILE_RANGE_FLAG_REUSE_SINGLE_TILE };
			uint32_t		       heap_offset[] = { 0 };
			uint32_t			   tile_counts[] = { 1 };
			queue->UpdateTileMappings(r.get(), 1, nullptr, nullptr, m_physical_heap.get(), 1, &flags[0], &heap_offset[0], nullptr, D3D12_TILE_MAPPING_FLAG_NONE);
		}

		//Map null tile to the same heap
		{
			auto r = m_null_resource.get();

			D3D12_TILE_RANGE_FLAGS flags[] = { D3D12_TILE_RANGE_FLAG_REUSE_SINGLE_TILE };
			uint32_t		       heap_offset[] = { 0 };
			uint32_t			   tile_counts[] = { 1 };
			queue->UpdateTileMappings(r, 1, nullptr, nullptr, m_physical_heap.get(), 1, &flags[0], &heap_offset[0], nullptr, D3D12_TILE_MAPPING_FLAG_NONE);
		}
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

	void ResidencyManager::ProcessSamples(const std::vector<DecodedSample> & samples, uint32_t frame_number)
	{
		if (!samples.empty())
		{
			for (auto&& sample : samples)
			{
				// Interpret each sample in the context of each managed resource.
				for (auto&& resource : m_resources)
				{

					// Samples are encoded assuming a maximally-sized (15-MIP) texture, so offset the
					// sampled value by the difference between actual MIPs and maximum MIPs.
					// Also, due to low-detail MIPs not being part of the MIP chain, we clamp the actual
					// MIP to guarantee it always hits at least one level represented in the tiled resource.
					int16_t				mips = static_cast<int16_t>(resource->m_textureDescription.MipLevels);
					int16_t actualMip = std::max<int16_t>(0, std::min<int16_t>(mips - 1, sample.mip));

					// Loop over sampled MIP through the least detailed MIP.
					for (auto mip = actualMip; mip < mips; ++mip)
					{
						// Calculate the tile coordinate.
						TileKey tileKey = {};
						tileKey.m_resource = resource->m_resource.get();

						uint32_t subResource = mip + sample.face * mips;

						const auto& tilings = resource->m_subresourceTilings[subResource];
						tileKey.m_coordinate.Subresource = subResource;

						float tileX = std::max<float>(tilings.WidthInTiles * sample.u, 0.0f);
						tileX = std::min<float>(tilings.WidthInTiles - 1.0f, tileX);

						tileKey.m_coordinate.X = static_cast<uint32_t>(tileX);
						float tileY = std::max<float>(tilings.HeightInTiles * sample.v, 0.0f);

						tileY = std::min<float>(tilings.HeightInTiles - 1.0f, tileY);
						tileKey.m_coordinate.Y = static_cast<uint32_t>(tileY);

						// See if the tile is already being tracked.
						auto tileIterator = m_trackedTiles.find(tileKey);
						if (tileIterator == m_trackedTiles.end())
						{
							// Tile is not being tracked currently, so enqueue it for load.
							std::unique_ptr<TrackedTile> tile(new TrackedTile);
							tile->m_managedResource = resource.get();
							tile->m_coordinate = tileKey.m_coordinate;
							tile->m_lastSeen = frame_number;
							tile->m_state = TileState::Seen;
							tile->m_mipLevel = mip;
							tile->m_face = sample.face;

							m_seenTileList.push_back(tile.get());
							m_trackedTiles[tileKey] = std::move(tile);

						}
						else
						{
							// If tile is already tracked, simply update the last-seen value.
							tileIterator->second->m_lastSeen = frame_number;
						}
					}
				}
			}
		}
	}

	void ResidencyManager::UpdateTiles(ID3D12CommandQueue* queue, ID3D12GraphicsCommandList* list, uint32_t frame_index, uint32_t frame_number, const std::vector<DecodedSample>& samples)
	{
		ProcessSamples(samples, frame_number);

		concurrency::task_group g;

		// Sort the tile lists.

		g.run([this]
		{
			m_seenTileList.sort(LoadPredicate);
		});

		g.run([this]
		{
			m_loadingTileList.sort(MapPredicate);
		});

		g.run_and_wait([this]
		{
			m_mappedTileList.sort(EvictPredicate);
		});


		// Initiate loads for seen tiles.
		for (auto i = m_active_tile_loading_operations.load(); i < SampleSettings::TileResidency::MaxSimultaneousFileLoadTasks; i++)
		{
			if (m_seenTileList.empty())
			{
				break;
			}

			TrackedTile* tileToLoad = m_seenTileList.front();

			m_seenTileList.pop_front();

			m_active_tile_loading_operations++;

			tileToLoad->m_managedResource->m_loader->LoadTileAsync(tileToLoad->m_coordinate).then([this, tileToLoad](std::vector<uint8_t> tileData)
			{
				tileToLoad->m_tileData = std::move(tileData);
				tileToLoad->m_state = TileState::Loaded;
				m_active_tile_loading_operations--;
			});

			// Move the tile to the loading list.
			m_loadingTileList.push_back(tileToLoad);
		}

		// Loop over the loading / loaded tile list for mapping candidates.
		struct TileMappingUpdateArguments
		{
			std::vector<D3D12_TILED_RESOURCE_COORDINATE> m_coordinates;

			std::vector<uint32_t>						 m_rangeFlags;
			std::vector<uint32_t>						 m_physicalOffsets;

			// For convenience, the tracked tile mapping is also saved.
			std::list<TrackedTile*>						 m_tilesToMap;
		};

		std::map<ID3D12Resource1*, TileMappingUpdateArguments> coalescedMapArguments;

		for (auto i = 0; i < SampleSettings::TileResidency::MaxTilesLoadedPerFrame; i++)
		{
			if (m_loadingTileList.empty())
			{
				break;
			}

			auto tileToMap = m_loadingTileList.front();

			if (tileToMap->m_state != TileState::Loaded)
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

				if (tileToMap->m_lastSeen < tileToEvict->m_lastSeen)
				{
					// If the candidate tile to map is older than the eviction candidate,
					// skip the mapping and discard it. This can occur if a tile load stalls,
					// and by the time it is ready it has moved off-screen.
					TileKey tileKey = { tileToMap->m_coordinate, tileToMap->m_managedResource->m_resource.get() };

					// Remove the tile from the tracked list.
					m_trackedTiles.erase(tileKey);

					//todo: assert the tile is not in the other lists


					// Move on to the next map candidate.
					continue;
				}

				m_mappedTileList.pop_front();

				// Save the physical tile that was freed so the new tile can use it.
				physicalTileOffset = tileToEvict->m_physicalTileOffset;

				TileKey tileKey = { tileToEvict->m_coordinate, tileToEvict->m_managedResource->m_resource.get() };

				// Add the new NULL-mapping to the argument list.
				coalescedMapArguments[tileToEvict->m_managedResource->m_resource.get()].m_coordinates.push_back(tileToEvict->m_coordinate);
				coalescedMapArguments[tileToEvict->m_managedResource->m_resource.get()].m_rangeFlags.push_back(D3D12_TILE_RANGE_FLAG_NULL);
				coalescedMapArguments[tileToEvict->m_managedResource->m_resource.get()].m_physicalOffsets.push_back(physicalTileOffset);

				// Update the residency map to remove this level of detail.
				int baseTilesCoveredWidth = tileToEvict->m_managedResource->m_subresourceTilings[0].WidthInTiles / tileToEvict->m_managedResource->m_subresourceTilings[tileToEvict->m_coordinate.Subresource].WidthInTiles;
				int baseTilesCoveredHeight = tileToEvict->m_managedResource->m_subresourceTilings[0].HeightInTiles / tileToEvict->m_managedResource->m_subresourceTilings[tileToEvict->m_coordinate.Subresource].HeightInTiles;


				for (int Y = 0; Y < baseTilesCoveredHeight; Y++)
				{
					for (int X = 0; X < baseTilesCoveredWidth; X++)
					{
						int tileY = tileToEvict->m_coordinate.Y * baseTilesCoveredHeight + Y;
						int tileX = tileToEvict->m_coordinate.X * baseTilesCoveredWidth + X;
						uint8_t* value = &tileToEvict->m_managedResource->m_residencyShadow[tileToEvict->m_face][tileY * tileToEvict->m_managedResource->m_subresourceTilings[0].WidthInTiles + tileX];
						*value = std::max<uint8_t>((tileToEvict->m_mipLevel + 1) * 16, *value);
					}
				}

				// Remove the tile from the tracked list.
				m_trackedTiles.erase(tileKey);
			}

			// Add the new mapping to the argument list.
			coalescedMapArguments[tileToMap->m_managedResource->m_resource.get()].m_coordinates.push_back(tileToMap->m_coordinate);
			coalescedMapArguments[tileToMap->m_managedResource->m_resource.get()].m_rangeFlags.push_back(0);
			coalescedMapArguments[tileToMap->m_managedResource->m_resource.get()].m_physicalOffsets.push_back(physicalTileOffset);
			tileToMap->m_physicalTileOffset = physicalTileOffset;
			tileToMap->m_state = TileState::Mapped;
			coalescedMapArguments[tileToMap->m_managedResource->m_resource.get()].m_tilesToMap.push_back(tileToMap);

			// Update the residency map to add this level of detail.
			int baseTilesCoveredWidth = tileToMap->m_managedResource->m_subresourceTilings[0].WidthInTiles / tileToMap->m_managedResource->m_subresourceTilings[tileToMap->m_coordinate.Subresource].WidthInTiles;
			int baseTilesCoveredHeight = tileToMap->m_managedResource->m_subresourceTilings[0].HeightInTiles / tileToMap->m_managedResource->m_subresourceTilings[tileToMap->m_coordinate.Subresource].HeightInTiles;
			for (int Y = 0; Y < baseTilesCoveredHeight; Y++)
			{
				for (int X = 0; X < baseTilesCoveredWidth; X++)
				{
					int tileY = tileToMap->m_coordinate.Y * baseTilesCoveredHeight + Y;
					int tileX = tileToMap->m_coordinate.X * baseTilesCoveredWidth + X;
					uint8_t* value = &tileToMap->m_managedResource->m_residencyShadow[tileToMap->m_face][tileY * tileToMap->m_managedResource->m_subresourceTilings[0].WidthInTiles + tileX];
					*value = std::min<uint8_t>(tileToMap->m_mipLevel * 16, *value);
				}
			}

			m_mappedTileList.push_back(tileToMap);
		}
	}
}
/*

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
	std::list<std::shared_ptr<TrackedTile>> tilesToMap;
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
			(UINT)perResourceArguments.second.coordinates.size(),
			perResourceArguments.second.coordinates.data(),
			sizes.data(),
			m_tilePool.Get(),
			(UINT)perResourceArguments.second.rangeFlags.size(),
			perResourceArguments.second.rangeFlags.data(),
			perResourceArguments.second.physicalOffsets.data(),
			rangeCounts.data(),
			0
		)
	);

	context->TiledResourceBarrier(NULL, perResourceArguments.first);
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
		context->TiledResourceBarrier(NULL, perResourceArguments.first);
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

*/