#pragma once

namespace uc
{
    namespace lip
    {
        struct load_context
        {
            load_context()
            {

            }

            explicit load_context(void* pointer) : m_base_pointer(pointer)
            {

            }

            void* base_pointer() const
            {
                return m_base_pointer;
            }

            void* m_base_pointer  = nullptr;
        };


        //todo:
        struct move_context
        {
            move_context(const void* , const void* )
            {

            }

            const void* m_base_pointer;
        };
    }
}



