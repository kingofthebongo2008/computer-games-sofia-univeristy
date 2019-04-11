//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved

#include "pch.h"
#include "tile_loader.h"
#include "sample_settings.h"

namespace sample
{
	//using namespace concurrency;
	//using namespace std::experimental;

	namespace
	{
		IAsyncOperation< IRandomAccessStream > OpenStreamAsync(const std::wstring& filename)
		{
			auto folder = Windows::ApplicationModel::Package::Current().InstalledLocation();
			auto file	= co_await folder.GetFileAsync(filename.c_str());
			auto stream = co_await file.OpenReadAsync();
			return stream;
		}
	}

	TileLoader::TileLoader(const std::wstring& filename, std::vector<D3D12_SUBRESOURCE_TILING>* tilingInfo) :
		m_filename(filename)
		, m_tilingInfo(tilingInfo)
		, m_subresourcesPerFaceInResource( 0 )
		, m_subresourcesPerFaceInFile ( 0 )
	{
		m_openStream = OpenStreamPrivate(m_filename, m_tilingInfo);
	}

	IAsyncOperation< IRandomAccessStream >	 TileLoader::OpenStreamPrivate(const std::wstring& s, std::vector<D3D12_SUBRESOURCE_TILING>* tilingInfo )
	{
		auto stream = co_await OpenStreamAsync(s);

		m_subresourcesPerFaceInResource				= static_cast<uint32_t>(tilingInfo->size() / 6 );
		uint32_t tilesForSingleFaceMostDetailedMip	= tilingInfo->at(1).StartTileIndexInOverallResource;
		uint32_t tilesPerFace						= static_cast<uint32_t>( (stream.Size() / SampleSettings::TileSizeInBytes) / 6 );
		
		//Setup offsets
		for (uint32_t face = 0; face < 6; face++)
		{
			uint32_t tileIndexInFace		= 0;
			uint32_t tilesInSubresource		= tilesForSingleFaceMostDetailedMip;
			m_subresourcesPerFaceInFile		= 0;
			
			while ( tileIndexInFace < tilesPerFace )
			{
				uint32_t offset		= ( face * tilesPerFace) + tileIndexInFace;

				m_subresourceTileOffsets.push_back(offset);
				tileIndexInFace		+= tilesInSubresource;
				tilesInSubresource	= std::max(1U, tilesInSubresource / 4);
				m_subresourcesPerFaceInFile++;
			}
		}

		return stream;
	}

	task<std::vector<byte>> TileLoader::LoadTileAsync(D3D12_TILED_RESOURCE_COORDINATE coordinate)
	{
		uint32_t subresourceInFile = (coordinate.Subresource / m_subresourcesPerFaceInResource ) * m_subresourcesPerFaceInFile + coordinate.Subresource % m_subresourcesPerFaceInResource;
		size_t fileOffset		   = (m_subresourceTileOffsets[subresourceInFile] + (coordinate.Y * m_tilingInfo->at(coordinate.Subresource).WidthInTiles + coordinate.X) ) * SampleSettings::TileSizeInBytes;

		auto offset		= fileOffset;
		auto stream		= co_await m_openStream;
		auto reader		= DataReader(stream.GetInputStreamAt(offset));
		auto bytesRead	= co_await reader.LoadAsync(SampleSettings::TileSizeInBytes);

		std::vector<byte> tileData(SampleSettings::TileSizeInBytes);
		winrt::array_view<uint8_t> view(tileData);
		reader.ReadBytes(view);
		co_return std::move(tileData);
	}
}
