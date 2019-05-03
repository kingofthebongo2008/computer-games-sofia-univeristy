#pragma once

#include <cstdint>

#include "type_info.h"

namespace uc
{
    namespace lip
    {
        class member_info_base
        {
        public:

            member_info_base(
                const char*         name,
                type_info::offset_t	offset,
                type_info           type_info,
                bool                is_smart_member = false,
                bool                is_array = false,
                uint32_t            flags = 0,
                uint32_t            array_size = 0
            ) :
                m_type_info(type_info)
                , m_name(name)
                , m_offset(offset)
                , m_flags(flags)
                , m_is_array(is_array)
                , m_is_smart_member(is_smart_member)
                , m_array_size(array_size)
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

             const char* const name() const
            {
                return m_name;
            }

            type_info::offset_t offset () const
            {
                 return m_offset;
            }

            
            type_identifier type_id() const
            {
                return m_type_info.type_id();
            }
            

            uint32_t flags()const
            {
                return m_flags;
            }

            type_info::size_t size() const
            {
                return m_type_info.size();
            }

            bool is_array() const
            {
                return m_is_array;
            }

            bool is_smart_member() const
            {
                return m_is_smart_member;
            }

            uint32_t array_size() const
            {
                return m_array_size;
            }

            void write_object(const void* root_object, size_t offset, void* ctx) const
            {
                do_write_object(root_object, offset, ctx);
            }

        private:
            type_info               m_type_info;
            const char*		        m_name;
            type_info::offset_t     m_offset;
            uint32_t                m_flags;
            bool                    m_is_array;
            bool                    m_is_smart_member;  //handles custom binarization
            uint32_t                m_array_size;

        protected:

            virtual void do_write_object(const void*, size_t , void* ) const
            {

            }
        };


        template <typename t>
        struct member_info_typed_base_traits
        {
            static const constexpr bool is_smart_member = false;
        };


        template <>
        struct member_info_typed_base_traits < char >
        {
            static const constexpr bool is_smart_member = true;
        };

        template <>
        struct member_info_typed_base_traits < unsigned char >
        {
            static const constexpr bool is_smart_member = true;
        };

        template <>
        struct member_info_typed_base_traits < short >
        {
            static const constexpr bool is_smart_member = true;
        };

        template <>
        struct member_info_typed_base_traits < unsigned short >
        {
            static const constexpr bool is_smart_member = true;
        };

        template <>
        struct member_info_typed_base_traits < int >
        {
            static const constexpr bool is_smart_member = true;
        };

        template <>
        struct member_info_typed_base_traits < unsigned int >
        {
            static const constexpr bool is_smart_member = true;
        };

        template <>
        struct member_info_typed_base_traits < long >
        {
            static const constexpr bool is_smart_member = true;
        };

        template <>
        struct member_info_typed_base_traits < unsigned long >
        {
            static const constexpr bool is_smart_member = true;
        };

        template <>
        struct member_info_typed_base_traits < long long >
        {
            static const constexpr bool is_smart_member = true;
        };

        template <>
        struct member_info_typed_base_traits < unsigned long long >
        {
            static const constexpr bool is_smart_member = true;
        };

        template <>
        struct member_info_typed_base_traits < float >
        {
            static const constexpr bool is_smart_member = true;
        };

        template <>
        struct member_info_typed_base_traits < double >
        {
            static const constexpr bool is_smart_member = true;
        };

        template <>
        struct member_info_typed_base_traits < bool >
        {
            static const constexpr bool is_smart_member = true;
        };

        template <>
        struct member_info_typed_base_traits < signed char >
        {
            static const constexpr bool is_smart_member = true;
        };

        template <typename t> struct write_smart_object
        {
            static void apply(const void* root_object, size_t offset, void* ctx)
            {
                root_object;
                offset;
                ctx;
            }
        };
        

        template <typename t>
        class member_info_typed_base : public member_info_base
        {
            using base = member_info_base;

            public:

            member_info_typed_base(const char*	name, type_info::offset_t offset, bool is_smart_member = member_info_typed_base_traits<t>::is_smart_member, bool is_array = false, uint32_t flags = 0, uint32_t array_size = 0)
                : base(name, offset, make_type_info<t>(), is_smart_member, is_array, flags, array_size )
            {

            }
             
            void do_write_object(const void* root_object, size_t offset, void* ctx) const override
            {
                constexpr const bool is_smart_member = member_info_typed_base_traits<t>::is_smart_member;

                if ( is_smart_member )
                {
                    write_smart_object<t>::apply( root_object, offset, ctx );
                }
            }
            
        };
    }
}



