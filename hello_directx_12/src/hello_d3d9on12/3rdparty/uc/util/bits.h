#pragma once

#include <cstdint>

namespace uc
{

    namespace util
    {
        template<uint32_t bit, typename t> inline bool bit_is_set(t value);
        template<uint32_t bit, typename t> inline t bit_reset(t value);
        template<uint32_t bit, typename t> inline t bit_set(t value);

        template<uint32_t bit, typename t> inline bool bit_is_set(uint32_t value)
        {
            return (value & (1 << bit)) != 0;
        }

        template<uint32_t bit, typename t> inline uint32_t bit_reset(uint32_t value)
        {
            return value  & (~(1 << bit));
        }

        template<uint32_t bit, typename t> inline uint32_t bit_set(uint32_t value)
        {
            return value | (1 << bit);
        }

        template<uint16_t bit, typename t> inline bool bit_is_set(uint16_t value)
        {
            return (value & (1 << bit)) != 0;
        }

        template<uint16_t bit, typename t> inline uint16_t bit_reset(uint16_t value)
        {
            return value  & (~(1 << bit));
        }

        template<uint16_t bit, typename t> inline uint16_t bit_set(uint16_t value)
        {
            return value | (1 << bit);
        }

        template<uint64_t bit, typename t> inline bool bit_is_set(uint64_t value)
        {
            return (value & (static_cast<uint64_t> (1) << bit)) != 0;
        }

        template<uint64_t bit, typename t> inline uint64_t bit_reset(uint64_t value)
        {
            return value  & (~(static_cast<uint64_t> (1) << bit));
        }

        template<uint64_t bit, typename t> inline uint64_t bit_set(uint64_t value)
        {
            return value | (static_cast<uint64_t> (1) << bit);
        }

        template<uint8_t bit, typename t> inline bool bit_is_set(uint8_t value)
        {
            return (value & (1 << bit)) != 0;
        }

        template<uint8_t bit, typename t> inline uint8_t bit_reset(uint8_t value)
        {
            return value  & (~(1 << bit));
        }

        template<unsigned char bit, typename t> inline unsigned char bit_set(unsigned char value)
        {
            return value | (1 << bit);
        }

        template<uint32_t bit, typename t> inline t bit_set(t value, bool on)
        {
            if (on)
            {
                return bit_set<bit>();
            }
            else
            {
                return bit_reset<bit>();
            }
        }

        template<uint32_t bit, typename t, bool on> inline t bit_set(t value)
        {
            if (on)
            {
                return bit_set<bit>();
            }
            else
            {
                return bit_reset<bit>();
            }
        }

        inline bool bit_is_set(uint32_t value, uint32_t bit)
        {
            return (value & (1U << bit)) != 0;
        }

        inline bool bit_is_set(uint64_t value, uint32_t bit)
        {
            return (value & (1ULL << bit)) != 0;
        }

        inline uint32_t bit_reset(uint32_t value, uint32_t bit)
        {
            return value & (~(1U << bit));
        }

        inline uint64_t bit_reset(uint64_t value, uint32_t bit)
        {
            return value & (~(1ULL << bit));
        }

        inline uint32_t bit_set(uint32_t value, uint32_t bit)
        {
            return value | (1U << bit);
        }

        inline uint64_t bit_set(uint64_t value, uint32_t bit)
        {
            return value | (1ULL << bit);
        }
    }
}


