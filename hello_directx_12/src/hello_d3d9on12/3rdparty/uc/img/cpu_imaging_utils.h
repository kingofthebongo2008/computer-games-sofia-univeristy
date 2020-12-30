#pragma once

#include <cstdint>
#include <memory>
#include <tuple>


#include "cpu_imaging.h"
#include "cpu_imaging_utils_base.h"
#include "cpu_imaging_utils_cpu.h"

namespace uc
{
    namespace gx
    {
        namespace imaging
        {
            using cpu_texture =  texture < cpu_texture_storage >;

            template <typename image_t> image_t             wic_to_image_type(const WICPixelFormatGUID guid);
            template <typename image_t> WICPixelFormatGUID  image_type_to_wic(image_t image);

            inline image_type wic_to_image_type(const WICPixelFormatGUID guid)
            {
                struct WICTranslate
                {
                    GUID                wic;
                    image_type          format;
                };
                
                const WICTranslate g_WICFormats[] =
                {
                    { GUID_WICPixelFormat128bppRGBAFloat,       image_type::r32_g32_b32_a32_float },

                    { GUID_WICPixelFormat64bppRGBAHalf,         image_type::r16_g16_b16_a16_float },
                    { GUID_WICPixelFormat64bppRGBA,             image_type::r16_g16_b16_a16_unorm },

                    { GUID_WICPixelFormat32bppRGBA,             image_type::r8_g8_b8_a8_unorm },
                    { GUID_WICPixelFormat32bppBGRA,             image_type::b8_g8_r8_a8_unorm }, // DXGI 1.1
                    { GUID_WICPixelFormat32bppBGR,              image_type::b8_g8_r8_x8_unorm }, // DXGI 1.1

                    { GUID_WICPixelFormat32bppRGBA1010102XR,    image_type::r10_g10_b10_xr_bias_a2_unorm }, // DXGI 1.1
                    { GUID_WICPixelFormat32bppRGBA1010102,      image_type::r10_g10_b10_a2_unorm },

                    { GUID_WICPixelFormat16bppBGRA5551,         image_type::b5_g5_r5_a1_unorm },
                    { GUID_WICPixelFormat16bppBGR565,           image_type::b5_g6_r5_unorm },

                    { GUID_WICPixelFormat32bppGrayFloat,        image_type::r32_float },
                    { GUID_WICPixelFormat16bppGrayHalf,         image_type::r16_float },
                    { GUID_WICPixelFormat16bppGray,             image_type::r16_unorm },
                    { GUID_WICPixelFormat8bppGray,              image_type::r8_unorm },

                    { GUID_WICPixelFormat8bppAlpha,             image_type::a8_unorm },
                    { GUID_WICPixelFormat96bppRGBFloat,         image_type::r32_g32_b32_float },
                };

                for (size_t i = 0; i < _countof(g_WICFormats); ++i)
                {
                    if (g_WICFormats[i].wic == guid)
                    {
                        return g_WICFormats[i].format;
                    }
                }

                return image_type::unknown;
            }

            inline WICPixelFormatGUID image_type_to_wic(image_type image)
            {
                struct WICTranslate
                {
                    GUID                wic;
                    image_type          format;
                };

                const WICTranslate g_WICFormats[] =
                {
                    { GUID_WICPixelFormat128bppRGBAFloat,       image_type::r32_g32_b32_a32_float },

                    { GUID_WICPixelFormat64bppRGBAHalf,         image_type::r16_g16_b16_a16_float },
                    { GUID_WICPixelFormat64bppRGBA,             image_type::r16_g16_b16_a16_unorm },

                    { GUID_WICPixelFormat32bppRGBA,             image_type::r8_g8_b8_a8_unorm },
                    { GUID_WICPixelFormat32bppBGRA,             image_type::b8_g8_r8_a8_unorm }, // DXGI 1.1
                    { GUID_WICPixelFormat32bppBGR,              image_type::b8_g8_r8_x8_unorm }, // DXGI 1.1

                    { GUID_WICPixelFormat32bppRGBA1010102XR,    image_type::r10_g10_b10_xr_bias_a2_unorm }, // DXGI 1.1
                    { GUID_WICPixelFormat32bppRGBA1010102,      image_type::r10_g10_b10_a2_unorm },

                    { GUID_WICPixelFormat16bppBGRA5551,         image_type::b5_g5_r5_a1_unorm },
                    { GUID_WICPixelFormat16bppBGR565,           image_type::b5_g6_r5_unorm },

                    { GUID_WICPixelFormat32bppGrayFloat,        image_type::r32_float },
                    { GUID_WICPixelFormat16bppGrayHalf,         image_type::r16_float },
                    { GUID_WICPixelFormat16bppGray,             image_type::r16_unorm },
                    { GUID_WICPixelFormat8bppGray,              image_type::r8_unorm },

                    { GUID_WICPixelFormat8bppAlpha,             image_type::a8_unorm },
                    { GUID_WICPixelFormat96bppRGBFloat,         image_type::r32_g32_b32_float },
                };

                for (size_t i = 0; i < _countof(g_WICFormats); ++i)
                {
                    if (g_WICFormats[i].format == image)
                    {
                        return g_WICFormats[i].wic;
                    }
                }

                return GUID_WICPixelFormatUndefined;
            }

            inline WICPixelFormatGUID wic_to_wic(const WICPixelFormatGUID& guid)
            {
                struct WICConvert
                {
                    GUID        source;
                    GUID        target;
                };

                const WICConvert g_WICConvert[] =
                {
                    // Note target GUID in this conversion table must be one of those directly supported formats (above).
                    { GUID_WICPixelFormatBlackWhite,            GUID_WICPixelFormat8bppGray }, // DXGI_FORMAT_R8_UNORM

                    { GUID_WICPixelFormat1bppIndexed,           GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM 
                    { GUID_WICPixelFormat2bppIndexed,           GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM 
                    { GUID_WICPixelFormat4bppIndexed,           GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM 
                    { GUID_WICPixelFormat8bppIndexed,           GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM 

                    { GUID_WICPixelFormat2bppGray,              GUID_WICPixelFormat8bppGray }, // DXGI_FORMAT_R8_UNORM 
                    { GUID_WICPixelFormat4bppGray,              GUID_WICPixelFormat8bppGray }, // DXGI_FORMAT_R8_UNORM 

                    { GUID_WICPixelFormat16bppGrayFixedPoint,   GUID_WICPixelFormat16bppGrayHalf }, // DXGI_FORMAT_R16_FLOAT 
                    { GUID_WICPixelFormat32bppGrayFixedPoint,   GUID_WICPixelFormat32bppGrayFloat }, // DXGI_FORMAT_R32_FLOAT 

                    { GUID_WICPixelFormat16bppBGR555,           GUID_WICPixelFormat16bppBGRA5551 }, // DXGI_FORMAT_B5G5R5A1_UNORM

                    { GUID_WICPixelFormat32bppBGR101010,        GUID_WICPixelFormat32bppRGBA1010102 }, // DXGI_FORMAT_R10G10B10A2_UNORM

                    { GUID_WICPixelFormat24bppBGR,              GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM 
                    { GUID_WICPixelFormat24bppRGB,              GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM 
                    { GUID_WICPixelFormat32bppPBGRA,            GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM 
                    { GUID_WICPixelFormat32bppPRGBA,            GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM 

                    { GUID_WICPixelFormat48bppRGB,              GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
                    { GUID_WICPixelFormat48bppBGR,              GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
                    { GUID_WICPixelFormat64bppBGRA,             GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
                    { GUID_WICPixelFormat64bppPRGBA,            GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
                    { GUID_WICPixelFormat64bppPBGRA,            GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM

                    { GUID_WICPixelFormat48bppRGBFixedPoint,    GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT 
                    { GUID_WICPixelFormat48bppBGRFixedPoint,    GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT 
                    { GUID_WICPixelFormat64bppRGBAFixedPoint,   GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT 
                    { GUID_WICPixelFormat64bppBGRAFixedPoint,   GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT 
                    { GUID_WICPixelFormat64bppRGBFixedPoint,    GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT 
                    { GUID_WICPixelFormat64bppRGBHalf,          GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT 
                    { GUID_WICPixelFormat48bppRGBHalf,          GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT 

                    { GUID_WICPixelFormat128bppPRGBAFloat,      GUID_WICPixelFormat128bppRGBAFloat }, // DXGI_FORMAT_R32G32B32A32_FLOAT 
                    { GUID_WICPixelFormat128bppRGBFloat,        GUID_WICPixelFormat128bppRGBAFloat }, // DXGI_FORMAT_R32G32B32A32_FLOAT 
                    { GUID_WICPixelFormat128bppRGBAFixedPoint,  GUID_WICPixelFormat128bppRGBAFloat }, // DXGI_FORMAT_R32G32B32A32_FLOAT 
                    { GUID_WICPixelFormat128bppRGBFixedPoint,   GUID_WICPixelFormat128bppRGBAFloat }, // DXGI_FORMAT_R32G32B32A32_FLOAT 
                    { GUID_WICPixelFormat32bppRGBE,             GUID_WICPixelFormat128bppRGBAFloat }, // DXGI_FORMAT_R32G32B32A32_FLOAT 

                    { GUID_WICPixelFormat32bppCMYK,             GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM 
                    { GUID_WICPixelFormat64bppCMYK,             GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
                    { GUID_WICPixelFormat40bppCMYKAlpha,        GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
                    { GUID_WICPixelFormat80bppCMYKAlpha,        GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM

                    { GUID_WICPixelFormat32bppRGB,              GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM
                    { GUID_WICPixelFormat64bppRGB,              GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
                    { GUID_WICPixelFormat64bppPRGBAHalf,        GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT 
                };

                for (size_t i = 0; i < _countof(g_WICConvert); ++i)
                {
                    if (g_WICConvert[i].source == guid)
                    {
                        return g_WICConvert[i].target;
                    }
                }

                return GUID_WICPixelFormat32bppRGBA;
            }

            struct bitmap_source_tuple
            {
                imaging::bitmap_source      m_source;
                imaging::wic_frame_decode   m_frame;
            };

            inline bitmap_source_tuple create_bitmap_source(const wchar_t* url_path, imaging::wic_imaging_factory factory)
            {
                auto stream0    = imaging::create_stream_reading(factory, url_path);
                auto decoder0   = imaging::create_decoder_reading(factory, stream0);
                auto frame0     = imaging::create_decode_frame(decoder0);

                imaging::bitmap_source bitmap(frame0);

                bitmap_source_tuple r = { bitmap, frame0 };
                return r;
            }

            inline bitmap_source_tuple create_bitmap_source(const uint8_t* memory, size_t memory_size, imaging::wic_imaging_factory factory)
            {
                auto stream0    = imaging::create_stream_reading(factory, memory, memory_size);
                auto decoder0   = imaging::create_decoder_reading(factory, stream0);
                auto frame0     = imaging::create_decode_frame(decoder0);

                imaging::bitmap_source bitmap(frame0);

                bitmap_source_tuple r = { bitmap, frame0 };
                return r;
            }

            inline cpu_texture read_image( bitmap_source_tuple bitmap, imaging::wic_imaging_factory factory)
            {
                //auto format     = bitmap.m_source.get_pixel_format();
                auto size       = bitmap.m_source.get_size();

                auto bpp        = imaging::wic_bits_per_pixel(factory, bitmap.m_source.get_pixel_format() );
                auto row_pitch  = (bpp * std::get<0>(size) + 7) / 8;
                auto row_height = std::get<1>(size);
                auto image_size = row_pitch * row_height;

                WICPixelFormatGUID pixel_format;
                throw_if_failed(bitmap.m_frame->GetPixelFormat(&pixel_format));

                WICPixelFormatGUID convert_pixel_format = pixel_format;


                std::vector<uint8_t> temp;

                auto type = wic_to_image_type(pixel_format);

                if (type == image_type::unknown )
                {
                    convert_pixel_format = wic_to_wic(pixel_format);

                    type = wic_to_image_type(convert_pixel_format);

                    if (type == image_type::unknown)
                    {
                        throw_if_failed(E_FAIL);
                    }
                }

                if (convert_pixel_format != pixel_format)
                {
                    auto scaler = create_bitmap_scaler(factory, bitmap.m_frame, std::get<0>(size), std::get<1>(size));

                    if ( false)
                    {
                        bpp = imaging::wic_bits_per_pixel(factory, convert_pixel_format);
                        row_pitch = (bpp * std::get<0>(size) + 7) / 8;
                        row_height = std::get<1>(size);
                        image_size = row_pitch * row_height;
                        temp.resize(image_size);
                        throw_if_failed(scaler->CopyPixels(nullptr, static_cast<uint32_t>(row_pitch), static_cast<uint32_t>(image_size), &temp[0]));
                    }
                    else
                    {
                        bpp = imaging::wic_bits_per_pixel(factory, convert_pixel_format);
                        row_pitch = (bpp * std::get<0>(size) + 7) / 8;
                        row_height = std::get<1>(size);
                        image_size = row_pitch * row_height;


                        auto converter = create_format_converter(factory, scaler, pixel_format, convert_pixel_format);

                        temp.resize(image_size);
                        throw_if_failed(converter->CopyPixels(nullptr, static_cast<uint32_t>(row_pitch), static_cast<uint32_t>(image_size), &temp[0]));
                    }
                }
                else
                {
                    temp.resize(image_size);
                    bitmap.m_source.copy_pixels(nullptr, static_cast<uint32_t>(row_pitch), static_cast<uint32_t>(image_size), &temp[0]);
                }
                return cpu_texture(std::get<0>(size), std::get<1>(size), type, std::move(temp));
            }

            inline cpu_texture read_image(const wchar_t* url_path)
            {
                auto factory = imaging::create_factory();
                return read_image(create_bitmap_source(url_path, factory), factory);
            }

            inline cpu_texture read_image(const uint8_t* memory, size_t memory_size)
            {
                auto factory = imaging::create_factory();
                return read_image(create_bitmap_source(memory, memory_size, factory), factory);
            }

            template <typename texture > inline void write_image(const texture& t, const wchar_t* url_path)
            {
                auto factory = imaging::create_factory();
                auto stream0 = imaging::create_stream_writing(factory, url_path);
                auto encoder0 = imaging::create_encoder_writing(factory, stream0);
                auto frame0 = imaging::create_encode_frame(encoder0);

                throw_if_failed(frame0->SetSize(t.width(), t.height()));

                WICPixelFormatGUID formatGUID = image_type_to_wic( t.type() );

                throw_if_failed(frame0->SetPixelFormat(&formatGUID));

                auto proxy = t.pixels();

                throw_if_failed(frame0->WritePixels(t.height(), t.row_pitch(), static_cast<uint32_t>(t.size()), const_cast<uint8_t*>(proxy.get_pixels_cpu())));
                throw_if_failed(frame0->Commit());
                throw_if_failed(encoder0->Commit());
            }

            inline cpu_texture make_image(uint32_t width, uint32_t height, image_type t)
            {
                auto pitch           = get_row_pitch(t, width);
                auto size            = height * pitch;
                std::vector<uint8_t> pixels(size);

                return cpu_texture(width, height, t, std::move(pixels));
            }
        }
    }
}
