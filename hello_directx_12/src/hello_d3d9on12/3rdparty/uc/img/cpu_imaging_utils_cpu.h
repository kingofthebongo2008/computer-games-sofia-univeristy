#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace uc
{
    namespace gx
    {
        namespace imaging
        {
            class cpu_texture_storage
            {

            public:

                class storage_proxy
                {

                public:

                    storage_proxy(uint8_t* pixels) : m_pixels(pixels)
                    {

                    }

                    uint8_t* get_pixels_cpu() const
                    {
                        return m_pixels;
                    }

                private:

                    uint8_t* m_pixels;
                };

                class read_only_storage_proxy
                {

                public:

                    read_only_storage_proxy(const uint8_t* pixels) : m_pixels(pixels)
                    {

                    }

                    const uint8_t* get_pixels_cpu() const
                    {
                        return m_pixels;
                    }

                private:

                    const uint8_t* m_pixels;
                };

                cpu_texture_storage( const std::vector< uint8_t >& pixels ) :
                    m_pixels(pixels)
                {

                }

                cpu_texture_storage(std::vector< uint8_t >&& pixels) :
                    m_pixels(std::move(pixels) )
                {

                }

                cpu_texture_storage() = default;
                cpu_texture_storage(cpu_texture_storage&& o) = default;
                cpu_texture_storage(const cpu_texture_storage& o) = default;
                ~cpu_texture_storage() = default;

                cpu_texture_storage& operator=(cpu_texture_storage&&) = default;
                cpu_texture_storage& operator=(const cpu_texture_storage&) = default;
    
                storage_proxy  pixels()
                {
                    return storage_proxy(&m_pixels[0]);
                }

                read_only_storage_proxy  pixels() const
                {
                    return read_only_storage_proxy(&m_pixels[0]);
                }

            private:

                std::vector< uint8_t > m_pixels;
            };
        }
    }
}
