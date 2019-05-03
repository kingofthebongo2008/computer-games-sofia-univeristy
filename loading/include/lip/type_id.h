#pragma once

#include <cstdint>
#include <exception>
#include <stdexcept>

#include <uc_dev/fnd/string_hash.h>

namespace uc
{
    namespace lip
    {
        namespace details
        {
            class constexpr_string
            { 
                // constexpr string
                private:
                    const char* const m_string;
                    const std::size_t m_size;

                public:
                    template<std::size_t n>
                    constexpr constexpr_string(const char(&a)[n]) : m_string(a), m_size(n-1)
                    {

                    }
  
                    constexpr const char operator[](std::size_t n) const
                    {
                        return n < m_size ? m_string[n] : throw std::out_of_range("");
                    }

                    constexpr const std::size_t size() const
                    {
                        return m_size;
                    }

                    constexpr const char* c_str() const
                    {
                        return m_string;
                    }
            };

            template <typename t> constexpr const char* type_name()
            {
                return typeid(t).name();
            }
        }

        template <typename t> struct type_id;

        template <> struct type_id<char>
        {
            enum { value = 1 };

            static constexpr auto type_name()
            {
                return details::type_name<char>();
            }
        };

        template <> struct type_id<unsigned char>
        {
            enum { value = 2 };

            static constexpr auto type_name()
            {
                return details::type_name<unsigned char>();
            }
        };

        template <> struct type_id<short>
        {
            enum { value = 3 };
            static constexpr auto type_name()
            {
                return details::type_name<short>();
            }
        };

        template <> struct type_id<unsigned short>
        {
            enum { value = 4 };
            static constexpr auto type_name()
            {
                return details::type_name<unsigned short>();
            }
        };

        template <> struct type_id<int>
        {
            enum { value = 5 };
            static constexpr auto type_name()
            {
                return details::type_name<int>();
            }
        };

        template <> struct type_id<unsigned int>
        {
            enum { value = 6 };
            static constexpr auto type_name()
            {
                return details::type_name<unsigned int>();
            }
        };

        template <> struct type_id<long>
        {
            enum { value = 7 };
            static constexpr auto type_name()
            {
                return details::type_name<long>();
            }
        };

        template <> struct type_id<unsigned long>
        {
            enum { value = 8 };
            static constexpr auto type_name()
            {
                return details::type_name<unsigned long>();
            }
        };

        template <> struct type_id<long long>
        {
            enum { value = 9 };
            static constexpr auto type_name()
            {
                return details::type_name<long long>();
            }
        };

        template <> struct type_id<unsigned long long>
        {
            enum { value = 10 };
            static constexpr auto type_name()
            {
                return details::type_name<unsigned long long>();
            }
        };

        template <> struct type_id<float>
        {
            enum { value = 11 };
            static constexpr auto type_name()
            {
                return details::type_name<float>();
            }
        };

        template <> struct type_id<double>
        {
            enum { value = 12 };
            static constexpr auto type_name()
            {
                return details::type_name<double>();
            }
        };

        template <> struct type_id<bool>
        {
            enum { value = 13 };
            static constexpr auto type_name()
            {
                return details::type_name<bool>();
            }
        };

        template <> struct type_id<signed char>
        {
            enum { value = 14 };
            static constexpr auto type_name()
            {
                return details::type_name<signed char>();
            }
        };

        using type_identifier = std::uint32_t;
    }
}

#if !defined(LIP_DECLARE_TYPE_ID2)
#define LIP_DECLARE_TYPE_ID2(type,id)\
template <> struct type_id<type>\
{\
    constexpr static uint32_t value = id;\
    static constexpr auto type_name()\
    {\
        return details::type_name<type>(); \
    }\
};
#endif

#if !defined(LIP_DECLARE_TYPE_ID)
#define LIP_DECLARE_TYPE_ID(type) LIP_DECLARE_TYPE_ID2(type, generate_hash(#type))
#endif

#if !defined(LIP_OFFSET_OF)
//    #define LIP_OFFSET_OF offsetof
    #define LIP_OFFSET_OF(s,m) ((size_t)&(((s*)0)->m))
#endif

#if !defined(LIP_OFFSET_OF_BASE)
#define LIP_OFFSET_OF_BASE(Super,Base) (size_t)(Base*)(Super*)1 -1 
#endif
