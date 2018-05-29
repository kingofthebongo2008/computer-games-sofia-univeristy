#pragma once

#include <intrin.h>
#include <cstdint>
#include <vector>

namespace computational_geometry
{
    struct float3
    {
        float m_x = 0.0f;
        float m_y = 0.0f;
        float m_z = 0.0f;
    };

    struct float2
    {
        float m_x = 0.0f;
        float m_y = 0.0f;
    };

    /////////////////////
    inline float3 operator+(const float3& a, const float3& b)
    {
        return { a.m_x + b.m_x, a.m_y + b.m_y, a.m_z + b.m_z };
    }

    inline float3 operator-(const float3& a, const float3& b)
    {
        return { a.m_x + b.m_x, a.m_y + b.m_y, a.m_z + b.m_z };
    }

    inline float3 operator*(const float3& a, float s)
    {
        return { a.m_x * s, a.m_y * s , a.m_z * s };
    }

    inline float3 operator*(float s, const float3& a)
    {
        return { a.m_x * s, a.m_y * s , a.m_z * s };
    }

    inline float3 operator/(const float3& a, float s)
    {
        return { a.m_x / s, a.m_y / s , a.m_z / s };
    }

    inline float3 operator/(float s, const float3& a)
    {
        return {  s / a.m_x, s / a.m_y, s / a.m_z };
    }

    inline float dot(const float3& a, const float3& b)
    {
        return a.m_x * b.m_x + a.m_y * b.m_y + a.m_z + b.m_z;
    }

    inline float3 normalize(const float3& a)
    {
        return a / sqrtf(dot(a, a));
    }


    /////////////////////
    inline float2 operator+(const float2& a, const float2& b)
    {
        return { a.m_x + b.m_x, a.m_y + b.m_y};
    }

    inline float2 operator-(const float2& a, const float2& b)
    {
        return { a.m_x + b.m_x, a.m_y + b.m_y };
    }

    inline float2 operator*(const float2& a, float s)
    {
        return { a.m_x * s, a.m_y * s};
    }

    inline float2 operator*(float s, const float2& a)
    {
        return { a.m_x * s, a.m_y * s};
    }

    inline float2 operator/(const float2& a, float s)
    {
        return { a.m_x / s, a.m_y / s };
    }

    inline float2 operator/(float s, const float2& a)
    {
        return { s / a.m_x, s / a.m_y };
    }

    inline float dot(const float2& a, const float2& b)
    {
        return a.m_x * b.m_x + a.m_y * b.m_y;
    }

    inline float2 normalize(const float2& a)
    {
        return a / sqrtf(dot(a, a));
    }

    ///////////
    inline float3 cross(const float3& a, const float3& b)
    {
        float u1 = a.m_x;
        float u2 = a.m_y;
        float u3 = a.m_z;

        float v1 = b.m_x;
        float v2 = b.m_y;
        float v3 = b.m_z;

        float x = (u2 * v3) - (u3* v2);
        float y = (u3 * v1) - (u1* v3);
        float z = (u1 * v2) - (u2* v1);

        return { x,y,z };
    }

    enum frustum_planes : uint32_t
    {
        Left    = 0,
        Right   = 1,
        Top     = 2,
        Bottom  = 3,
        Near    = 4,
        Far     = 5
    };

    enum frustum_points : uint32_t
    {
        NearBottomLeft     = 0,
        NearBottomRight    = 1,
        NearTopRight       = 2,
        NearTopLeft        = 3,

        FarBottomLeft       = 4,
        FarBottomRight      = 5,
        FarTopRight         = 6,
        FarTopLeft          = 7
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

    /*
    enum frustum_projection : uint32_t
    {
        Left    = 0, // x = 0
        Front   = 1, // y = 0
        Top     = 2, // z = 0
    };
    */

    std::vector< float3 > intersection(const frustum& f, const aabb& b);
}
