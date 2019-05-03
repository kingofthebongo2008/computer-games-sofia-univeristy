#pragma once

#include <vector>
#include "pointers.h"


namespace uc
{
    namespace lip
    {

        template <typename t> class unique_lip_pointer
        {
            public:

            unique_lip_pointer()
            {

            }

            unique_lip_pointer(std::vector<uint8_t>&& memory) : m_pointer_memory(std::move(memory)), m_pointer(make_unique<t>((void*) &m_pointer_memory[0]))
            {

            }

            unique_lip_pointer(unique_lip_pointer&& o) : m_pointer_memory( std::move( o.m_pointer_memory)), m_pointer( std::move(o.m_pointer))
            {

            }

            unique_lip_pointer& operator=(unique_lip_pointer&& o)
            {
                m_pointer_memory = std::move(o.m_pointer_memory);
                m_pointer = std::move(o.m_pointer);
                return *this;
            }


            t* operator->()
            {
                return m_pointer.get();
            }

            const t* operator->() const
            {
                return m_pointer.get();
            }

            t* get() const
            {
                return m_pointer.get();
            }

            explicit operator bool() const throw()
            {
                // test for non-null pointer
                return ( m_pointer.get() != lip::reloc_pointer< t >() );
            }

            private:

            std::vector<uint8_t>    m_pointer_memory;
            lip::reloc_pointer< t > m_pointer;

            unique_lip_pointer(const unique_lip_pointer&) = delete;
            unique_lip_pointer& operator=(const unique_lip_pointer&) = delete;

            //todo: other member like unique_ptr
        };

        template <typename t> inline unique_lip_pointer<t> make_unique_lip_pointer( std::vector<uint8_t>&& memory )
        {
            return unique_lip_pointer<t>(std::move(memory));
        }
    }

}
