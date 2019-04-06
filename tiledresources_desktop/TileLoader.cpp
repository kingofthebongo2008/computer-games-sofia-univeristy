//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved

#include "pch.h"
#include "DirectXHelper.h"
#include "SampleSettings.h"
#include "ResidencyManager.h"
#include "TileLoader.h"

using namespace TiledResources;

using namespace concurrency;

#define ADD_TILE_BORDERS 1

TileLoader::TileLoader(const std::wstring & filename, std::vector<D3D11_SUBRESOURCE_TILING>* tilingInfo, bool border) :
    m_filename(filename),
    m_tilingInfo(tilingInfo),
    m_border(border)
{
    CREATEFILE2_EXTENDED_PARAMETERS parameters = { 0 };
    parameters.dwSize = sizeof(parameters);
    parameters.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
    parameters.dwFileFlags = FILE_FLAG_OVERLAPPED | FILE_FLAG_RANDOM_ACCESS;
    parameters.dwSecurityQosFlags = SECURITY_ANONYMOUS;
    parameters.lpSecurityAttributes = nullptr;
    parameters.hTemplateFile = nullptr;
    HANDLE fileHandle = CreateFile2(
        filename.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        OPEN_EXISTING,
        &parameters
        );
    assert(fileHandle != INVALID_HANDLE_VALUE);
    m_fileHandle.Attach(fileHandle);

    BY_HANDLE_FILE_INFORMATION info;
    GetFileInformationByHandle(m_fileHandle.Get(), &info);
    m_fileSize = ((unsigned long long) info.nFileSizeHigh << 32ULL) + info.nFileSizeLow;

    UINT numTilesPerFace = static_cast<UINT>((m_fileSize / SampleSettings::TileSizeInBytes) / 6);
    UINT tilesForSingleFaceMostDetailedMip = tilingInfo->at(1).StartTileIndexInOverallResource;
    m_subresourcesPerFaceInResource = static_cast<UINT>(tilingInfo->size() / 6);
    for (short face = 0; face < 6; face++)
    {
        UINT tileIndexInFace = 0;
        UINT tilesInSubresource = tilesForSingleFaceMostDetailedMip;
        m_subresourcesPerFaceInFile = 0;
        while (tileIndexInFace < numTilesPerFace)
        {
            size_t offset = (face * numTilesPerFace) + tileIndexInFace;
            m_subresourceTileOffsets.push_back(offset);
            tileIndexInFace += tilesInSubresource;
            tilesInSubresource = max(1, tilesInSubresource / 4);
            m_subresourcesPerFaceInFile++;
        }
    }
}

task<std::vector<byte>> TileLoader::LoadTileAsync(D3D11_TILED_RESOURCE_COORDINATE coordinate)
{
    UINT subresourceInFile = (coordinate.Subresource / m_subresourcesPerFaceInResource) * m_subresourcesPerFaceInFile + coordinate.Subresource % m_subresourcesPerFaceInResource;
    size_t fileOffset = (m_subresourceTileOffsets[subresourceInFile] + (coordinate.Y * m_tilingInfo->at(coordinate.Subresource).WidthInTiles + coordinate.X)) * SampleSettings::TileSizeInBytes;

    return create_task([this, fileOffset, subresourceInFile]()
    {
        std::vector<byte> tileData(0x10000);
        HANDLE readCompleteEvent = CreateEventEx(
            nullptr,
            nullptr,
            CREATE_EVENT_MANUAL_RESET,
            SYNCHRONIZE | EVENT_MODIFY_STATE
            );
        assert(readCompleteEvent != NULL);
        OVERLAPPED overlapped = { 0 };
        overlapped.Offset = (unsigned long long)fileOffset & 0xFFFFFFFFULL;
        overlapped.OffsetHigh = ((unsigned long long)fileOffset >> 32ULL) & 0xFFFFFFFFULL;
        overlapped.hEvent = readCompleteEvent;
        DWORD bytesRead = 0;
        BOOL result = ReadFile(
            m_fileHandle.Get(),
            tileData.data(),
            0x10000,
            &bytesRead,
            &overlapped
            );
        assert(result == TRUE || GetLastError() == ERROR_IO_PENDING);
        WaitForSingleObjectEx(readCompleteEvent, INFINITE, TRUE);

        if (m_border)
        {
            for (int blockRow = 0; blockRow < 256 / 4; blockRow++)
            {
                for (int blockCol = 0; blockCol < 512 / 4; blockCol++)
                {
                    int borderWidth = 1;// 1 << (2 - subresourceInFile % 15);
                    int colorSelect = (17 - subresourceInFile % 15);
                    byte* base = &tileData[(blockRow*(512 / 4) + blockCol) * 8];
                    if (blockRow < borderWidth || blockRow >= (256 / 4) - borderWidth || blockCol < borderWidth || blockCol >= (512 / 4) - borderWidth)
                    {
                        unsigned short block[4] =
                        {
                            0x001F,
                            0x0000,
                            0x0000,
                            0x0000
                        };
                        unsigned short colors[6] =
                        {
                            0xF800,
                            0xFFB0,
                            0x07B0,
                            0x07FF,
                            0x001F,
                            0xF81F
                        };
                        block[0] = colors[colorSelect % 6];
                        memcpy(base, block, sizeof(block));
                    }
                }
            }
        }

        return tileData;
    });
}
