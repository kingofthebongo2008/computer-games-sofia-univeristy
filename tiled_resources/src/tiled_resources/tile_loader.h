//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved

#pragma once

#include <d3d12.h>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.Streams.h>

namespace sample
{
	using namespace winrt;
	using namespace winrt::Windows::Foundation;
	using namespace winrt::Windows::Foundation::Collections;
	using namespace winrt::Windows::Storage;
	using namespace winrt::Windows::Storage::Streams;
		
	using namespace std::experimental;
	using namespace Concurrency;

    class TileLoader
    {
		public:

        TileLoader(const std::wstring & filename, std::vector<D3D12_SUBRESOURCE_TILING>* tilingInfo);
        concurrency::task<std::vector<byte> > LoadTileAsync( D3D12_TILED_RESOURCE_COORDINATE coordinate );

    private:

		std::wstring							 m_filename;
		std::vector<D3D12_SUBRESOURCE_TILING>*	 m_tilingInfo = nullptr;
		IAsyncOperation<IRandomAccessStream>	 m_openStream;
		IRandomAccessStream						 m_openStreamData;
		
        std::vector<size_t>						 m_subresourceTileOffsets;
		uint32_t								 m_subresourcesPerFaceInResource = 0;
		uint32_t								 m_subresourcesPerFaceInFile = 0;

		IAsyncOperation< IRandomAccessStream >	 OpenStreamPrivate(const std::wstring& s, std::vector<D3D12_SUBRESOURCE_TILING>* tilingInfo);
    };
}
