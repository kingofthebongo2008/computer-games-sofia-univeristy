#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <dxgi1_5.h>


namespace uc
{
    namespace gx
    {
        namespace imaging
        {
            enum class image_type : int32_t
            {
                //map to dxgi_formats
                r32_g32_b32_a32_float        = DXGI_FORMAT_R32G32B32A32_FLOAT,
                r16_g16_b16_a16_float        = DXGI_FORMAT_R16G16B16A16_FLOAT,
                r16_g16_b16_a16_unorm        = DXGI_FORMAT_R16G16B16A16_UNORM,
                r8_g8_b8_a8_unorm            = DXGI_FORMAT_R8G8B8A8_UNORM,
                
                b8_g8_r8_a8_unorm            = DXGI_FORMAT_B8G8R8A8_UNORM,
                b8_g8_r8_x8_unorm            = DXGI_FORMAT_B8G8R8X8_UNORM,


                r10_g10_b10_xr_bias_a2_unorm = DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM,
                r10_g10_b10_a2_unorm         = DXGI_FORMAT_R10G10B10A2_UNORM,

                b5_g5_r5_a1_unorm            = DXGI_FORMAT_B5G5R5A1_UNORM,
                b5_g6_r5_unorm               = DXGI_FORMAT_B5G6R5_UNORM,

                r32_float                    = DXGI_FORMAT_R32_FLOAT,
                r16_float                    = DXGI_FORMAT_R16_FLOAT,
                r16_unorm                    = DXGI_FORMAT_R16_UNORM,
                r8_unorm                     = DXGI_FORMAT_R8_UNORM,

                a8_unorm                     = DXGI_FORMAT_A8_UNORM,

                r32_g32_b32_float            = DXGI_FORMAT_R32G32B32_FLOAT,

                unknown                      = DXGI_FORMAT_UNKNOWN
            };

            template <image_type> inline uint32_t get_bpp();

            template <> inline uint32_t get_bpp< image_type::r32_g32_b32_a32_float>()
            {
                return 128;
            }

            template <> inline uint32_t get_bpp< image_type::r16_g16_b16_a16_float>()
            {
                return 64;
            }

            template <> inline uint32_t get_bpp< image_type::r16_g16_b16_a16_unorm>()
            {
                return 64;
            }

            template <> inline uint32_t get_bpp< image_type::r8_g8_b8_a8_unorm>()
            {
                return 32;
            }

            template <> inline uint32_t get_bpp< image_type::b8_g8_r8_a8_unorm>()
            {
                return 32;
            }

            template <> inline uint32_t get_bpp< image_type::b8_g8_r8_x8_unorm>()
            {
                return 32;
            }

            template <> inline uint32_t get_bpp< image_type::r10_g10_b10_xr_bias_a2_unorm>()
            {
                return 32;
            }

            template <> inline uint32_t get_bpp< image_type::r10_g10_b10_a2_unorm>()
            {
                return 32;
            }

            template <> inline uint32_t get_bpp< image_type::b5_g5_r5_a1_unorm>()
            {
                return 16;
            }

            template <> inline uint32_t get_bpp< image_type::b5_g6_r5_unorm>()
            {
                return 16;
            }

            template <> inline uint32_t get_bpp< image_type::r32_float>()
            {
                return 32;
            }

            template <> inline uint32_t get_bpp< image_type::r16_float>()
            {
                return 16;
            }

            template <> inline uint32_t get_bpp< image_type::r8_unorm>()
            {
                return 8;
            }

            template <> inline uint32_t get_bpp< image_type::a8_unorm>()
            {
                return 8;
            }

            template <> inline uint32_t get_bpp< image_type::r32_g32_b32_float>()
            {
                return 96;
            }

            template <image_type t> inline uint32_t get_row_pitch(uint32_t width)
            {
                return  (get_bpp<t>() * width + 7) / 8;
            }

            template <image_type t> inline uint32_t get_size(uint32_t width, uint32_t height)
            {
                return  get_row_pitch<t>(width) * height;
            }

            inline uint32_t get_bpp( const image_type t )
            {
                switch (t)
                {
                    case image_type::r32_g32_b32_a32_float: return 128;
                    case image_type::r16_g16_b16_a16_float: return 64;
                    case image_type::r16_g16_b16_a16_unorm: return 64;
                    case image_type::r8_g8_b8_a8_unorm:     return 32;
                    case image_type::b8_g8_r8_a8_unorm:     return 32;
                    case image_type::b8_g8_r8_x8_unorm:     return 32;
                    case image_type::r10_g10_b10_xr_bias_a2_unorm: return 32;
                    case image_type::r10_g10_b10_a2_unorm:  return 32;
                    case image_type::b5_g5_r5_a1_unorm:     return 16;
                    case image_type::b5_g6_r5_unorm:        return 16;
                    case image_type::r32_float:             return 32;
                    case image_type::r16_float:             return 16;
                    case image_type::r16_unorm:             return 16;
                    case image_type::r8_unorm:              return 8;
                    case image_type::a8_unorm:              return 8;
                    case image_type::r32_g32_b32_float:     return 96;
                    default: return 0;
                }
            }

            inline uint32_t get_row_pitch(const image_type t, uint32_t width)
            {
                return  (get_bpp(t) * width + 7) / 8;
            }

            inline uint32_t get_size(const image_type t, uint32_t width, uint32_t height)
            {
                return  get_row_pitch(t,width) * height;
            }

            template <typename pixels_storage>
            class texture : public pixels_storage
            {
                private:

                std::vector<uint8_t> make_pixels(uint32_t size)
                {
                    std::vector<uint8_t> r;
                    r.resize(size);
                    return r;
                }

                public:
                texture(uint32_t width, uint32_t height, image_type type) :
                    pixels_storage( make_pixels( get_size( type, width, height)))
                    , m_image_type(type)
                    , m_width(width)
                    , m_height(height)

                {

                }

                texture(uint32_t width, uint32_t height, image_type type, std::vector<uint8_t>&& pixels) :
                      pixels_storage(std::move(pixels))
                    , m_image_type(type)
                    , m_width(width)
                    , m_height(height)

                {

                }

                texture(uint32_t width, uint32_t height, image_type type, const std::vector<uint8_t>& pixels) :
                    pixels_storage(pixels)
                    , m_image_type(type)
                    , m_width(width)
                    , m_height(height)

                {

                }

                texture() = default;
                texture(texture&& o) = default;
                texture(const texture& o) = default;
                ~texture() = default;

                texture& operator=(texture&&) = default;
                texture& operator=(const texture&) = default;

                uint32_t width() const
                {
                    return m_width;
                }

                uint32_t height() const
                {
                    return m_height;
                }

                size_t   bpp() const
                {
                    return get_bpp(m_image_type);
                }

                uint32_t row_pitch() const
                {
                    return get_row_pitch(m_image_type, width());
                }

                uint32_t slice_pitch() const
                {
                    return height();
                }

                image_type type() const
                {
                    return m_image_type;
                }

                size_t size() const
                {
                    return get_size(m_image_type, width(), height());
                }

                private:
                image_type  m_image_type;
                uint32_t    m_width;
                uint32_t    m_height;
            };
        }
    }
}
