#pragma once

#include <cstdint>
#include <vector>

namespace uc
{
    namespace lip
    {
        struct memory_writer
        {
            public:

            memory_writer( )
            {

            }

            template <typename t > void write(t c)
            {
                write(&c, sizeof(t));
            }

            void write( const void* bytes, size_t size )    //todo: gsl span
            {
                auto s = m_bytes.size();
                m_bytes.resize(m_bytes.size() + size);
                auto data = reinterpret_cast<const uint8_t*> ( bytes );
                std::memcpy(&m_bytes[s], bytes, size);
            }

            std::vector<uint8_t> m_bytes;
        };

    }
}

