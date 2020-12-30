#pragma once

#include <tuple>

#include <wrl/client.h>
#include <cstdint>

#include <wincodec.h>
#include "error.h"

namespace uc
{
    namespace gx
    {
        namespace imaging
        {
            typedef Microsoft::WRL::ComPtr<IWICImagingFactory>     wic_imaging_factory;
            typedef Microsoft::WRL::ComPtr<IWICStream>             wic_stream;
            typedef Microsoft::WRL::ComPtr<IWICBitmapDecoder>      wic_decoder;
            typedef Microsoft::WRL::ComPtr<IWICBitmapEncoder>      wic_encoder;
            typedef Microsoft::WRL::ComPtr<IWICBitmapFrameDecode>  wic_frame_decode;
            typedef Microsoft::WRL::ComPtr<IWICBitmapFrameEncode>  wic_frame_encode;
            typedef Microsoft::WRL::ComPtr<IWICBitmapSource>       wic_bitmap_source;
            typedef Microsoft::WRL::ComPtr<IWICComponentInfo>      wic_component_info;
            typedef Microsoft::WRL::ComPtr<IWICPixelFormatInfo>    wic_pixel_format_info;
            typedef Microsoft::WRL::ComPtr<IWICBitmapScaler>       wic_bitmap_scaler;
            typedef Microsoft::WRL::ComPtr<IWICFormatConverter>    wic_format_converter;

            typedef Microsoft::WRL::ComPtr<IPropertyBag2>          property_bag2;

            inline wic_imaging_factory create_factory()
            {
                wic_imaging_factory factory;
                throw_if_failed(CoCreateInstance(CLSID_WICImagingFactory2, nullptr, CLSCTX_INPROC_SERVER, __uuidof(IWICImagingFactory2), (LPVOID*)&factory));
                return factory;
            }

            inline wic_stream create_stream_reading(wic_imaging_factory f, const wchar_t* path)
            {
                wic_stream stream;

                throw_if_failed(f->CreateStream(&stream));
                throw_if_failed(stream->InitializeFromFilename(path, GENERIC_READ));

                return stream;
            }

            inline wic_stream create_stream_reading(wic_imaging_factory f, const uint8_t* memory, size_t memory_size)
            {
                wic_stream stream;

                throw_if_failed(f->CreateStream(&stream));
                throw_if_failed(stream->InitializeFromMemory(const_cast<BYTE*>(memory), static_cast<DWORD>(memory_size)));
                return stream;
            }


            inline wic_stream create_stream_writing(wic_imaging_factory f, const wchar_t* path)
            {
                wic_stream stream;

                throw_if_failed(f->CreateStream(&stream));
                throw_if_failed(stream->InitializeFromFilename(path, GENERIC_WRITE));

                return stream;
            }

            inline wic_decoder create_decoder_reading(wic_imaging_factory f, wic_stream s)
            {
                wic_decoder decoder;
                throw_if_failed(f->CreateDecoderFromStream(s.Get(), 0, WICDecodeMetadataCacheOnDemand, &decoder));
                return decoder;
            }

            inline wic_encoder create_encoder_writing(wic_imaging_factory f, wic_stream s)
            {
                wic_encoder encoder;
                throw_if_failed(f->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder));
                throw_if_failed(encoder->Initialize(s.Get(), WICBitmapEncoderNoCache));

                return encoder;
            }

            inline wic_bitmap_scaler create_bitmap_scaler(wic_imaging_factory f, wic_bitmap_source frame, uint32_t width, uint32_t height)
            {
                wic_bitmap_scaler scaler;

                throw_if_failed(f->CreateBitmapScaler(&scaler));

                throw_if_failed(scaler->Initialize(frame.Get(), width, height, WICBitmapInterpolationModeFant));

                return scaler;
            }


            inline wic_format_converter create_format_converter(wic_imaging_factory f, wic_bitmap_scaler scaler, const GUID& src, const GUID& dst)
            {
                wic_format_converter converter;

                throw_if_failed(f->CreateFormatConverter(&converter));
                BOOL can_convert = FALSE;

                throw_if_failed(converter->CanConvert(src, dst, &can_convert));
                if (!can_convert)
                {
                    throw_if_failed(E_FAIL);
                }

                throw_if_failed(converter->Initialize(scaler.Get(), dst, WICBitmapDitherTypeErrorDiffusion, 0, 0, WICBitmapPaletteTypeCustom));

                return converter;

            }

            inline wic_frame_decode create_decode_frame(wic_decoder decoder)
            {
                wic_frame_decode frame;

                throw_if_failed(decoder->GetFrame(0, &frame));

                return frame;
            }

            inline wic_frame_encode create_encode_frame(wic_encoder encoder)
            {
                wic_frame_encode frame;
                property_bag2 bag;

                throw_if_failed(encoder->CreateNewFrame(&frame, &bag));
                throw_if_failed(frame->Initialize(bag.Get()));

                return frame;
            }

            inline wic_component_info create_component_info(wic_imaging_factory factory, REFGUID targetGuid)
            {
                wic_component_info r;

                throw_if_failed(factory->CreateComponentInfo(targetGuid, &r));

                return r;
            }

            inline size_t wic_bits_per_pixel(wic_imaging_factory factory, REFGUID targetGuid)
            {
                auto info = create_component_info(factory, targetGuid);

                WICComponentType type;

                throw_if_failed(info->GetComponentType(&type));

                if (type != WICPixelFormat)
                    return 0;

                wic_pixel_format_info pixel_info;

                throw_if_failed( info.As<IWICPixelFormatInfo>(&pixel_info) );

                uint32_t bpp;
                throw_if_failed(pixel_info->GetBitsPerPixel(&bpp));
                return bpp;
            }

            class bitmap_source
            {
            public:
                explicit bitmap_source(wic_bitmap_source  source) : m_source(source)
                {

                }


                std::tuple < uint32_t, uint32_t> get_size() const
                {
                    uint32_t width;
                    uint32_t height;
                    throw_if_failed(m_source->GetSize(&width, &height));
                    return std::make_tuple(width, height);
                }

                WICPixelFormatGUID get_pixel_format() const
                {
                    WICPixelFormatGUID r;

                    throw_if_failed(m_source->GetPixelFormat(&r));
                    return r;
                }

                void copy_pixels(const WICRect *prc, uint32_t stride, uint32_t buffer_size, uint8_t* buffer)
                {
                    throw_if_failed(m_source->CopyPixels(prc, stride, buffer_size, buffer));
                }

            private:

                wic_bitmap_source m_source;
            };
        }
    }
}
