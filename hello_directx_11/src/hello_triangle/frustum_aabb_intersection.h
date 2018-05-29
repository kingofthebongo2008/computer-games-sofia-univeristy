#pragma once

#include <intrin.h>
#include <cstdint>

namespace computational_geometry
{
    struct float3
    {
        float m_x = 0.0f;
        float m_y = 0.0f;
        float m_z = 0.0f;
    };

    enum class frustum_points : uint32_t
    {
        Left    = 0,
        Right   = 1,
        Top     = 2,
        Bottom  = 3,
        Near    = 4,
        Far     = 5
    };

    struct frustum
    {
        float3 m_points[8];
    };

    struct aabb
    {
        float3 m_min;
        float3 m_max;
    };
}
