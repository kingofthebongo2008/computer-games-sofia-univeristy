#pragma once

#include <cstdint>
#include <vector>
#include <memory>


namespace uc
{
    namespace gx
    {
        namespace imaging
        {
            // Generate a simple black and white checkerboard texture.
            inline std::vector<uint8_t> checker_board_texture_vector( uint32_t width, uint32_t height, uint32_t bpp = 32)
            {
                const uint32_t row_pitch     = (width * bpp + 7) / 8 ;
                const uint32_t cell_pitch    = row_pitch >> 3;		// The width of a cell in the checkboard texture.
                const uint32_t cell_height   = width >> 3;	        // The height of a cell in the checkerboard texture.
                const uint32_t textureSize   = row_pitch * height;

                std::vector<uint8_t> data(textureSize);
                uint8_t*            pData = &data[0];

                for (uint32_t n = 0; n < textureSize; n += (bpp + 7) /8 )
                {
                    uint32_t x = n % row_pitch;
                    uint32_t y = n / row_pitch;
                    uint32_t i = x / cell_pitch;
                    uint32_t j = y / cell_height;

                    if (i % 2 == j % 2)
                    {
                        pData[n]     = 0x00;	// R
                        pData[n + 1] = 0x00;	// G
                        pData[n + 2] = 0x00;	// B
                        pData[n + 3] = 0xff;	// A
                    }
                    else
                    {
                        pData[n]     = 0xff;	// R
                        pData[n + 1] = 0xff;	// G
                        pData[n + 2] = 0xff;	// B
                        pData[n + 3] = 0xff;	// A
                    }
                }

                return data;
            }

            inline std::unique_ptr<uint8_t[]> checker_board_texture_array(uint32_t width, uint32_t height, uint32_t bpp = 32)
            {

                const uint32_t row_pitch = (width * bpp + 7) / 8;
                const uint32_t cell_pitch = row_pitch >> 3;		// The width of a cell in the checkboard texture.
                const uint32_t cell_height = width >> 3;	        // The height of a cell in the checkerboard texture.
                const uint32_t textureSize = row_pitch * height;

                std::unique_ptr<uint8_t[]> data(new uint8_t[textureSize]);

                uint8_t*            pData = &data[0];

                for (uint32_t n = 0; n < textureSize; n += (bpp + 7) / 8)
                {
                    uint32_t x = n % row_pitch;
                    uint32_t y = n / row_pitch;
                    uint32_t i = x / cell_pitch;
                    uint32_t j = y / cell_height;

                    if (i % 2 == j % 2)
                    {
                        pData[n] = 0x00;	    // R
                        pData[n + 1] = 0x00;	// G
                        pData[n + 2] = 0x00;	// B
                        pData[n + 3] = 0xff;	// A
                    }
                    else
                    {
                        pData[n] = 0xff;	// R
                        pData[n + 1] = 0xff;	// G
                        pData[n + 2] = 0xff;	// B
                        pData[n + 3] = 0xff;	// A
                    }
                }

                return data;
            }

            template <uint32_t> constexpr inline uint32_t get_byte(uint32_t v);

            template <> constexpr inline uint32_t get_byte<0>(uint32_t v)
            {
                return v & 0xff;
            }

            template <> constexpr inline uint32_t get_byte<1>(uint32_t v)
            {
                return (v >> 8) & 0xff;
            }

            template <> constexpr inline uint32_t get_byte<2>(uint32_t v)
            {
                return (v >> 16) & 0xff;
            }

            template <> constexpr inline uint32_t get_byte<3>(uint32_t v)
            {
                return (v >> 24) & 0xff;
            }

            template <uint32_t swizzle_mask > inline uint32_t swizzle( uint32_t v )
            {
                constexpr auto b0_mask = get_byte< 0 >(swizzle_mask);
                constexpr auto b1_mask = get_byte< 1 >(swizzle_mask);
                constexpr auto b2_mask = get_byte< 2 >(swizzle_mask);
                constexpr auto b3_mask = get_byte< 3 >(swizzle_mask);

                auto b0 = get_byte<b0_mask>(v);
                auto b1 = get_byte<b1_mask>(v);
                auto b2 = get_byte<b2_mask>(v);
                auto b3 = get_byte<b3_mask>(v);

                return (b3 << 24) | (b2 << 16) | (b1 << 8) | b0;
            }

            constexpr uint32_t swizzle_mask( uint32_t b0, uint32_t b1, uint32_t b2, uint32_t b3 )
            {
                return (b3 << 24) | (b2 << 16) | (b1 << 8) | b0;
            }

            enum swizle_masks : uint32_t
            {
                x_y_z_w = swizzle_mask(0, 1, 2, 3),
                z_y_x_w = swizzle_mask(2, 1, 0, 3),
                x_x_x_x = swizzle_mask(0, 0, 0, 0),
            };

            inline std::vector<uint8_t> bpp24_2_bpp32( uint32_t width, uint32_t height, const void* pixels )
            {
                const uint32_t bpp_d            = 32;
                const uint32_t bpp_s            = 24;
                const uint32_t d_pitch          = (width * bpp_d + 7) / 8;
                const uint32_t s_pitch          = (width * bpp_s + 7) / 8;
                const uint32_t texture_size     = d_pitch * height;

                std::vector<uint8_t> data(texture_size);

                const uint8_t* s = reinterpret_cast<const uint8_t* __restrict>(pixels);
                uint8_t* d       = reinterpret_cast<uint8_t* __restrict>(&data[0]);

                constexpr uint32_t mask = x_x_x_x;

                for (auto i = 0U; i < height; ++i)
                {
                    auto row_s = s + i * s_pitch;
                    auto row_d = d + i * d_pitch;

                    for (auto j = 0U; j < width; j++)
                    {
                        uint8_t b1 = *(row_s)++;
                        uint8_t b2 = *(row_s)++;
                        uint8_t b3 = *(row_s)++;

                        uint32_t value = (b3 << 16 ) | ( b2 << 8 ) | ( b1 ) ;// << 8);
                        
                        uint32_t swizzled = swizzle< mask >(value);

                        uint32_t* r = reinterpret_cast<uint32_t*>(row_d);
                        *r = swizzled;
                        row_d += 4;
                        
                        /*
                        *(row_d)++ = *(row_s)++;
                        *(row_d)++ = *(row_s)++;
                        *(row_d)++ = *(row_s)++;
                        *(row_d)++ = 0;
                        */
                    }
                }

                return data;
            }
            
        }
    }
}
