//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved

#pragma once

#include <d3d12.h>

#include "tile_loader.h"

namespace sample
{
    // Data and metadata for a tiled resource layer.
    struct ManagedTiledResource
    {
        D3D12_RESOURCE_DESC									m_textureDescription;
		D3D12_PACKED_MIP_INFO								m_packedMipDescription;
        D3D12_TILE_SHAPE									m_tileShape;
        std::vector<D3D12_SUBRESOURCE_TILING>				m_subresourceTilings;
        
		std::unique_ptr<TileLoader>							m_loader;						//loader of binary data
        std::vector<uint8_t>								m_residencyShadow[6];			//six cube faces, holding the mip level of every tile

		uint32_t											m_totalTiles;					//total tile in the virtual resource

		uint32_t ResidencyWidth()	const { return m_subresourceTilings[0].WidthInTiles;  }
		uint32_t ResidencyHeight()	const { return m_subresourceTilings[0].HeightInTiles; }

		winrt::com_ptr<ID3D12Resource1>						m_resource;						//reserved resource
		winrt::com_ptr<ID3D12Resource1>						m_residencyResource;			//loaded mip level for every tile
		winrt::com_ptr<ID3D12Resource1>						m_residencyResourceUpload[2];	//two heaps per frame, which are used to upload the 
    };

    // Unique identifier for a tile.
    struct TileKey
    {
        D3D12_TILED_RESOURCE_COORDINATE m_coordinate;
		ID3D12Resource1*				m_resource;
    };

    // Define the < relational operator for use as a key in std::map.
    inline bool operator <(const TileKey & a, const TileKey & b)
    {
        if (a.m_resource < b.m_resource) return true;
        if (a.m_resource > b.m_resource) return false;
        if (a.m_coordinate.Subresource < b.m_coordinate.Subresource) return true;
        if (a.m_coordinate.Subresource > b.m_coordinate.Subresource) return false;
        if (a.m_coordinate.Z < b.m_coordinate.Z) return true;
        if (a.m_coordinate.Z > b.m_coordinate.Z) return false;
        if (a.m_coordinate.Y < b.m_coordinate.Y) return true;
        if (a.m_coordinate.Y > b.m_coordinate.Y) return false;
        return a.m_coordinate.X < b.m_coordinate.X;
    }

    enum class TileState
    {
        Seen,
        Loading,
        Loaded,
        Mapped
    };

    struct TrackedTile
    {
        ManagedTiledResource*				m_managedResource;
        D3D12_TILED_RESOURCE_COORDINATE		m_coordinate;
        uint16_t							m_mipLevel;
        uint16_t							m_face;
        uint32_t							m_physicalTileOffset;
        unsigned int						m_lastSeen;
        std::vector<uint8_t>				m_tileData;
        TileState							m_state;
    };

	struct ResidencyManagerCreateContext
	{
		ID3D12Device1* m_device;

		ID3D12DescriptorHeap* m_shader_heap;
		uint32_t              m_shader_heap_index;    //free slot

		std::wstring		  m_diffuse;
		std::wstring		  m_normal;
	};

	struct ResidencyManagerCreateResult
	{
		uint32_t m_diffuse_srv;
		uint32_t m_diffuse_residency_srv;

		uint32_t m_normal_srv;
		uint32_t m_normal_residency_srv;
	};

    class ResidencyManager
    {
		public:

        ResidencyManager(ID3D12Device1* d);

		ResidencyManagerCreateResult CreateResidencyManager(const ResidencyManagerCreateContext& ctx);

		void UpdateTiles(ID3D12GraphicsCommandList* list, uint32_t frame_index );
		void ResetInitialData(ID3D12GraphicsCommandList* list, uint32_t frame_index);

	private:

		std::unique_ptr<ManagedTiledResource>	m_resources[2];

        // Set of resources managed by this class.

        // Tiled Resource tile pool.
        //Microsoft::WRL::ComPtr<ID3D11Buffer> m_tilePool;

        // Map of all tracked tiles.
        std::map<TileKey, std::unique_ptr<TrackedTile> >	m_trackedTiles;

        // List of seen tiles ready for loading.
        std::list<std::unique_ptr<TrackedTile>>				m_seenTileList;

        // List of loading and loaded tiles.
        std::list<std::unique_ptr<TrackedTile>>				m_loadingTileList;

        // List of mapped tiles.
        std::list<std::unique_ptr<TrackedTile>>				m_mappedTileList;

        volatile LONG m_activeTileLoadingOperations;

        uint32_t m_reservedTiles;
		uint32_t m_defaultTileIndex;

		winrt::com_ptr<ID3D12Resource1> m_upload_heap[2];   //to upload new tiles to the gpu, one per frame, must be able to have memory for all our uploads
		winrt::com_ptr<ID3D12Heap>		m_physical_heap;	//to backup the reserved resources;
    };

	static bool LoadPredicate(const std::unique_ptr<TrackedTile>& a, const std::unique_ptr<TrackedTile>& b)
    {
        // Prefer more recently seen tiles.
        if (a->m_lastSeen > b->m_lastSeen) return true;
        if (a->m_lastSeen < b->m_lastSeen) return false;

        // Break ties by loading less detailed tiles first.
        return a->m_mipLevel > b->m_mipLevel;
    }

	static bool MapPredicate(const std::unique_ptr<TrackedTile>& a, const std::unique_ptr<TrackedTile>& b)
    {
        // Only loaded tiles can be mapped, so put those first.
        if (a->m_state == TileState::Loaded && b->m_state == TileState::Loading) return true;
        if (a->m_state == TileState::Loading && b->m_state == TileState::Loaded) return false;

        // Then prefer more recently seen tiles.
        if (a->m_lastSeen > b->m_lastSeen) return true;
        if (a->m_lastSeen < b->m_lastSeen) return false;

        // Break ties by mapping less detailed tiles first.
        return a->m_mipLevel > b->m_mipLevel;
    }

	static bool EvictPredicate(const std::unique_ptr<TrackedTile>& a, const std::unique_ptr<TrackedTile>& b)
    {
        // Evict older tiles first.
        if (a->m_lastSeen < b->m_lastSeen) return true;
        if (a->m_lastSeen > b->m_lastSeen) return false;

        // To break ties, evict more detailed tiles first.
        return a->m_mipLevel < b->m_mipLevel;
    }
}
