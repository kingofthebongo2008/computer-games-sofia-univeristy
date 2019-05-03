#pragma once

#include <cstdint>

#include "base_class_info_base.h"

namespace uc
{
    namespace lip
    {
        template <typename t>
        class base_class_typed_info_base : public base_class_info_base
        {

            using base = base_class_info_base;

            public:
                base_class_typed_info_base( type_info::offset_t offset )
                :   base(make_type_info<t>(), offset )
                {

                }
        };

        template <typename t, typename base_class_type_t>
        class base_class_info : public base_class_typed_info_base< base_class_type_t>
        {
            using base = base_class_typed_info_base< base_class_type_t>;

        public:
            base_class_info( )
                : base( static_cast<type_info::offset_t> (LIP_OFFSET_OF_BASE(t, base_class_type_t)) )
            {

            }
        };

        template <typename t, typename base_class_type_t>
        inline auto make_base_class_info()
        {
            return base_class_info<t, base_class_type_t>();
        }



    }
}



