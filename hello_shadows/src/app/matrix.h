#pragma once

#include "vector.h"

namespace uc
{
    namespace math
    {
        struct alignas(16) float4x4
        {
                float4 r[4];
        };

// Fix-up for (1st) XMMATRIX parameter to pass in-register for ARM64 and vector call; by reference otherwise
//#if defined(_X86) || defined(_X64) || defined(_ARM) || defined(_ARM64)
#if defined(_X64) || defined(_ARM) || defined(_ARM64)
        using afloat4x4 = const float4x4;
#else
        using afloat4x4 = const float4x4&;
#endif

        // Fix-up for (2nd+) XMMATRIX parameters to pass by reference
        using bfloat4x4 = const float4x4&;

        
        inline float4x4 UC_MATH_CALL identity_matrix()
        {
            float4x4 m;

            m.r[0] = identity_r0();
            m.r[1] = identity_r1();
            m.r[2] = identity_r2();
            m.r[3] = identity_r3();

            return m;
        }

        inline float4x4 UC_MATH_CALL zero_matrix()
        {
            float4x4 m;

            m.r[0] = zero();
            m.r[1] = zero();
            m.r[2] = zero();
            m.r[3] = zero();

            return m;
        }

        inline float4x4 UC_MATH_CALL swap_x_y()
        {
            float4x4 m;

            m.r[0] = identity_r1();
            m.r[1] = identity_r0();
            m.r[2] = identity_r2();
            m.r[3] = identity_r3();

            return m;
        }

        inline float4x4 UC_MATH_CALL swap_x_z()
        {
            float4x4 m;

            
            m.r[0] = identity_r2();
            m.r[1] = identity_r1();
            m.r[2] = identity_r0();
            m.r[3] = identity_r3();

            return m;
        }

        inline float4x4 UC_MATH_CALL swap_y_z()
        {
            float4x4 m;

            m.r[0] = identity_r0();
            m.r[1] = identity_r2();
            m.r[2] = identity_r1();
            m.r[3] = identity_r3();

            return m;
        }

        inline float4x4 UC_MATH_CALL load44(const float* __restrict address)
        {
            float4x4 m;

            m.r[0] = load4(address);
            m.r[1] = load4(address + 4);
            m.r[2] = load4(address + 8);
            m.r[3] = load4(address + 12);

            return m;
        }

        inline float4x4 UC_MATH_CALL load44u(const float* __restrict address)
        {
            float4x4 m;

            m.r[0] = load4u(address);
            m.r[1] = load4u(address + 4);
            m.r[2] = load4u(address + 8);
            m.r[3] = load4u(address + 12);

            return m;
        }

        inline float4x4 UC_MATH_CALL load43(const float* __restrict address)
        {
            float4x4 m;

            m.r[0] = load4(address);
            m.r[1] = load4(address + 4);
            m.r[2] = load4(address + 8);
            m.r[3] = identity_r3();

            return m;
        }

        inline void UC_MATH_CALL store44(float* __restrict address, afloat4x4 m)
        {
            store4(address, m.r[0]);
            store4(address + 4, m.r[1]);
            store4(address + 8, m.r[2]);
            store4(address + 12, m.r[3]);
        }

        inline void UC_MATH_CALL store43(float* __restrict address, afloat4x4 m)
        {
            store4(address, m.r[0]);
            store4(address + 4, m.r[1]);
            store4(address + 8, m.r[2]);
        }

        inline float4x4 UC_MATH_CALL transpose(float4x4 m)
        {
            float4x4 m1;

            float4 v1 = shuffle<x, y, x, y>(m.r[0], m.r[1]);
            float4 v2 = shuffle<x, y, x, y>(m.r[2], m.r[3]);
            float4 v3 = shuffle<z, w, z, w>(m.r[0], m.r[1]);
            float4 v4 = shuffle<z, w, z, w>(m.r[2], m.r[3]);

            m1.r[0] = shuffle<x, z, x, z>(v1, v2);
            m1.r[1] = shuffle<y, w, y, w>(v1, v2);
            m1.r[2] = shuffle<x, z, x, z>(v3, v4);
            m1.r[3] = shuffle<y, w, y, w>(v3, v4);

            return m1;
        }

        inline float4x4 UC_MATH_CALL inverse(afloat4x4 m)
        {
            float4 tmp;
            float4 det;
            float4 minor0;
            float4 minor1;
            float4 minor2;
            float4 minor3;
            float4 row0;
            float4 row1;
            float4 row2;
            float4 row3;

            //------ Transposition
            float4x4 m1 = transpose(m);

            row0 = m1.r[0];
            row1 = swizzle<z, w, x, y>(m1.r[1]);
            row2 = m1.r[2];
            row3 = swizzle<z, w, x, y>(m1.r[3]);

            //------ 
            tmp = mul(row2, row3);
            tmp = swizzle<y, x, w, z>(tmp); //b1

            minor0 = mul(row1, tmp);
            minor1 = mul(row0, tmp);

            tmp = swizzle<z, w, x, y>(tmp); //4e

            minor0 = sub(mul(row1, tmp), minor0);
            minor1 = sub(mul(row0, tmp), minor1);

            minor1 = swizzle<z, w, x, y>(minor1); //4e

            //------ 
            tmp = mul(row1, row2);
            tmp = swizzle<y, x, w, z>(tmp);

            minor0 = add(mul(row3, tmp), minor0);
            minor3 = mul(row0, tmp);

            tmp = swizzle<z, w, x, y>(tmp);

            minor0 = sub(minor0, mul(row3, tmp));
            minor3 = sub(mul(row0, tmp), minor3);
            minor3 = swizzle<z, w, x, y>(minor3);

            //------ 
            tmp = mul(swizzle<z, w, x, y>(row1), row3);
            tmp = swizzle<y, x, w, z>(tmp);
            row2 = swizzle<z, w, x, y>(row2);

            minor0 = add(mul(row2, tmp), minor0);
            minor2 = mul(row0, tmp);

            tmp = swizzle<z, w, x, y>(tmp);

            minor0 = sub(minor0, mul(row2, tmp));
            minor2 = sub(mul(row0, tmp), minor2);
            minor2 = swizzle<z, w, x, y>(minor2);

            //------ 

            tmp = mul(row0, row1);
            tmp = swizzle<y, x, w, z>(tmp);

            minor2 = add(mul(row3, tmp), minor2);
            minor3 = sub(mul(row2, tmp), minor3);

            tmp = swizzle<z, w, x, y>(tmp);
            minor2 = sub(mul(row3, tmp), minor2);
            minor3 = sub(minor3, mul(row2, tmp));

            //------ 

            tmp = mul(row0, row3);
            tmp = swizzle<y, x, w, z>(tmp);

            minor1 = sub(minor1, mul(row2, tmp));
            minor2 = add(mul(row1, tmp), minor2);

            tmp = swizzle<z, w, x, y>(tmp);
            minor1 = add(mul(row2, tmp), minor1);
            minor2 = sub(minor2, mul(row1, tmp));

            //------ 
            tmp = mul(row0, row2);
            tmp = swizzle<y, x, w, z>(tmp);

            minor1 = add(mul(row3, tmp), minor1);
            minor3 = sub(minor3, mul(row1, tmp));

            tmp = swizzle<z, w, x, y>(tmp);

            minor1 = sub(minor1, mul(row3, tmp));
            minor3 = add(mul(row1, tmp), minor3);
            //------ 

            det = mul(row0, minor0);
            det = add(swizzle<z, w, x, y>(det), det);
            det = _mm_add_ss(swizzle<y, x, z, w>(det), det);
            tmp = _mm_rcp_ss(det);

            det = _mm_sub_ss(_mm_add_ss(tmp, tmp), _mm_mul_ss(det, _mm_mul_ss(tmp, tmp)));
            det = swizzle<x, x, x, x>(det);

            float4x4 m2;

            m2.r[0] = mul(det, minor0);
            m2.r[1] = mul(det, minor1);
            m2.r[2] = mul(det, minor2);
            m2.r[3] = mul(det, minor3);

            return m2;

        }

        inline float4 UC_MATH_CALL mul(afloat4 v, afloat4x4 m)
        {
            float4  v1 = swizzle<x, x, x, x>(v);
            float4  v2 = swizzle<y, y, y, y>(v);
            float4  v3 = swizzle<z, z, z, z>(v);
            float4  v4 = swizzle<w, w, w, w>(v);

            float4  v5 = mul(v1, m.r[0]);
            float4  v6 = mul(v2, m.r[1]);
            float4  v7 = mul(v3, m.r[2]);
            float4  v8 = mul(v4, m.r[3]);

            float4  v9 = add(v5, v6);
            float4  v10 = add(v7, v8);
            float4  v11 = add(v9, v10);

            return v11;
        }

        inline float4 UC_MATH_CALL mul(afloat4x4 m, afloat4 v)
        {
            float4x4 trans = transpose(m);
            return mul(v, trans);
        }

        inline float4x4 UC_MATH_CALL mul(afloat4x4 m1, bfloat4x4 m2)
        {
            float4x4 m;

            m.r[0] = mul(m1.r[0], m2);
            m.r[1] = mul(m1.r[1], m2);
            m.r[2] = mul(m1.r[2], m2);
            m.r[3] = mul(m1.r[3], m2);

            return m;
        }

        inline float4x4 UC_MATH_CALL add(afloat4x4 m1, bfloat4x4 m2)
        {
            float4x4 m;

            m.r[0] = add(m1.r[0], m2.r[0]);
            m.r[1] = add(m1.r[1], m2.r[1]);
            m.r[2] = add(m1.r[2], m2.r[2]);
            m.r[3] = add(m1.r[3], m2.r[3]);

            return m;
        }

        inline float4x4 UC_MATH_CALL matrix44_sub(afloat4x4 m1, bfloat4x4 m2)
        {
            float4x4 m;

            m.r[0] = sub(m1.r[0], m2.r[0]);
            m.r[1] = sub(m1.r[1], m2.r[1]);
            m.r[2] = sub(m1.r[2], m2.r[2]);
            m.r[3] = sub(m1.r[3], m2.r[3]);

            return m;
        }

        inline float4x4 UC_MATH_CALL matrix44_mad(afloat4x4 m1, bfloat4x4 m2, bfloat4x4 m3)
        {
            float4x4 m4 = mul(m1, m2);
            float4x4 m5 = add(m4, m3);
            return m5;
        }
    }
}

