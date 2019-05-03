#pragma once

#include <cstdint>

#include "member_info_base.h"

namespace uc
{
    namespace lip
    {
        template<typename t, typename member_type_t >
        class member_info : public member_info_typed_base<member_type_t>
        {
            using base = member_info_typed_base<member_type_t>;

            public:

            member_info( const char* name, type_info::offset_t offset ) : base(name,offset)
            {

            }
        };

        template <typename t, typename member_type_t>
        inline auto make_member_info(const char* name, member_type_t( t::*member_ptr ) )
        {
            return member_info<t, member_type_t>(name, static_cast<type_info::offset_t> ( LIP_OFFSET_OF(t, *member_ptr)));
        }

        template <typename t, typename member_type_t>
        inline auto make_member_info_ptr(const char* name, member_type_t(t::*member_ptr))
        {
            return new member_info<t, member_type_t>(name, static_cast<type_info::offset_t> (LIP_OFFSET_OF(t, *member_ptr)));
        }

        template<typename t, typename member_type_t, std::size_t s >
        class array_member_info : public member_info_typed_base<member_type_t>
        {
            using base = member_info_typed_base<member_type_t>;

            public:

            array_member_info( const char* name, type_info::offset_t offset ) : base(name, offset, member_info_typed_base_traits<member_type_t>::is_smart_member , true, 0, s )
            {

            }

            size_t size() const
            {
                return s;
            }
        };

        template <typename t, typename member_type_t, size_t size>
        inline auto make_array_member_info(const char* name, member_type_t(t::*member_ptr)[size] )
        {
            return array_member_info<t, member_type_t, size>(name, static_cast<type_info::offset_t>(LIP_OFFSET_OF(t, *member_ptr)) );
        }

        template <typename t, typename member_type_t, size_t size>
        inline auto make_array_member_info_ptr(const char* name, member_type_t(t::*member_ptr)[size])
        {
            return new array_member_info<t, member_type_t, size>(name, static_cast<type_info::offset_t>(LIP_OFFSET_OF(t, *member_ptr)));
        }

    }
}



