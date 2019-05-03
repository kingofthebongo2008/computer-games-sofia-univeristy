#pragma once

#include <cstdint>

#include "type_info.h"

namespace uc
{
    namespace lip
    {
        class base_class_info_base
        {

            public:
            base_class_info_base(type_info info, type_info::offset_t offset)
                : m_type_info(info)
                , m_offset(offset)
            {
            }

            type_info::alignment_t alignment() const
            {
                return m_type_info.alignment();
            }

            bool is_polymorphic() const
            {
                return m_type_info.is_polymorphic();
            }

            const char* name() const
            {
                return m_type_info.name();
            }

            type_info::offset_t	offset() const
            {
                return m_offset;
            }

            unsigned int type_id() const
            {
                return m_type_info.type_id();
            }

        protected:

            type_info           m_type_info;
            type_info::offset_t m_offset;
        };

    }
}



