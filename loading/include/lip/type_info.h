#pragma once

#include <cstdint>

#include "type_id.h"
#include "type_traits.h"

namespace uc
{
    namespace lip
    {
        class type_info
        {

            public:

            using alignment_t = std::uint16_t;
            using size_t      = std::uint16_t;
            using offset_t    = std::uint16_t;

            private:

                const char *		m_name;			//name of the type for the tools
                type_identifier		m_type_id;		//type id of the type, crc of the typename
                alignment_t		    m_alignment;	//alignment of the type
                size_t  		    m_size;			//size of the type
                uint16_t		    m_traits;		//type traits

                static constexpr const uint32_t bit_is_polymorphic = 1;
                static constexpr const uint32_t bit_is_enum = 2;
                static constexpr const uint32_t bit_is_arithmetic = 3;
                static constexpr const uint32_t bit_is_abstract = 4;
                static constexpr const uint32_t bit_is_class = 5;
                static constexpr const uint32_t bit_is_union = 6;
                static constexpr const uint32_t bit_is_pod = 7;

                template<uint32_t bit> bool bit_is_set() const
                {
                    return (m_traits & (1 << bit)) != 0;
                }

                template<uint32_t bit> void bit_reset()
                {
                    m_traits = m_traits &  (~(1 << bit));
                }

                template<uint32_t bit> void bit_set()
                {
                    m_traits = m_traits | (1 << bit);
                }

                template<uint32_t bit> void bit_set( bool value )
                {
                    if (value)
                    {
                        bit_set<bit>();
                    }
                    else
                    {
                        bit_reset<bit>();
                    }
                }

                void set_is_union(bool isunion)
                {
                    bit_set<bit_is_union>(isunion);
                }

                void set_is_pod(bool ispod)
                {
                    bit_set<bit_is_pod>(ispod);
                }

                template < uint32_t bit > 
                uint16_t make_bit(bool value) const
                {
                    if (value)
                    {
                        return (1 << bit);
                    }
                    else
                    {
                        return 0;
                    }
                }


            private:

                type_info() = delete;

            public:

                type_info
                (
                    type_identifier id,
                    const char*     name,
                    alignment_t     alignment,
                    size_t          size,

                    bool            is_polymorphic,
                    bool            is_enum,
                    bool            is_arithmetic,
                    bool            is_abstract,
                    bool            is_class,
                    bool            is_union,
                    bool            is_pod
                ) : m_type_id(id)
                   , m_name(name)
                   , m_alignment(alignment)
                   , m_size(size)
                {
                    m_traits = make_bit< bit_is_polymorphic >(is_polymorphic) | make_bit< bit_is_enum >(is_enum) | make_bit< bit_is_arithmetic >(is_arithmetic) |
                               make_bit < bit_is_abstract>(is_abstract) | make_bit< bit_is_class >(is_class) | make_bit < bit_is_union >(is_union) | make_bit < bit_is_pod >(is_pod);
                }

                type_identifier type_id() const
                {
                    return m_type_id;
                }

                const char* const name() const
                {
                    return m_name;
                }

                alignment_t alignment() const
                {
                    return m_alignment;
                }

                size_t size() const
                {
                    return m_size;
                }

                bool	is_polymorphic() const
                {
                    return bit_is_set<bit_is_polymorphic>();
                }

                bool is_enum() const
                {
                    return bit_is_set<bit_is_enum>();
                }

                bool is_arithmetic() const
                {
                    return bit_is_set<bit_is_arithmetic>();
                }

                 bool is_abstract() const
                {
                    return bit_is_set<bit_is_abstract>();
                }

                 bool is_class() const
                {
                    return bit_is_set<bit_is_class>();
                }

                 bool is_union() const
                {
                    return bit_is_set<bit_is_union>();
                }

                 bool is_pod() const
                {
                    return bit_is_set<bit_is_pod>();
                }
        };

        template <typename t> type_info make_type_info(type_identifier type_id, const char*	name)
        {
            return type_info(
                type_id,
                name,
                alignment_of<t>::value,
                sizeof(t),
                is_polymorphic<t>::value,
                is_enum<t>::value,
                is_arithmetic<t>::value,
                is_abstract<t>::value,
                is_class<t>::value,
                is_union<t>::value,
                is_pod<t>::value);
        }

        template <typename t>  type_info make_type_info()
        {
            return make_type_info<t>( type_id<t>::value, type_id<t>::type_name() );
        }
    }
}

