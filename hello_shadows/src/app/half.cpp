#include "pch.h"

#include "half.h"



namespace uc
{
    namespace math
    {
        namespace details2
        {
            math::half  base_table[512];
            uint8_t	    shift_table[512];

            static void generate_tables_1()
            {
                uint32_t	i;
                int32_t	e;

                for (i = 0; i < 256; ++i)
                {
                    e = i - 127;

                    // Very small numbers map to zero

                    if (e < -24)
                    {
                        base_table[i | 0x000] = 0x0000;
                        base_table[i | 0x100] = 0x8000;
                        shift_table[i | 0x000] = 24;
                        shift_table[i | 0x100] = 24;
                    }
                    // Small numbers map to denorms
                    else if (e < -14)
                    {
                        base_table[i | 0x000] = (0x0400 >> (-e - 14));
                        base_table[i | 0x100] = (0x0400 >> (-e - 14)) | 0x8000;
                        shift_table[i | 0x000] = static_cast<uint8_t> (-e - 1);
                        shift_table[i | 0x100] = static_cast<uint8_t> (-e - 1);
                    }

                    // Normal numbers just lose precision
                    else if (e <= 15)
                    {
                        base_table[i | 0x000] = static_cast<half> (((e + 15) << 10));
                        base_table[i | 0x100] = static_cast<half> (((e + 15) << 10) | 0x8000);
                        shift_table[i | 0x000] = 13;
                        shift_table[i | 0x100] = 13;
                    }

                    // Large numbers map to Infinity
                    else if (e < 128)
                    {
                        base_table[i | 0x000] = 0x7C00;
                        base_table[i | 0x100] = 0xFC00;
                        shift_table[i | 0x000] = 24;
                        shift_table[i | 0x100] = 24;
                    }
                    // Infinity and NaN's stay Infinity and NaN's
                    else
                    {
                        base_table[i | 0x000] = 0x7C00;
                        base_table[i | 0x100] = 0xFC00;
                        shift_table[i | 0x000] = 13;
                        shift_table[i | 0x100] = 13;
                    }
                }
            }

            uint32_t	mantissa_table[2048];
            uint32_t	exponent_table[64];
            uint16_t	offset_table[64];

            static uint32_t convert_mantissa(uint32_t i)
            {
                uint32_t m = i << 13;				// Zero pad mantissa bits
                uint32_t e = 0;					// Zero exponent

                while (!(m & 0x00800000))		// While not normalized
                {
                    e -= 0x00800000;				// Decrement exponent (1<<23)
                    m <<= 1;						// Shift mantissa
                }

                m &= ~0x00800000;				// Clear leading 1 bit
                e += 0x38800000;					// Adjust bias ((127-14)<<23)

                return m | e;
            }

            static void generate_tables_2()
            {
                // mantissa table
                mantissa_table[0] = 0;

                for (uint32_t i = 1; i <= 1023; ++i)
                {
                    mantissa_table[i] = convert_mantissa(i);
                }

                for (uint32_t i = 1024; i < 2048; ++i)
                {
                    mantissa_table[i] = 0x38000000 + ((i - 1024) << 13);
                }

                // exponent table
                exponent_table[0] = 0;
                exponent_table[32] = 0x80000000;

                for (uint32_t i = 1; i <= 30; ++i)
                {
                    exponent_table[i] = i << 23;
                }

                for (uint32_t i = 33; i <= 62; ++i)
                {
                    exponent_table[i] = 0x80000000 + ((i - 32) << 23);
                }

                exponent_table[31] = 0x47800000;
                exponent_table[63] = 0xC7800000;

                //offset table
                for (uint32_t i = 0; i < 64; ++i)
                {
                    offset_table[i] = 1024;
                }

                offset_table[0] = 0;

                offset_table[32] = 0;

            }

            void generate_tables()
            {
                generate_tables_1();
                generate_tables_2();
            }
        }
    }
}


