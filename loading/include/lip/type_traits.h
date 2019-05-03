#pragma once

#include <type_traits>

namespace uc
{
    namespace lip
    {
        template <typename t> struct is_abstract
        {
            static const constexpr bool value = std::is_abstract<t>::value;
        };

        template <typename t> struct is_array
        {
            static const constexpr bool value = std::is_array<t>::value;
        };

        template <typename t> struct is_arithmetic
        {
            static const constexpr bool value = std::is_arithmetic<t>::value;
        };

        template <typename t> struct is_class
        {
            static const constexpr bool value = std::is_class<t>::value;
        };

        template <typename t> struct is_enum
        {
            static const constexpr bool value = std::is_enum<t>::value;
        };

        template <typename t> struct is_fundamental
        {
            static const constexpr bool value = std::is_fundamental<t>::value;
        };

        template <typename t> struct is_polymorphic
        {
            static const constexpr bool value = std::is_polymorphic<t>::value;
        };

        template <typename t> struct is_pod
        {
            static const constexpr bool value = std::is_pod<t>::value;
        };

        template <typename t> struct is_union
        {
            static const constexpr bool value = std::is_union<t>::value;
        };

        template <typename b, typename d> struct is_base_and_derived
        {
            static const constexpr bool value = std::is_base_and_derived<b, d>::value;
        };

        template <typename t > struct alignment_of
        {
            static const constexpr size_t value = std::alignment_of<t>::value;
        };


    }
}

