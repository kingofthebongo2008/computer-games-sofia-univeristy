#pragma once

#include "math.h"

#include <memory>
#include <malloc.h>

namespace uc
{
    namespace math
    {
        namespace details
        {
            struct aligned_object
            {
                void operator () (void* d) const
                {
                    _aligned_free(d);
                }
            };
        }

        using managed_float4x4 = std::unique_ptr< math::float4x4, details::aligned_object >;

        inline managed_float4x4 make_float4x4()
        {
            math::float4x4* a = reinterpret_cast<float4x4*> (_aligned_malloc(sizeof(math::float4x4), 16));
            return managed_float4x4(a, details::aligned_object());
        }
    }
}

