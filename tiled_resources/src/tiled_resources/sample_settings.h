//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved

#pragma once

namespace TiledResources
{
    namespace SampleSettings
    {
        namespace Controls
        {
            static const WPARAM Forward = 'W';
            static const WPARAM Left = 'A';
            static const WPARAM Back = 'S';
            static const WPARAM Right = 'D';
            static const WPARAM Up = VK_SPACE;
            static const WPARAM Down = VK_CONTROL;
            static const WPARAM RollLeft = 'Q';
            static const WPARAM RollRight = 'E';
            static const WPARAM ToggleDebug = VK_PAUSE;
            static const WPARAM ResetMappings = VK_DELETE;
            static const WPARAM ToggleBorder = 'T';
            static const WPARAM ToggleLodLimit = VK_TAB;
        }
        namespace CameraDynamics
        {
            static const float TranslationSpeed = 1.0f;
            static const float RotationSpeed = 60.0f * DirectX::XM_PI / 180.0f;
            static const float TransientRotationMultiplier = 1.0f / 500.0f;
        }
        namespace TileResidency
        {
            static const unsigned int PoolSizeInTiles = 256;
            static const unsigned int MaxSimultaneousFileLoadTasks = 10;
            static const unsigned int MaxTilesLoadedPerFrame = 100;
        }
        namespace TerrainAssets
        {
            namespace Diffuse
            {
                static const unsigned int DimensionSize = 16384;
                static const DXGI_FORMAT Format = DXGI_FORMAT_BC1_UNORM;
                static const unsigned int UnpackedMipCount = 6; // Set to Log2(DimensionSize / Standard Width of Format) + 1.
            }
            namespace Normal
            {
                static const unsigned int DimensionSize = 16384;
                static const DXGI_FORMAT Format =  DXGI_FORMAT_BC5_SNORM;
                static const unsigned int UnpackedMipCount = 7; // Set to Log2(DimensionSize / Standard Width of Format) + 1.
            }
        }
        namespace Sampling
        {
            static const float Ratio = 8.0f; // Ratio of screen size to sample target size.
            static const unsigned int SamplesPerFrame = 100;
        }
        static const UINT TileSizeInBytes = 0x10000; // Tiles are always 65536 Bytes.
    }
}
