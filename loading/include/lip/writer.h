#pragma once

#include <cstdint>

namespace uc
{
    namespace lip
    {
        template <typename write_interface> struct writer
        {
            public:

            writer( write_interface* i ) : m_interface(i), m_position(0)
            {

            }

            template <typename t > void write(t c)
            {
                m_interface->write<t>(c);
                m_position += sizeof(c);
            }

            void write(const void* bytes, size_t size)
            {
                m_interface->write(bytes, size);
                m_position += size;
            }

            void	reset()
            {
                m_position = 0;
            }

            size_t position() const
            {
                return m_position;
            }

            void	align( uint32_t alignment )
            {
            
            }

            write_interface* m_interface;
            size_t           m_position;
        };

        template <typename write_interface> void align(writer< write_interface>& writer, size_t alignment)
        {
            const char fill = 0x00;

            auto pos = writer.position();
            auto aligned_position = ((pos + alignment - 1) / alignment) * alignment;// &~(alignment - 1);
            auto padding = aligned_position - pos;

            for (std::size_t i = 0; i<padding; ++i)
            {
                writer.write(fill);
            }
        }

        template <typename write_interface> writer<write_interface> make_writer(write_interface* i)
        {
            return writer<write_interface>(i);
        }

        template <typename write_interface> writer<write_interface> make_writer(write_interface& i)
        {
            return writer<write_interface>(&i);
        }
    }
}

